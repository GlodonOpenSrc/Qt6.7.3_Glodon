/**
 * Copyright 2019 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import {Protocol} from 'devtools-protocol';

import type {Browser} from '../api/Browser.js';
import type {BrowserContext} from '../api/BrowserContext.js';
import {Page, PageEmittedEvents} from '../api/Page.js';
import {Target, TargetType} from '../api/Target.js';
import {Deferred} from '../util/Deferred.js';

import {CDPSession, CDPSessionImpl} from './Connection.js';
import {CDPPage} from './Page.js';
import {Viewport} from './PuppeteerViewport.js';
import {TargetManager} from './TargetManager.js';
import {TaskQueue} from './TaskQueue.js';
import {debugError} from './util.js';
import {WebWorker} from './WebWorker.js';

/**
 * @internal
 */
export enum InitializationStatus {
  SUCCESS = 'success',
  ABORTED = 'aborted',
}

/**
 * @internal
 */
export class CDPTarget extends Target {
  #browserContext?: BrowserContext;
  #session?: CDPSession;
  #targetInfo: Protocol.Target.TargetInfo;
  #targetManager?: TargetManager;
  #sessionFactory:
    | ((isAutoAttachEmulated: boolean) => Promise<CDPSession>)
    | undefined;

  /**
   * @internal
   */
  _initializedDeferred = Deferred.create<InitializationStatus>();
  /**
   * @internal
   */
  _isClosedDeferred = Deferred.create<void>();
  /**
   * @internal
   */
  _targetId: string;

  /**
   * To initialize the target for use, call initialize.
   *
   * @internal
   */
  constructor(
    targetInfo: Protocol.Target.TargetInfo,
    session: CDPSession | undefined,
    browserContext: BrowserContext | undefined,
    targetManager: TargetManager | undefined,
    sessionFactory:
      | ((isAutoAttachEmulated: boolean) => Promise<CDPSession>)
      | undefined
  ) {
    super();
    this.#session = session;
    this.#targetManager = targetManager;
    this.#targetInfo = targetInfo;
    this.#browserContext = browserContext;
    this._targetId = targetInfo.targetId;
    this.#sessionFactory = sessionFactory;
    if (this.#session && this.#session instanceof CDPSessionImpl) {
      this.#session._setTarget(this);
    }
  }

  /**
   * @internal
   */
  _subtype(): string | undefined {
    return this.#targetInfo.subtype;
  }

  /**
   * @internal
   */
  _session(): CDPSession | undefined {
    return this.#session;
  }

  /**
   * @internal
   */
  protected _sessionFactory(): (
    isAutoAttachEmulated: boolean
  ) => Promise<CDPSession> {
    if (!this.#sessionFactory) {
      throw new Error('sessionFactory is not initialized');
    }
    return this.#sessionFactory;
  }

  override createCDPSession(): Promise<CDPSession> {
    if (!this.#sessionFactory) {
      throw new Error('sessionFactory is not initialized');
    }
    return this.#sessionFactory(false).then(session => {
      (session as CDPSessionImpl)._setTarget(this);
      return session;
    });
  }

  override url(): string {
    return this.#targetInfo.url;
  }

  override type(): TargetType {
    const type = this.#targetInfo.type;
    switch (type) {
      case 'page':
        return TargetType.PAGE;
      case 'background_page':
        return TargetType.BACKGROUND_PAGE;
      case 'service_worker':
        return TargetType.SERVICE_WORKER;
      case 'shared_worker':
        return TargetType.SHARED_WORKER;
      case 'browser':
        return TargetType.BROWSER;
      case 'webview':
        return TargetType.WEBVIEW;
      case 'tab':
        return TargetType.TAB;
      default:
        return TargetType.OTHER;
    }
  }

  /**
   * @internal
   */
  _targetManager(): TargetManager {
    if (!this.#targetManager) {
      throw new Error('targetManager is not initialized');
    }
    return this.#targetManager;
  }

  /**
   * @internal
   */
  _getTargetInfo(): Protocol.Target.TargetInfo {
    return this.#targetInfo;
  }

  override browser(): Browser {
    if (!this.#browserContext) {
      throw new Error('browserContext is not initialised');
    }
    return this.#browserContext.browser();
  }

  override browserContext(): BrowserContext {
    if (!this.#browserContext) {
      throw new Error('browserContext is not initialised');
    }
    return this.#browserContext;
  }

  override opener(): Target | undefined {
    const {openerId} = this.#targetInfo;
    if (!openerId) {
      return;
    }
    return this.browser()._targets.get(openerId);
  }

  /**
   * @internal
   */
  _targetInfoChanged(targetInfo: Protocol.Target.TargetInfo): void {
    this.#targetInfo = targetInfo;
    this._checkIfInitialized();
  }

  /**
   * @internal
   */
  _initialize(): void {
    this._initializedDeferred.resolve(InitializationStatus.SUCCESS);
  }

  /**
   * @internal
   */
  protected _checkIfInitialized(): void {
    if (!this._initializedDeferred.resolved()) {
      this._initializedDeferred.resolve(InitializationStatus.SUCCESS);
    }
  }
}

/**
 * @internal
 */
export class PageTarget extends CDPTarget {
  #defaultViewport?: Viewport;
  protected pagePromise?: Promise<Page>;
  #screenshotTaskQueue: TaskQueue;
  #ignoreHTTPSErrors: boolean;

  /**
   * @internal
   */
  constructor(
    targetInfo: Protocol.Target.TargetInfo,
    session: CDPSession | undefined,
    browserContext: BrowserContext,
    targetManager: TargetManager,
    sessionFactory: (isAutoAttachEmulated: boolean) => Promise<CDPSession>,
    ignoreHTTPSErrors: boolean,
    defaultViewport: Viewport | null,
    screenshotTaskQueue: TaskQueue
  ) {
    super(targetInfo, session, browserContext, targetManager, sessionFactory);
    this.#ignoreHTTPSErrors = ignoreHTTPSErrors;
    this.#defaultViewport = defaultViewport ?? undefined;
    this.#screenshotTaskQueue = screenshotTaskQueue;
  }

  override _initialize(): void {
    this._initializedDeferred
      .valueOrThrow()
      .then(async result => {
        if (result === InitializationStatus.ABORTED) {
          return;
        }
        const opener = this.opener();
        if (!(opener instanceof PageTarget)) {
          return;
        }
        if (!opener || !opener.pagePromise || this.type() !== 'page') {
          return true;
        }
        const openerPage = await opener.pagePromise;
        if (!openerPage.listenerCount(PageEmittedEvents.Popup)) {
          return true;
        }
        const popupPage = await this.page();
        openerPage.emit(PageEmittedEvents.Popup, popupPage);
        return true;
      })
      .catch(debugError);
    this._checkIfInitialized();
  }

  override async page(): Promise<Page | null> {
    if (!this.pagePromise) {
      const session = this._session();
      this.pagePromise = (
        session
          ? Promise.resolve(session)
          : this._sessionFactory()(/* isAutoAttachEmulated=*/ false)
      ).then(client => {
        return CDPPage._create(
          client,
          this,
          this.#ignoreHTTPSErrors,
          this.#defaultViewport ?? null,
          this.#screenshotTaskQueue
        );
      });
    }
    return (await this.pagePromise) ?? null;
  }

  override _checkIfInitialized(): void {
    if (this._initializedDeferred.resolved()) {
      return;
    }
    if (this._getTargetInfo().url !== '') {
      this._initializedDeferred.resolve(InitializationStatus.SUCCESS);
    }
  }
}

/**
 * @internal
 */
export class WorkerTarget extends CDPTarget {
  #workerPromise?: Promise<WebWorker>;

  override async worker(): Promise<WebWorker | null> {
    if (!this.#workerPromise) {
      const session = this._session();
      // TODO(einbinder): Make workers send their console logs.
      this.#workerPromise = (
        session
          ? Promise.resolve(session)
          : this._sessionFactory()(/* isAutoAttachEmulated=*/ false)
      ).then(client => {
        return new WebWorker(
          client,
          this._getTargetInfo().url,
          () => {} /* consoleAPICalled */,
          () => {} /* exceptionThrown */
        );
      });
    }
    return this.#workerPromise;
  }
}

/**
 * @internal
 */
export class OtherTarget extends CDPTarget {}
