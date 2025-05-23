/// <reference types="node" />

import { ChildProcess } from 'child_process';
import { Protocol } from 'devtools-protocol';
import { ProtocolMapping } from 'devtools-protocol/types/protocol-mapping.js';
import type { Readable } from 'stream';

/**
 * The Accessibility class provides methods for inspecting the browser's
 * accessibility tree. The accessibility tree is used by assistive technology
 * such as {@link https://en.wikipedia.org/wiki/Screen_reader | screen readers} or
 * {@link https://en.wikipedia.org/wiki/Switch_access | switches}.
 *
 * @remarks
 *
 * Accessibility is a very platform-specific thing. On different platforms,
 * there are different screen readers that might have wildly different output.
 *
 * Blink - Chrome's rendering engine - has a concept of "accessibility tree",
 * which is then translated into different platform-specific APIs. Accessibility
 * namespace gives users access to the Blink Accessibility Tree.
 *
 * Most of the accessibility tree gets filtered out when converting from Blink
 * AX Tree to Platform-specific AX-Tree or by assistive technologies themselves.
 * By default, Puppeteer tries to approximate this filtering, exposing only
 * the "interesting" nodes of the tree.
 *
 * @public
 */
export declare class Accessibility {
    #private;
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: updateClient */
    /**
     * Captures the current state of the accessibility tree.
     * The returned object represents the root accessible node of the page.
     *
     * @remarks
     *
     * **NOTE** The Chrome accessibility tree contains nodes that go unused on
     * most platforms and by most screen readers. Puppeteer will discard them as
     * well for an easier to process tree, unless `interestingOnly` is set to
     * `false`.
     *
     * @example
     * An example of dumping the entire accessibility tree:
     *
     * ```ts
     * const snapshot = await page.accessibility.snapshot();
     * console.log(snapshot);
     * ```
     *
     * @example
     * An example of logging the focused node's name:
     *
     * ```ts
     * const snapshot = await page.accessibility.snapshot();
     * const node = findFocusedNode(snapshot);
     * console.log(node && node.name);
     *
     * function findFocusedNode(node) {
     *   if (node.focused) return node;
     *   for (const child of node.children || []) {
     *     const foundNode = findFocusedNode(child);
     *     return foundNode;
     *   }
     *   return null;
     * }
     * ```
     *
     * @returns An AXNode object representing the snapshot.
     */
    snapshot(options?: SnapshotOptions): Promise<SerializedAXNode | null>;
    private serializeTree;
    private collectInterestingNodes;
}

/* Excluded from this release type: Action */

/**
 * @public
 */
export declare interface ActionOptions {
    signal?: AbortSignal;
}

/**
 * @public
 */
export declare type ActionResult = 'continue' | 'abort' | 'respond';

/* Excluded from this release type: addEventListener_2 */

/* Excluded from this release type: addPageBinding */

/* Excluded from this release type: ARIAQueryHandler */

/* Excluded from this release type: assert */

/* Excluded from this release type: AsyncIterableUtil */

/**
 * @public
 */
export declare interface AutofillData {
    creditCard: {
        number: string;
        name: string;
        expiryMonth: string;
        expiryYear: string;
        cvc: string;
    };
}

/**
 * @public
 */
export declare type Awaitable<T> = T | PromiseLike<T>;

/**
 * @public
 */
export declare type AwaitableIterable<T> = Iterable<T> | AsyncIterable<T>;

/* Excluded from this release type: AwaitableIterator */

/**
 * @public
 */
export declare type AwaitedLocator<T> = T extends Locator<infer S> ? S : never;

declare type BeginSubclassSelectorTokens = ['.', '#', '[', ':'];

/* Excluded from this release type: Binding */

/* Excluded from this release type: BindingPayload */

/**
 * @public
 */
export declare interface BoundingBox extends Point {
    /**
     * the width of the element in pixels.
     */
    width: number;
    /**
     * the height of the element in pixels.
     */
    height: number;
}

/**
 * @public
 */
export declare interface BoxModel {
    content: Quad;
    padding: Quad;
    border: Quad;
    margin: Quad;
    width: number;
    height: number;
}

/**
 * A Browser is created when Puppeteer connects to a browser instance, either through
 * {@link PuppeteerNode.launch} or {@link Puppeteer.connect}.
 *
 * @remarks
 *
 * The Browser class extends from Puppeteer's {@link EventEmitter} class and will
 * emit various events which are documented in the {@link BrowserEmittedEvents} enum.
 *
 * @example
 * An example of using a {@link Browser} to create a {@link Page}:
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.goto('https://example.com');
 *   await browser.close();
 * })();
 * ```
 *
 * @example
 * An example of disconnecting from and reconnecting to a {@link Browser}:
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   // Store the endpoint to be able to reconnect to the browser.
 *   const browserWSEndpoint = browser.wsEndpoint();
 *   // Disconnect puppeteer from the browser.
 *   browser.disconnect();
 *
 *   // Use the endpoint to reestablish a connection
 *   const browser2 = await puppeteer.connect({browserWSEndpoint});
 *   // Close the browser.
 *   await browser2.close();
 * })();
 * ```
 *
 * @public
 */
export declare class Browser extends EventEmitter {
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: _attach */
    /* Excluded from this release type: _detach */
    /* Excluded from this release type: _targets */
    /**
     * The spawned browser process. Returns `null` if the browser instance was created with
     * {@link Puppeteer.connect}.
     */
    process(): ChildProcess | null;
    /* Excluded from this release type: _getIsPageTargetCallback */
    /**
     * Creates a new incognito browser context. This won't share cookies/cache with other
     * browser contexts.
     *
     * @example
     *
     * ```ts
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   // Create a new incognito browser context.
     *   const context = await browser.createIncognitoBrowserContext();
     *   // Create a new page in a pristine context.
     *   const page = await context.newPage();
     *   // Do stuff
     *   await page.goto('https://example.com');
     * })();
     * ```
     */
    createIncognitoBrowserContext(options?: BrowserContextOptions): Promise<BrowserContext>;
    /**
     * Returns an array of all open browser contexts. In a newly created browser, this will
     * return a single instance of {@link BrowserContext}.
     */
    browserContexts(): BrowserContext[];
    /**
     * Returns the default browser context. The default browser context cannot be closed.
     */
    defaultBrowserContext(): BrowserContext;
    /* Excluded from this release type: _disposeContext */
    /**
     * The browser websocket endpoint which can be used as an argument to
     * {@link Puppeteer.connect}.
     *
     * @returns The Browser websocket url.
     *
     * @remarks
     *
     * The format is `ws://${host}:${port}/devtools/browser/<id>`.
     *
     * You can find the `webSocketDebuggerUrl` from `http://${host}:${port}/json/version`.
     * Learn more about the
     * {@link https://chromedevtools.github.io/devtools-protocol | devtools protocol} and
     * the {@link
     * https://chromedevtools.github.io/devtools-protocol/#how-do-i-access-the-browser-target
     * | browser endpoint}.
     */
    wsEndpoint(): string;
    /**
     * Promise which resolves to a new {@link Page} object. The Page is created in
     * a default browser context.
     */
    newPage(): Promise<Page>;
    /* Excluded from this release type: _createPageInContext */
    /**
     * All active targets inside the Browser. In case of multiple browser contexts, returns
     * an array with all the targets in all browser contexts.
     */
    targets(): Target[];
    /**
     * The target associated with the browser.
     */
    target(): Target;
    /**
     * Searches for a target in all browser contexts.
     *
     * @param predicate - A function to be run for every target.
     * @returns The first target found that matches the `predicate` function.
     *
     * @example
     *
     * An example of finding a target for a page opened via `window.open`:
     *
     * ```ts
     * await page.evaluate(() => window.open('https://www.example.com/'));
     * const newWindowTarget = await browser.waitForTarget(
     *   target => target.url() === 'https://www.example.com/'
     * );
     * ```
     */
    waitForTarget(predicate: (x: Target) => boolean | Promise<boolean>, options?: WaitForTargetOptions): Promise<Target>;
    /**
     * An array of all open pages inside the Browser.
     *
     * @remarks
     *
     * In case of multiple browser contexts, returns an array with all the pages in all
     * browser contexts. Non-visible pages, such as `"background_page"`, will not be listed
     * here. You can find them using {@link Target.page}.
     */
    pages(): Promise<Page[]>;
    /**
     * A string representing the browser name and version.
     *
     * @remarks
     *
     * For headless browser, this is similar to `HeadlessChrome/61.0.3153.0`. For
     * non-headless or new-headless, this is similar to `Chrome/61.0.3153.0`. For
     * Firefox, it is similar to `Firefox/116.0a1`.
     *
     * The format of browser.version() might change with future releases of
     * browsers.
     */
    version(): Promise<string>;
    /**
     * The browser's original user agent. Pages can override the browser user agent with
     * {@link Page.setUserAgent}.
     */
    userAgent(): Promise<string>;
    /**
     * Closes the browser and all of its pages (if any were opened). The
     * {@link Browser} object itself is considered to be disposed and cannot be
     * used anymore.
     */
    close(): Promise<void>;
    /**
     * Disconnects Puppeteer from the browser, but leaves the browser process running.
     * After calling `disconnect`, the {@link Browser} object is considered disposed and
     * cannot be used anymore.
     */
    disconnect(): void;
    /**
     * Indicates that the browser is connected.
     */
    isConnected(): boolean;
}

/* Excluded from this release type: BrowserCloseCallback */

/**
 * Generic browser options that can be passed when launching any browser or when
 * connecting to an existing browser instance.
 * @public
 */
export declare interface BrowserConnectOptions {
    /**
     * Whether to ignore HTTPS errors during navigation.
     * @defaultValue `false`
     */
    ignoreHTTPSErrors?: boolean;
    /**
     * Sets the viewport for each page.
     */
    defaultViewport?: Viewport | null;
    /**
     * Slows down Puppeteer operations by the specified amount of milliseconds to
     * aid debugging.
     */
    slowMo?: number;
    /**
     * Callback to decide if Puppeteer should connect to a given target or not.
     */
    targetFilter?: TargetFilterCallback;
    /* Excluded from this release type: _isPageTarget */
    /* Excluded from this release type: protocol */
    /**
     * Timeout setting for individual protocol (CDP) calls.
     *
     * @defaultValue `180_000`
     */
    protocolTimeout?: number;
}

/**
 * BrowserContexts provide a way to operate multiple independent browser
 * sessions. When a browser is launched, it has a single BrowserContext used by
 * default. The method {@link Browser.newPage | Browser.newPage} creates a page
 * in the default browser context.
 *
 * @remarks
 *
 * The Browser class extends from Puppeteer's {@link EventEmitter} class and
 * will emit various events which are documented in the
 * {@link BrowserContextEmittedEvents} enum.
 *
 * If a page opens another page, e.g. with a `window.open` call, the popup will
 * belong to the parent page's browser context.
 *
 * Puppeteer allows creation of "incognito" browser contexts with
 * {@link Browser.createIncognitoBrowserContext | Browser.createIncognitoBrowserContext}
 * method. "Incognito" browser contexts don't write any browsing data to disk.
 *
 * @example
 *
 * ```ts
 * // Create a new incognito browser context
 * const context = await browser.createIncognitoBrowserContext();
 * // Create a new page inside context.
 * const page = await context.newPage();
 * // ... do stuff with page ...
 * await page.goto('https://example.com');
 * // Dispose context once it's no longer needed.
 * await context.close();
 * ```
 *
 * @public
 */
export declare class BrowserContext extends EventEmitter {
    /* Excluded from this release type: __constructor */
    /**
     * An array of all active targets inside the browser context.
     */
    targets(): Target[];
    /**
     * This searches for a target in this specific browser context.
     *
     * @example
     * An example of finding a target for a page opened via `window.open`:
     *
     * ```ts
     * await page.evaluate(() => window.open('https://www.example.com/'));
     * const newWindowTarget = await browserContext.waitForTarget(
     *   target => target.url() === 'https://www.example.com/'
     * );
     * ```
     *
     * @param predicate - A function to be run for every target
     * @param options - An object of options. Accepts a timeout,
     * which is the maximum wait time in milliseconds.
     * Pass `0` to disable the timeout. Defaults to 30 seconds.
     * @returns Promise which resolves to the first target found
     * that matches the `predicate` function.
     */
    waitForTarget(predicate: (x: Target) => boolean | Promise<boolean>, options?: {
        timeout?: number;
    }): Promise<Target>;
    /**
     * An array of all pages inside the browser context.
     *
     * @returns Promise which resolves to an array of all open pages.
     * Non visible pages, such as `"background_page"`, will not be listed here.
     * You can find them using {@link Target.page | the target page}.
     */
    pages(): Promise<Page[]>;
    /**
     * Returns whether BrowserContext is incognito.
     * The default browser context is the only non-incognito browser context.
     *
     * @remarks
     * The default browser context cannot be closed.
     */
    isIncognito(): boolean;
    /**
     * @example
     *
     * ```ts
     * const context = browser.defaultBrowserContext();
     * await context.overridePermissions('https://html5demos.com', [
     *   'geolocation',
     * ]);
     * ```
     *
     * @param origin - The origin to grant permissions to, e.g. "https://example.com".
     * @param permissions - An array of permissions to grant.
     * All permissions that are not listed here will be automatically denied.
     */
    overridePermissions(origin: string, permissions: Permission[]): Promise<void>;
    /**
     * Clears all permission overrides for the browser context.
     *
     * @example
     *
     * ```ts
     * const context = browser.defaultBrowserContext();
     * context.overridePermissions('https://example.com', ['clipboard-read']);
     * // do stuff ..
     * context.clearPermissionOverrides();
     * ```
     */
    clearPermissionOverrides(): Promise<void>;
    /**
     * Creates a new page in the browser context.
     */
    newPage(): Promise<Page>;
    /**
     * The browser this browser context belongs to.
     */
    browser(): Browser;
    /**
     * Closes the browser context. All the targets that belong to the browser context
     * will be closed.
     *
     * @remarks
     * Only incognito browser contexts can be closed.
     */
    close(): Promise<void>;
    get id(): string | undefined;
}

/**
 * @public
 */
export declare const enum BrowserContextEmittedEvents {
    /**
     * Emitted when the url of a target inside the browser context changes.
     * Contains a {@link Target} instance.
     */
    TargetChanged = "targetchanged",
    /**
     * Emitted when a target is created within the browser context, for example
     * when a new page is opened by
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/open | window.open}
     * or by {@link BrowserContext.newPage | browserContext.newPage}
     *
     * Contains a {@link Target} instance.
     */
    TargetCreated = "targetcreated",
    /**
     * Emitted when a target is destroyed within the browser context, for example
     * when a page is closed. Contains a {@link Target} instance.
     */
    TargetDestroyed = "targetdestroyed"
}

/**
 * BrowserContext options.
 *
 * @public
 */
export declare interface BrowserContextOptions {
    /**
     * Proxy server with optional port to use for all requests.
     * Username and password can be set in `Page.authenticate`.
     */
    proxyServer?: string;
    /**
     * Bypass the proxy for the given list of hosts.
     */
    proxyBypassList?: string[];
}

/**
 * All the events a {@link Browser | browser instance} may emit.
 *
 * @public
 */
export declare const enum BrowserEmittedEvents {
    /**
     * Emitted when Puppeteer gets disconnected from the browser instance. This
     * might happen because of one of the following:
     *
     * - browser is closed or crashed
     *
     * - The {@link Browser.disconnect | browser.disconnect } method was called.
     */
    Disconnected = "disconnected",
    /**
     * Emitted when the url of a target changes. Contains a {@link Target} instance.
     *
     * @remarks
     *
     * Note that this includes target changes in incognito browser contexts.
     */
    TargetChanged = "targetchanged",
    /**
     * Emitted when a target is created, for example when a new page is opened by
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/open | window.open}
     * or by {@link Browser.newPage | browser.newPage}
     *
     * Contains a {@link Target} instance.
     *
     * @remarks
     *
     * Note that this includes target creations in incognito browser contexts.
     */
    TargetCreated = "targetcreated",
    /**
     * Emitted when a target is destroyed, for example when a page is closed.
     * Contains a {@link Target} instance.
     *
     * @remarks
     *
     * Note that this includes target destructions in incognito browser contexts.
     */
    TargetDestroyed = "targetdestroyed"
}

/**
 * Launcher options that only apply to Chrome.
 *
 * @public
 */
export declare interface BrowserLaunchArgumentOptions {
    /**
     * Whether to run the browser in headless mode.
     *
     * @remarks
     * In the future `headless: true` will be equivalent to `headless: 'new'`.
     * You can read more about the change {@link https://developer.chrome.com/articles/new-headless/ | here}.
     * Consider opting in early by setting the value to `"new"`.
     *
     * @defaultValue `true`
     */
    headless?: boolean | 'new';
    /**
     * Path to a user data directory.
     * {@link https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/user_data_dir.md | see the Chromium docs}
     * for more info.
     */
    userDataDir?: string;
    /**
     * Whether to auto-open a DevTools panel for each tab. If this is set to
     * `true`, then `headless` will be forced to `false`.
     * @defaultValue `false`
     */
    devtools?: boolean;
    /**
     * Specify the debugging port number to use
     */
    debuggingPort?: number;
    /**
     * Additional command line arguments to pass to the browser instance.
     */
    args?: string[];
}

/* Excluded from this release type: BrowserWebSocketTransport */

/* Excluded from this release type: Callback */

/* Excluded from this release type: CallbackRegistry */

/* Excluded from this release type: CDPBrowser */

/* Excluded from this release type: CDPBrowserContext */

/* Excluded from this release type: CDPDialog */

/* Excluded from this release type: CDPElementHandle */

/* Excluded from this release type: CDPJSHandle */

/* Excluded from this release type: CDPKeyboard */

/* Excluded from this release type: CDPMouse */

/* Excluded from this release type: CDPPage */

/**
 * The `CDPSession` instances are used to talk raw Chrome Devtools Protocol.
 *
 * @remarks
 *
 * Protocol methods can be called with {@link CDPSession.send} method and protocol
 * events can be subscribed to with `CDPSession.on` method.
 *
 * Useful links: {@link https://chromedevtools.github.io/devtools-protocol/ | DevTools Protocol Viewer}
 * and {@link https://github.com/aslushnikov/getting-started-with-cdp/blob/HEAD/README.md | Getting Started with DevTools Protocol}.
 *
 * @example
 *
 * ```ts
 * const client = await page.target().createCDPSession();
 * await client.send('Animation.enable');
 * client.on('Animation.animationCreated', () =>
 *   console.log('Animation created!')
 * );
 * const response = await client.send('Animation.getPlaybackRate');
 * console.log('playback rate is ' + response.playbackRate);
 * await client.send('Animation.setPlaybackRate', {
 *   playbackRate: response.playbackRate / 2,
 * });
 * ```
 *
 * @public
 */
export declare class CDPSession extends EventEmitter {
    /* Excluded from this release type: __constructor */
    connection(): Connection | undefined;
    /* Excluded from this release type: parentSession */
    send<T extends keyof ProtocolMapping.Commands>(method: T, ...paramArgs: ProtocolMapping.Commands[T]['paramsType']): Promise<ProtocolMapping.Commands[T]['returnType']>;
    /**
     * Detaches the cdpSession from the target. Once detached, the cdpSession object
     * won't emit any events and can't be used to send messages.
     */
    detach(): Promise<void>;
    /**
     * Returns the session's id.
     */
    id(): string;
}

/* Excluded from this release type: CDPSessionEmittedEvents */

/* Excluded from this release type: CDPSessionImpl */

/* Excluded from this release type: CDPSessionOnMessageObject */

/* Excluded from this release type: CDPTarget */

/* Excluded from this release type: CDPTouchscreen */

/* Excluded from this release type: ChromeLauncher */

/**
 * @public
 */
export declare type ChromeReleaseChannel = 'chrome' | 'chrome-beta' | 'chrome-canary' | 'chrome-dev';

/* Excluded from this release type: ChromeTargetManager */

/**
 * @deprecated Import {@link Puppeteer} and use the static method
 * {@link Puppeteer.clearCustomQueryHandlers}
 *
 * @public
 */
export declare function clearCustomQueryHandlers(): void;

/**
 * @public
 */
export declare interface ClickOptions extends MouseClickOptions {
    /**
     * Offset for the clickable point relative to the top-left corner of the border box.
     */
    offset?: Offset;
}

declare type CombinatorTokens = [' ', '>', '+', '~', '|', '|'];

/**
 * @public
 */
export declare interface CommonEventEmitter {
    on(event: EventType, handler: Handler): this;
    off(event: EventType, handler: Handler): this;
    addListener(event: EventType, handler: Handler): this;
    removeListener(event: EventType, handler: Handler): this;
    emit(event: EventType, eventData?: unknown): boolean;
    once(event: EventType, handler: Handler): this;
    listenerCount(event: string): number;
    removeAllListeners(event?: EventType): this;
}

/* Excluded from this release type: CommonPuppeteerSettings */

declare type CompoundSelectorsOfComplexSelector<ComplexSelector extends string> = SplitWithDelemiters<ComplexSelector, CombinatorTokens> extends infer IntermediateTokens ? IntermediateTokens extends readonly string[] ? Drop<IntermediateTokens, ''> : never : never;

/**
 * Defines options to configure Puppeteer's behavior during installation and
 * runtime.
 *
 * See individual properties for more information.
 *
 * @public
 */
export declare interface Configuration {
    /**
     * Specifies a certain version of the browser you'd like Puppeteer to use.
     *
     * Can be overridden by `PUPPETEER_BROWSER_REVISION`.
     *
     * See {@link PuppeteerNode.launch | puppeteer.launch} on how executable path
     * is inferred.
     *
     * @defaultValue A compatible-revision of the browser.
     */
    browserRevision?: string;
    /**
     * Defines the directory to be used by Puppeteer for caching.
     *
     * Can be overridden by `PUPPETEER_CACHE_DIR`.
     *
     * @defaultValue `path.join(os.homedir(), '.cache', 'puppeteer')`
     */
    cacheDirectory?: string;
    /**
     * Specifies the URL prefix that is used to download the browser.
     *
     * Can be overridden by `PUPPETEER_DOWNLOAD_BASE_URL`.
     *
     * @remarks
     * This must include the protocol and may even need a path prefix.
     *
     * @defaultValue Either https://edgedl.me.gvt1.com/edgedl/chrome/chrome-for-testing or
     * https://archive.mozilla.org/pub/firefox/nightly/latest-mozilla-central,
     * depending on the product.
     */
    downloadBaseUrl?: string;
    /**
     * Specifies the path for the downloads folder.
     *
     * Can be overridden by `PUPPETEER_DOWNLOAD_PATH`.
     *
     * @defaultValue `<cacheDirectory>`
     */
    downloadPath?: string;
    /**
     * Specifies an executable path to be used in
     * {@link PuppeteerNode.launch | puppeteer.launch}.
     *
     * Can be overridden by `PUPPETEER_EXECUTABLE_PATH`.
     *
     * @defaultValue **Auto-computed.**
     */
    executablePath?: string;
    /**
     * Specifies which browser you'd like Puppeteer to use.
     *
     * Can be overridden by `PUPPETEER_PRODUCT`.
     *
     * @defaultValue `chrome`
     */
    defaultProduct?: Product;
    /**
     * Defines the directory to be used by Puppeteer for creating temporary files.
     *
     * Can be overridden by `PUPPETEER_TMP_DIR`.
     *
     * @defaultValue `os.tmpdir()`
     */
    temporaryDirectory?: string;
    /**
     * Tells Puppeteer to not download during installation.
     *
     * Can be overridden by `PUPPETEER_SKIP_DOWNLOAD`.
     */
    skipDownload?: boolean;
    /**
     * Tells Puppeteer to log at the given level.
     *
     * @defaultValue `warn`
     */
    logLevel?: 'silent' | 'error' | 'warn';
    /**
     * Defines experimental options for Puppeteer.
     */
    experiments?: ExperimentsConfiguration;
}

/**
 * @public
 */
export declare const 
/**
* @public
*/
/**
 * @public
 */
connect: (options: ConnectOptions) => Promise<Browser>;

/**
 * @public
 */
export declare class Connection extends EventEmitter {
    #private;
    constructor(url: string, transport: ConnectionTransport, delay?: number, timeout?: number);
    static fromSession(session: CDPSession): Connection | undefined;
    get timeout(): number;
    /* Excluded from this release type: _closed */
    /* Excluded from this release type: _sessions */
    /**
     * @param sessionId - The session id
     * @returns The current CDP session if it exists
     */
    session(sessionId: string): CDPSession | null;
    url(): string;
    send<T extends keyof ProtocolMapping.Commands>(method: T, ...paramArgs: ProtocolMapping.Commands[T]['paramsType']): Promise<ProtocolMapping.Commands[T]['returnType']>;
    /* Excluded from this release type: _rawSend */
    /* Excluded from this release type: closeBrowser */
    /* Excluded from this release type: onMessage */
    dispose(): void;
    /* Excluded from this release type: isAutoAttached */
    /* Excluded from this release type: _createSession */
    /**
     * @param targetInfo - The target info
     * @returns The CDP session that is created
     */
    createSession(targetInfo: Protocol.Target.TargetInfo): Promise<CDPSession>;
}

/* Excluded from this release type: ConnectionEmittedEvents */

/**
 * Copyright 2020 Google Inc. All rights reserved.
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
/**
 * @public
 */
export declare interface ConnectionTransport {
    send(message: string): void;
    close(): void;
    onmessage?: (message: string) => void;
    onclose?: () => void;
}

/**
 * @public
 */
export declare interface ConnectOptions extends BrowserConnectOptions {
    browserWSEndpoint?: string;
    browserURL?: string;
    transport?: ConnectionTransport;
    /**
     * Headers to use for the web socket connection.
     * @remarks
     * Only works in the Node.js environment.
     */
    headers?: Record<string, string>;
}

/* Excluded from this release type: _connectToCDPBrowser */

/* Excluded from this release type: ConsoleAPICalledCallback */

/**
 * ConsoleMessage objects are dispatched by page via the 'console' event.
 * @public
 */
export declare class ConsoleMessage {
    #private;
    /**
     * @public
     */
    constructor(type: ConsoleMessageType, text: string, args: JSHandle[], stackTraceLocations: ConsoleMessageLocation[]);
    /**
     * The type of the console message.
     */
    type(): ConsoleMessageType;
    /**
     * The text of the console message.
     */
    text(): string;
    /**
     * An array of arguments passed to the console.
     */
    args(): JSHandle[];
    /**
     * The location of the console message.
     */
    location(): ConsoleMessageLocation;
    /**
     * The array of locations on the stack of the console message.
     */
    stackTrace(): ConsoleMessageLocation[];
}

/**
 * @public
 */
export declare interface ConsoleMessageLocation {
    /**
     * URL of the resource if known or `undefined` otherwise.
     */
    url?: string;
    /**
     * 0-based line number in the resource if known or `undefined` otherwise.
     */
    lineNumber?: number;
    /**
     * 0-based column number in the resource if known or `undefined` otherwise.
     */
    columnNumber?: number;
}

/**
 * The supported types for console messages.
 * @public
 */
export declare type ConsoleMessageType = 'log' | 'debug' | 'info' | 'error' | 'warning' | 'dir' | 'dirxml' | 'table' | 'trace' | 'clear' | 'startGroup' | 'startGroupCollapsed' | 'endGroup' | 'assert' | 'profile' | 'profileEnd' | 'count' | 'timeEnd' | 'verbose';

/**
 * @public
 */
export declare interface ContinueRequestOverrides {
    /**
     * If set, the request URL will change. This is not a redirect.
     */
    url?: string;
    method?: string;
    postData?: string;
    headers?: Record<string, string>;
}

/**
 * The Coverage class provides methods to gather information about parts of
 * JavaScript and CSS that were used by the page.
 *
 * @remarks
 * To output coverage in a form consumable by {@link https://github.com/istanbuljs | Istanbul},
 * see {@link https://github.com/istanbuljs/puppeteer-to-istanbul | puppeteer-to-istanbul}.
 *
 * @example
 * An example of using JavaScript and CSS coverage to get percentage of initially
 * executed code:
 *
 * ```ts
 * // Enable both JavaScript and CSS coverage
 * await Promise.all([
 *   page.coverage.startJSCoverage(),
 *   page.coverage.startCSSCoverage(),
 * ]);
 * // Navigate to page
 * await page.goto('https://example.com');
 * // Disable both JavaScript and CSS coverage
 * const [jsCoverage, cssCoverage] = await Promise.all([
 *   page.coverage.stopJSCoverage(),
 *   page.coverage.stopCSSCoverage(),
 * ]);
 * let totalBytes = 0;
 * let usedBytes = 0;
 * const coverage = [...jsCoverage, ...cssCoverage];
 * for (const entry of coverage) {
 *   totalBytes += entry.text.length;
 *   for (const range of entry.ranges) usedBytes += range.end - range.start - 1;
 * }
 * console.log(`Bytes used: ${(usedBytes / totalBytes) * 100}%`);
 * ```
 *
 * @public
 */
export declare class Coverage {
    #private;
    constructor(client: CDPSession);
    /* Excluded from this release type: updateClient */
    /**
     * @param options - Set of configurable options for coverage defaults to
     * `resetOnNavigation : true, reportAnonymousScripts : false,`
     * `includeRawScriptCoverage : false, useBlockCoverage : true`
     * @returns Promise that resolves when coverage is started.
     *
     * @remarks
     * Anonymous scripts are ones that don't have an associated url. These are
     * scripts that are dynamically created on the page using `eval` or
     * `new Function`. If `reportAnonymousScripts` is set to `true`, anonymous
     * scripts URL will start with `debugger://VM` (unless a magic //# sourceURL
     * comment is present, in which case that will the be URL).
     */
    startJSCoverage(options?: JSCoverageOptions): Promise<void>;
    /**
     * Promise that resolves to the array of coverage reports for
     * all scripts.
     *
     * @remarks
     * JavaScript Coverage doesn't include anonymous scripts by default.
     * However, scripts with sourceURLs are reported.
     */
    stopJSCoverage(): Promise<JSCoverageEntry[]>;
    /**
     * @param options - Set of configurable options for coverage, defaults to
     * `resetOnNavigation : true`
     * @returns Promise that resolves when coverage is started.
     */
    startCSSCoverage(options?: CSSCoverageOptions): Promise<void>;
    /**
     * Promise that resolves to the array of coverage reports
     * for all stylesheets.
     *
     * @remarks
     * CSS Coverage doesn't include dynamically injected style tags
     * without sourceURLs.
     */
    stopCSSCoverage(): Promise<CoverageEntry[]>;
}

/**
 * The CoverageEntry class represents one entry of the coverage report.
 * @public
 */
export declare interface CoverageEntry {
    /**
     * The URL of the style sheet or script.
     */
    url: string;
    /**
     * The content of the style sheet or script.
     */
    text: string;
    /**
     * The covered range as start and end positions.
     */
    ranges: Array<{
        start: number;
        end: number;
    }>;
}

/* Excluded from this release type: createClientError */

/* Excluded from this release type: createDebuggableDeferred */

/* Excluded from this release type: createEvaluationError */

/* Excluded from this release type: createJSHandle */

/**
 * @public
 */
export declare interface Credentials {
    username: string;
    password: string;
}

/**
 * @public
 */
export declare class CSSCoverage {
    #private;
    constructor(client: CDPSession);
    /* Excluded from this release type: updateClient */
    start(options?: {
        resetOnNavigation?: boolean;
    }): Promise<void>;
    stop(): Promise<CoverageEntry[]>;
}

/**
 * Set of configurable options for CSS coverage.
 * @public
 */
export declare interface CSSCoverageOptions {
    /**
     * Whether to reset coverage on every navigation.
     */
    resetOnNavigation?: boolean;
}

/**
 * Copyright 2018 Google Inc. All rights reserved.
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
/**
 * @deprecated Do not use.
 *
 * @public
 */
export declare class CustomError extends Error {
    /* Excluded from this release type: __constructor */
}

/**
 * @public
 */
export declare interface CustomQueryHandler {
    /**
     * Searches for a {@link https://developer.mozilla.org/en-US/docs/Web/API/Node | Node} matching the given `selector` from {@link https://developer.mozilla.org/en-US/docs/Web/API/Node | node}.
     */
    queryOne?: (node: Node, selector: string) => Node | null;
    /**
     * Searches for some {@link https://developer.mozilla.org/en-US/docs/Web/API/Node | Nodes} matching the given `selector` from {@link https://developer.mozilla.org/en-US/docs/Web/API/Node | node}.
     */
    queryAll?: (node: Node, selector: string) => Iterable<Node>;
}

/**
 * @deprecated Import {@link Puppeteer} and use the static method
 * {@link Puppeteer.customQueryHandlerNames}
 *
 * @public
 */
export declare function customQueryHandlerNames(): string[];

/* Excluded from this release type: CustomQueryHandlerRegistry */

/* Excluded from this release type: customQueryHandlers */

declare interface CustomQuerySelector {
    querySelector(root: Node, selector: string): Awaitable<Node | null>;
    querySelectorAll(root: Node, selector: string): AwaitableIterable<Node>;
}

/**
 * This class mimics the injected {@link CustomQuerySelectorRegistry}.
 */
declare class CustomQuerySelectorRegistry {
    #private;
    register(name: string, handler: CustomQueryHandler): void;
    unregister(name: string): void;
    get(name: string): CustomQuerySelector | undefined;
    clear(): void;
}

declare namespace CustomQuerySelectors {
    export {
        CustomQuerySelector,
        customQuerySelectors
    }
}

declare const customQuerySelectors: CustomQuerySelectorRegistry;

/* Excluded from this release type: debug_2 */

/* Excluded from this release type: debugError */

/**
 * The default cooperative request interception resolution priority
 *
 * @public
 */
export declare const DEFAULT_INTERCEPT_RESOLUTION_PRIORITY = 0;

/**
 * @public
 */
export declare const 
/**
* @public
*/
/**
 * @public
 */
defaultArgs: (options?: BrowserLaunchArgumentOptions) => string[];

/* Excluded from this release type: Deferred */

/* Excluded from this release type: DeferredOptions */

/* Excluded from this release type: DelegatedLocator */

/**
 * @public
 */
export declare interface Device {
    userAgent: string;
    viewport: Viewport;
}

/**
 * Device request prompts let you respond to the page requesting for a device
 * through an API like WebBluetooth.
 *
 * @remarks
 * `DeviceRequestPrompt` instances are returned via the
 * {@link Page.waitForDevicePrompt} method.
 *
 * @example
 *
 * ```ts
 * const [deviceRequest] = Promise.all([
 *   page.waitForDevicePrompt(),
 *   page.click('#connect-bluetooth'),
 * ]);
 * await devicePrompt.select(
 *   await devicePrompt.waitForDevice(({name}) => name.includes('My Device'))
 * );
 * ```
 *
 * @public
 */
export declare class DeviceRequestPrompt {
    #private;
    /**
     * Current list of selectable devices.
     */
    devices: DeviceRequestPromptDevice[];
    /* Excluded from this release type: __constructor */
    /**
     * Resolve to the first device in the prompt matching a filter.
     */
    waitForDevice(filter: (device: DeviceRequestPromptDevice) => boolean, options?: WaitTimeoutOptions): Promise<DeviceRequestPromptDevice>;
    /**
     * Select a device in the prompt's list.
     */
    select(device: DeviceRequestPromptDevice): Promise<void>;
    /**
     * Cancel the prompt.
     */
    cancel(): Promise<void>;
}

/**
 * Device in a request prompt.
 *
 * @public
 */
export declare class DeviceRequestPromptDevice {
    /**
     * Device id during a prompt.
     */
    id: string;
    /**
     * Device name as it appears in a prompt.
     */
    name: string;
    /* Excluded from this release type: __constructor */
}

/* Excluded from this release type: DeviceRequestPromptManager */

/**
 * @deprecated Import {@link KnownDevices}
 *
 * @public
 */
export declare const devices: Readonly<Record<"Blackberry PlayBook" | "Blackberry PlayBook landscape" | "BlackBerry Z30" | "BlackBerry Z30 landscape" | "Galaxy Note 3" | "Galaxy Note 3 landscape" | "Galaxy Note II" | "Galaxy Note II landscape" | "Galaxy S III" | "Galaxy S III landscape" | "Galaxy S5" | "Galaxy S5 landscape" | "Galaxy S8" | "Galaxy S8 landscape" | "Galaxy S9+" | "Galaxy S9+ landscape" | "Galaxy Tab S4" | "Galaxy Tab S4 landscape" | "iPad" | "iPad landscape" | "iPad (gen 6)" | "iPad (gen 6) landscape" | "iPad (gen 7)" | "iPad (gen 7) landscape" | "iPad Mini" | "iPad Mini landscape" | "iPad Pro" | "iPad Pro landscape" | "iPad Pro 11" | "iPad Pro 11 landscape" | "iPhone 4" | "iPhone 4 landscape" | "iPhone 5" | "iPhone 5 landscape" | "iPhone 6" | "iPhone 6 landscape" | "iPhone 6 Plus" | "iPhone 6 Plus landscape" | "iPhone 7" | "iPhone 7 landscape" | "iPhone 7 Plus" | "iPhone 7 Plus landscape" | "iPhone 8" | "iPhone 8 landscape" | "iPhone 8 Plus" | "iPhone 8 Plus landscape" | "iPhone SE" | "iPhone SE landscape" | "iPhone X" | "iPhone X landscape" | "iPhone XR" | "iPhone XR landscape" | "iPhone 11" | "iPhone 11 landscape" | "iPhone 11 Pro" | "iPhone 11 Pro landscape" | "iPhone 11 Pro Max" | "iPhone 11 Pro Max landscape" | "iPhone 12" | "iPhone 12 landscape" | "iPhone 12 Pro" | "iPhone 12 Pro landscape" | "iPhone 12 Pro Max" | "iPhone 12 Pro Max landscape" | "iPhone 12 Mini" | "iPhone 12 Mini landscape" | "iPhone 13" | "iPhone 13 landscape" | "iPhone 13 Pro" | "iPhone 13 Pro landscape" | "iPhone 13 Pro Max" | "iPhone 13 Pro Max landscape" | "iPhone 13 Mini" | "iPhone 13 Mini landscape" | "JioPhone 2" | "JioPhone 2 landscape" | "Kindle Fire HDX" | "Kindle Fire HDX landscape" | "LG Optimus L70" | "LG Optimus L70 landscape" | "Microsoft Lumia 550" | "Microsoft Lumia 950" | "Microsoft Lumia 950 landscape" | "Nexus 10" | "Nexus 10 landscape" | "Nexus 4" | "Nexus 4 landscape" | "Nexus 5" | "Nexus 5 landscape" | "Nexus 5X" | "Nexus 5X landscape" | "Nexus 6" | "Nexus 6 landscape" | "Nexus 6P" | "Nexus 6P landscape" | "Nexus 7" | "Nexus 7 landscape" | "Nokia Lumia 520" | "Nokia Lumia 520 landscape" | "Nokia N9" | "Nokia N9 landscape" | "Pixel 2" | "Pixel 2 landscape" | "Pixel 2 XL" | "Pixel 2 XL landscape" | "Pixel 3" | "Pixel 3 landscape" | "Pixel 4" | "Pixel 4 landscape" | "Pixel 4a (5G)" | "Pixel 4a (5G) landscape" | "Pixel 5" | "Pixel 5 landscape" | "Moto G4" | "Moto G4 landscape", Device>>;

/**
 * Dialog instances are dispatched by the {@link Page} via the `dialog` event.
 *
 * @remarks
 *
 * @example
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   page.on('dialog', async dialog => {
 *     console.log(dialog.message());
 *     await dialog.dismiss();
 *     await browser.close();
 *   });
 *   page.evaluate(() => alert('1'));
 * })();
 * ```
 *
 * @public
 */
export declare class Dialog {
    #private;
    /* Excluded from this release type: __constructor */
    /**
     * The type of the dialog.
     */
    type(): Protocol.Page.DialogType;
    /**
     * The message displayed in the dialog.
     */
    message(): string;
    /**
     * The default value of the prompt, or an empty string if the dialog
     * is not a `prompt`.
     */
    defaultValue(): string;
    /* Excluded from this release type: sendCommand */
    /**
     * A promise that resolves when the dialog has been accepted.
     *
     * @param promptText - optional text that will be entered in the dialog
     * prompt. Has no effect if the dialog's type is not `prompt`.
     *
     */
    accept(promptText?: string): Promise<void>;
    /**
     * A promise which will resolve once the dialog has been dismissed
     */
    dismiss(): Promise<void>;
}

declare type Drop<Arr extends readonly unknown[], Remove, Acc extends unknown[] = []> = Arr extends [infer Head, ...infer Tail] ? Head extends Remove ? Drop<Tail, Remove> : Drop<Tail, Remove, [...Acc, Head]> : Acc;

/**
 * @public
 */
export declare type ElementFor<TagName extends keyof HTMLElementTagNameMap | keyof SVGElementTagNameMap> = TagName extends keyof HTMLElementTagNameMap ? HTMLElementTagNameMap[TagName] : TagName extends keyof SVGElementTagNameMap ? SVGElementTagNameMap[TagName] : never;

/**
 * ElementHandle represents an in-page DOM element.
 *
 * @remarks
 * ElementHandles can be created with the {@link Page.$} method.
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.goto('https://example.com');
 *   const hrefElement = await page.$('a');
 *   await hrefElement.click();
 *   // ...
 * })();
 * ```
 *
 * ElementHandle prevents the DOM element from being garbage-collected unless the
 * handle is {@link JSHandle.dispose | disposed}. ElementHandles are auto-disposed
 * when their origin frame gets navigated.
 *
 * ElementHandle instances can be used as arguments in {@link Page.$eval} and
 * {@link Page.evaluate} methods.
 *
 * If you're using TypeScript, ElementHandle takes a generic argument that
 * denotes the type of element the handle is holding within. For example, if you
 * have a handle to a `<select>` element, you can type it as
 * `ElementHandle<HTMLSelectElement>` and you get some nicer type checks.
 *
 * @public
 */
export declare abstract class ElementHandle<ElementType extends Node = Element> extends JSHandle<ElementType> {
    #private;
    /* Excluded from this release type: handle */
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: id */
    /* Excluded from this release type: disposed */
    /* Excluded from this release type: getProperty */
    /* Excluded from this release type: getProperty */
    /* Excluded from this release type: getProperties */
    /* Excluded from this release type: evaluate */
    /* Excluded from this release type: evaluateHandle */
    /* Excluded from this release type: jsonValue */
    /* Excluded from this release type: toString */
    /* Excluded from this release type: remoteObject */
    /* Excluded from this release type: dispose */
    asElement(): ElementHandle<ElementType>;
    /**
     * Frame corresponding to the current handle.
     */
    abstract get frame(): Frame;
    /**
     * Queries the current element for an element matching the given selector.
     *
     * @param selector - The selector to query for.
     * @returns A {@link ElementHandle | element handle} to the first element
     * matching the given selector. Otherwise, `null`.
     */
    $<Selector extends string>(selector: Selector): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * Queries the current element for all elements matching the given selector.
     *
     * @param selector - The selector to query for.
     * @returns An array of {@link ElementHandle | element handles} that point to
     * elements matching the given selector.
     */
    $$<Selector extends string>(selector: Selector): Promise<Array<ElementHandle<NodeFor<Selector>>>>;
    /**
     * Runs the given function on the first element matching the given selector in
     * the current element.
     *
     * If the given function returns a promise, then this method will wait till
     * the promise resolves.
     *
     * @example
     *
     * ```ts
     * const tweetHandle = await page.$('.tweet');
     * expect(await tweetHandle.$eval('.like', node => node.innerText)).toBe(
     *   '100'
     * );
     * expect(await tweetHandle.$eval('.retweets', node => node.innerText)).toBe(
     *   '10'
     * );
     * ```
     *
     * @param selector - The selector to query for.
     * @param pageFunction - The function to be evaluated in this element's page's
     * context. The first element matching the selector will be passed in as the
     * first argument.
     * @param args - Additional arguments to pass to `pageFunction`.
     * @returns A promise to the result of the function.
     */
    $eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<NodeFor<Selector>, Params> = EvaluateFuncWith<NodeFor<Selector>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * Runs the given function on an array of elements matching the given selector
     * in the current element.
     *
     * If the given function returns a promise, then this method will wait till
     * the promise resolves.
     *
     * @example
     * HTML:
     *
     * ```html
     * <div class="feed">
     *   <div class="tweet">Hello!</div>
     *   <div class="tweet">Hi!</div>
     * </div>
     * ```
     *
     * JavaScript:
     *
     * ```js
     * const feedHandle = await page.$('.feed');
     * expect(
     *   await feedHandle.$$eval('.tweet', nodes => nodes.map(n => n.innerText))
     * ).toEqual(['Hello!', 'Hi!']);
     * ```
     *
     * @param selector - The selector to query for.
     * @param pageFunction - The function to be evaluated in the element's page's
     * context. An array of elements matching the given selector will be passed to
     * the function as its first argument.
     * @param args - Additional arguments to pass to `pageFunction`.
     * @returns A promise to the result of the function.
     */
    $$eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<Array<NodeFor<Selector>>, Params> = EvaluateFuncWith<Array<NodeFor<Selector>>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * @deprecated Use {@link ElementHandle.$$} with the `xpath` prefix.
     *
     * Example: `await elementHandle.$$('xpath/' + xpathExpression)`
     *
     * The method evaluates the XPath expression relative to the elementHandle.
     * If `xpath` starts with `//` instead of `.//`, the dot will be appended
     * automatically.
     *
     * If there are no such elements, the method will resolve to an empty array.
     * @param expression - Expression to {@link https://developer.mozilla.org/en-US/docs/Web/API/Document/evaluate | evaluate}
     */
    $x(expression: string): Promise<Array<ElementHandle<Node>>>;
    /**
     * Wait for an element matching the given selector to appear in the current
     * element.
     *
     * Unlike {@link Frame.waitForSelector}, this method does not work across
     * navigations or if the element is detached from DOM.
     *
     * @example
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   let currentURL;
     *   page
     *     .mainFrame()
     *     .waitForSelector('img')
     *     .then(() => console.log('First URL with image: ' + currentURL));
     *
     *   for (currentURL of [
     *     'https://example.com',
     *     'https://google.com',
     *     'https://bbc.com',
     *   ]) {
     *     await page.goto(currentURL);
     *   }
     *   await browser.close();
     * })();
     * ```
     *
     * @param selector - The selector to query and wait for.
     * @param options - Options for customizing waiting behavior.
     * @returns An element matching the given selector.
     * @throws Throws if an element matching the given selector doesn't appear.
     */
    waitForSelector<Selector extends string>(selector: Selector, options?: WaitForSelectorOptions): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * Checks if an element is visible using the same mechanism as
     * {@link ElementHandle.waitForSelector}.
     */
    isVisible(): Promise<boolean>;
    /**
     * Checks if an element is hidden using the same mechanism as
     * {@link ElementHandle.waitForSelector}.
     */
    isHidden(): Promise<boolean>;
    /**
     * @deprecated Use {@link ElementHandle.waitForSelector} with the `xpath`
     * prefix.
     *
     * Example: `await elementHandle.waitForSelector('xpath/' + xpathExpression)`
     *
     * The method evaluates the XPath expression relative to the elementHandle.
     *
     * Wait for the `xpath` within the element. If at the moment of calling the
     * method the `xpath` already exists, the method will return immediately. If
     * the `xpath` doesn't appear after the `timeout` milliseconds of waiting, the
     * function will throw.
     *
     * If `xpath` starts with `//` instead of `.//`, the dot will be appended
     * automatically.
     *
     * @example
     * This method works across navigation.
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   let currentURL;
     *   page
     *     .waitForXPath('//img')
     *     .then(() => console.log('First URL with image: ' + currentURL));
     *   for (currentURL of [
     *     'https://example.com',
     *     'https://google.com',
     *     'https://bbc.com',
     *   ]) {
     *     await page.goto(currentURL);
     *   }
     *   await browser.close();
     * })();
     * ```
     *
     * @param xpath - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/XPath | xpath} of an
     * element to wait for
     * @param options - Optional waiting parameters
     * @returns Promise which resolves when element specified by xpath string is
     * added to DOM. Resolves to `null` if waiting for `hidden: true` and xpath is
     * not found in DOM, otherwise resolves to `ElementHandle`.
     * @remarks
     * The optional Argument `options` have properties:
     *
     * - `visible`: A boolean to wait for element to be present in DOM and to be
     *   visible, i.e. to not have `display: none` or `visibility: hidden` CSS
     *   properties. Defaults to `false`.
     *
     * - `hidden`: A boolean wait for element to not be found in the DOM or to be
     *   hidden, i.e. have `display: none` or `visibility: hidden` CSS properties.
     *   Defaults to `false`.
     *
     * - `timeout`: A number which is maximum time to wait for in milliseconds.
     *   Defaults to `30000` (30 seconds). Pass `0` to disable timeout. The
     *   default value can be changed by using the {@link Page.setDefaultTimeout}
     *   method.
     */
    waitForXPath(xpath: string, options?: {
        visible?: boolean;
        hidden?: boolean;
        timeout?: number;
    }): Promise<ElementHandle<Node> | null>;
    /**
     * Converts the current handle to the given element type.
     *
     * @example
     *
     * ```ts
     * const element: ElementHandle<Element> = await page.$(
     *   '.class-name-of-anchor'
     * );
     * // DO NOT DISPOSE `element`, this will be always be the same handle.
     * const anchor: ElementHandle<HTMLAnchorElement> =
     *   await element.toElement('a');
     * ```
     *
     * @param tagName - The tag name of the desired element type.
     * @throws An error if the handle does not match. **The handle will not be
     * automatically disposed.**
     */
    toElement<K extends keyof HTMLElementTagNameMap | keyof SVGElementTagNameMap>(tagName: K): Promise<HandleFor<ElementFor<K>>>;
    /**
     * Resolves the frame associated with the element, if any. Always exists for
     * HTMLIFrameElements.
     */
    abstract contentFrame(this: ElementHandle<HTMLIFrameElement>): Promise<Frame>;
    abstract contentFrame(): Promise<Frame | null>;
    /**
     * Returns the middle point within an element unless a specific offset is provided.
     */
    clickablePoint(offset?: Offset): Promise<Point>;
    /**
     * This method scrolls element into view if needed, and then
     * uses {@link Page} to hover over the center of the element.
     * If the element is detached from DOM, the method throws an error.
     */
    hover(this: ElementHandle<Element>): Promise<void>;
    /**
     * This method scrolls element into view if needed, and then
     * uses {@link Page | Page.mouse} to click in the center of the element.
     * If the element is detached from DOM, the method throws an error.
     */
    click(this: ElementHandle<Element>, options?: Readonly<ClickOptions>): Promise<void>;
    /**
     * This method creates and captures a dragevent from the element.
     */
    drag(this: ElementHandle<Element>, target: Point): Promise<Protocol.Input.DragData>;
    /**
     * This method creates a `dragenter` event on the element.
     */
    dragEnter(this: ElementHandle<Element>, data?: Protocol.Input.DragData): Promise<void>;
    /**
     * This method creates a `dragover` event on the element.
     */
    dragOver(this: ElementHandle<Element>, data?: Protocol.Input.DragData): Promise<void>;
    /**
     * This method triggers a drop on the element.
     */
    drop(this: ElementHandle<Element>, data?: Protocol.Input.DragData): Promise<void>;
    /**
     * This method triggers a dragenter, dragover, and drop on the element.
     */
    dragAndDrop(this: ElementHandle<Element>, target: ElementHandle<Node>, options?: {
        delay: number;
    }): Promise<void>;
    /**
     * Triggers a `change` and `input` event once all the provided options have been
     * selected. If there's no `<select>` element matching `selector`, the method
     * throws an error.
     *
     * @example
     *
     * ```ts
     * handle.select('blue'); // single selection
     * handle.select('red', 'green', 'blue'); // multiple selections
     * ```
     *
     * @param values - Values of options to select. If the `<select>` has the
     * `multiple` attribute, all values are considered, otherwise only the first
     * one is taken into account.
     */
    select(...values: string[]): Promise<string[]>;
    /**
     * Sets the value of an
     * {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input | input element}
     * to the given file paths.
     *
     * @remarks This will not validate whether the file paths exists. Also, if a
     * path is relative, then it is resolved against the
     * {@link https://nodejs.org/api/process.html#process_process_cwd | current working directory}.
     * For locals script connecting to remote chrome environments, paths must be
     * absolute.
     */
    uploadFile(this: ElementHandle<HTMLInputElement>, ...paths: string[]): Promise<void>;
    /**
     * This method scrolls element into view if needed, and then uses
     * {@link Touchscreen.tap} to tap in the center of the element.
     * If the element is detached from DOM, the method throws an error.
     */
    tap(this: ElementHandle<Element>): Promise<void>;
    touchStart(this: ElementHandle<Element>): Promise<void>;
    touchMove(this: ElementHandle<Element>): Promise<void>;
    touchEnd(this: ElementHandle<Element>): Promise<void>;
    /**
     * Calls {@link https://developer.mozilla.org/en-US/docs/Web/API/HTMLElement/focus | focus} on the element.
     */
    focus(): Promise<void>;
    /**
     * Focuses the element, and then sends a `keydown`, `keypress`/`input`, and
     * `keyup` event for each character in the text.
     *
     * To press a special key, like `Control` or `ArrowDown`,
     * use {@link ElementHandle.press}.
     *
     * @example
     *
     * ```ts
     * await elementHandle.type('Hello'); // Types instantly
     * await elementHandle.type('World', {delay: 100}); // Types slower, like a user
     * ```
     *
     * @example
     * An example of typing into a text field and then submitting the form:
     *
     * ```ts
     * const elementHandle = await page.$('input');
     * await elementHandle.type('some text');
     * await elementHandle.press('Enter');
     * ```
     *
     * @param options - Delay in milliseconds. Defaults to 0.
     */
    type(text: string, options?: Readonly<KeyboardTypeOptions>): Promise<void>;
    /**
     * Focuses the element, and then uses {@link Keyboard.down} and {@link Keyboard.up}.
     *
     * @remarks
     * If `key` is a single character and no modifier keys besides `Shift`
     * are being held down, a `keypress`/`input` event will also be generated.
     * The `text` option can be specified to force an input event to be generated.
     *
     * **NOTE** Modifier keys DO affect `elementHandle.press`. Holding down `Shift`
     * will type the text in upper case.
     *
     * @param key - Name of key to press, such as `ArrowLeft`.
     * See {@link KeyInput} for a list of all key names.
     */
    press(key: KeyInput, options?: Readonly<KeyPressOptions>): Promise<void>;
    /**
     * This method returns the bounding box of the element (relative to the main frame),
     * or `null` if the element is not visible.
     */
    boundingBox(): Promise<BoundingBox | null>;
    /**
     * This method returns boxes of the element, or `null` if the element is not visible.
     *
     * @remarks
     *
     * Boxes are represented as an array of points;
     * Each Point is an object `{x, y}`. Box points are sorted clock-wise.
     */
    boxModel(): Promise<BoxModel | null>;
    /**
     * This method scrolls element into view if needed, and then uses
     * {@link Page.(screenshot:3) } to take a screenshot of the element.
     * If the element is detached from DOM, the method throws an error.
     */
    screenshot(this: ElementHandle<Element>, options?: ScreenshotOptions): Promise<string | Buffer>;
    /* Excluded from this release type: assertConnectedElement */
    /* Excluded from this release type: scrollIntoViewIfNeeded */
    /**
     * Resolves to true if the element is visible in the current viewport. If an
     * element is an SVG, we check if the svg owner element is in the viewport
     * instead. See https://crbug.com/963246.
     *
     * @param options - Threshold for the intersection between 0 (no intersection) and 1
     * (full intersection). Defaults to 1.
     */
    isIntersectingViewport(this: ElementHandle<Element>, options?: {
        threshold?: number;
    }): Promise<boolean>;
    /**
     * Scrolls the element into view using either the automation protocol client
     * or by calling element.scrollIntoView.
     */
    scrollIntoView(this: ElementHandle<Element>): Promise<void>;
    /* Excluded from this release type: assertElementHasWorld */
    /**
     * If the element is a form input, you can use {@link ElementHandle.autofill}
     * to test if the form is compatible with the browser's autofill
     * implementation. Throws an error if the form cannot be autofilled.
     *
     * @remarks
     *
     * Currently, Puppeteer supports auto-filling credit card information only and
     * in Chrome in the new headless and headful modes only.
     *
     * ```ts
     * // Select an input on the credit card form.
     * const name = await page.waitForSelector('form #name');
     * // Trigger autofill with the desired data.
     * await name.autofill({
     *   creditCard: {
     *     number: '4444444444444444',
     *     name: 'John Smith',
     *     expiryMonth: '01',
     *     expiryYear: '2030',
     *     cvc: '123',
     *   },
     * });
     * ```
     */
    abstract autofill(data: AutofillData): Promise<void>;
}

/* Excluded from this release type: EmulationManager */

/**
 * @public
 */
export declare type ErrorCode = 'aborted' | 'accessdenied' | 'addressunreachable' | 'blockedbyclient' | 'blockedbyresponse' | 'connectionaborted' | 'connectionclosed' | 'connectionfailed' | 'connectionrefused' | 'connectionreset' | 'internetdisconnected' | 'namenotresolved' | 'timedout' | 'failed';

/* Excluded from this release type: ErrorLike */

/**
 * @deprecated Import error classes directly.
 *
 * Puppeteer methods might throw errors if they are unable to fulfill a request.
 * For example, `page.waitForSelector(selector[, options])` might fail if the
 * selector doesn't match any nodes during the given timeframe.
 *
 * For certain types of errors Puppeteer uses specific error classes. These
 * classes are available via `puppeteer.errors`.
 *
 * @example
 * An example of handling a timeout error:
 *
 * ```ts
 * try {
 *   await page.waitForSelector('.foo');
 * } catch (e) {
 *   if (e instanceof TimeoutError) {
 *     // Do something if this is a timeout.
 *   }
 * }
 * ```
 *
 * @public
 */
export declare const errors: PuppeteerErrors;

/**
 * @public
 */
export declare type EvaluateFunc<T extends unknown[]> = (...params: InnerParams<T>) => Awaitable<unknown>;

/**
 * @public
 */
export declare type EvaluateFuncWith<V, T extends unknown[]> = (...params: [V, ...InnerParams<T>]) => Awaitable<unknown>;

/* Excluded from this release type: evaluationString */

/**
 * The EventEmitter class that many Puppeteer classes extend.
 *
 * @remarks
 *
 * This allows you to listen to events that Puppeteer classes fire and act
 * accordingly. Therefore you'll mostly use {@link EventEmitter.on | on} and
 * {@link EventEmitter.off | off} to bind
 * and unbind to event listeners.
 *
 * @public
 */
export declare class EventEmitter implements CommonEventEmitter {
    private emitter;
    private eventsMap;
    /* Excluded from this release type: __constructor */
    /**
     * Bind an event listener to fire when an event occurs.
     * @param event - the event type you'd like to listen to. Can be a string or symbol.
     * @param handler - the function to be called when the event occurs.
     * @returns `this` to enable you to chain method calls.
     */
    on(event: EventType, handler: Handler<any>): this;
    /**
     * Remove an event listener from firing.
     * @param event - the event type you'd like to stop listening to.
     * @param handler - the function that should be removed.
     * @returns `this` to enable you to chain method calls.
     */
    off(event: EventType, handler: Handler<any>): this;
    /**
     * Remove an event listener.
     * @deprecated please use {@link EventEmitter.off} instead.
     */
    removeListener(event: EventType, handler: Handler<any>): this;
    /**
     * Add an event listener.
     * @deprecated please use {@link EventEmitter.on} instead.
     */
    addListener(event: EventType, handler: Handler<any>): this;
    /**
     * Emit an event and call any associated listeners.
     *
     * @param event - the event you'd like to emit
     * @param eventData - any data you'd like to emit with the event
     * @returns `true` if there are any listeners, `false` if there are not.
     */
    emit(event: EventType, eventData?: unknown): boolean;
    /**
     * Like `on` but the listener will only be fired once and then it will be removed.
     * @param event - the event you'd like to listen to
     * @param handler - the handler function to run when the event occurs
     * @returns `this` to enable you to chain method calls.
     */
    once(event: EventType, handler: Handler<any>): this;
    /**
     * Gets the number of listeners for a given event.
     *
     * @param event - the event to get the listener count for
     * @returns the number of listeners bound to the given event
     */
    listenerCount(event: EventType): number;
    /**
     * Removes all listeners. If given an event argument, it will remove only
     * listeners for that event.
     * @param event - the event to remove listeners for.
     * @returns `this` to enable you to chain method calls.
     */
    removeAllListeners(event?: EventType): this;
    private eventListenersCount;
}

/**
 * Copyright 2022 Google Inc. All rights reserved.
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
/**
 * @public
 */
export declare type EventType = string | symbol;

/* Excluded from this release type: ExceptionThrownCallback */

/**
 * @public
 */
export declare const 
/**
* @public
*/
/**
 * @public
 */
executablePath: (channel?: ChromeReleaseChannel | undefined) => string;

/* Excluded from this release type: ExecutionContext */

/**
 * Defines experiment options for Puppeteer.
 *
 * See individual properties for more information.
 *
 * @public
 */
export declare type ExperimentsConfiguration = Record<string, never>;

/* Excluded from this release type: FetchRequestId */

/**
 * File choosers let you react to the page requesting for a file.
 *
 * @remarks
 * `FileChooser` instances are returned via the {@link Page.waitForFileChooser} method.
 *
 * In browsers, only one file chooser can be opened at a time.
 * All file choosers must be accepted or canceled. Not doing so will prevent
 * subsequent file choosers from appearing.
 *
 * @example
 *
 * ```ts
 * const [fileChooser] = await Promise.all([
 *   page.waitForFileChooser(),
 *   page.click('#upload-file-button'), // some button that triggers file selection
 * ]);
 * await fileChooser.accept(['/tmp/myfile.pdf']);
 * ```
 *
 * @public
 */
export declare class FileChooser {
    #private;
    /* Excluded from this release type: __constructor */
    /**
     * Whether file chooser allow for
     * {@link https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input/file#attr-multiple | multiple}
     * file selection.
     */
    isMultiple(): boolean;
    /**
     * Accept the file chooser request with the given file paths.
     *
     * @remarks This will not validate whether the file paths exists. Also, if a
     * path is relative, then it is resolved against the
     * {@link https://nodejs.org/api/process.html#process_process_cwd | current working directory}.
     * For locals script connecting to remote chrome environments, paths must be
     * absolute.
     */
    accept(paths: string[]): Promise<void>;
    /**
     * Closes the file chooser without selecting any files.
     */
    cancel(): void;
}

/* Excluded from this release type: FilteredLocator */

/* Excluded from this release type: FirefoxLauncher */

/* Excluded from this release type: FirefoxTargetManager */

declare type FlatmapSplitWithDelemiters<Inputs extends readonly string[], Delemiters extends readonly string[], Acc extends string[] = []> = Inputs extends [infer FirstInput, ...infer RestInputs] ? FirstInput extends string ? RestInputs extends readonly string[] ? FlatmapSplitWithDelemiters<RestInputs, Delemiters, [
...Acc,
...SplitWithDelemiters<FirstInput, Delemiters>
]> : Acc : Acc : Acc;

/**
 * @public
 */
export declare type FlattenHandle<T> = T extends HandleOr<infer U> ? U : never;

/* Excluded from this release type: FlattenLazyArg */

/**
 * Represents a DOM frame.
 *
 * To understand frames, you can think of frames as `<iframe>` elements. Just
 * like iframes, frames can be nested, and when JavaScript is executed in a
 * frame, the JavaScript does not effect frames inside the ambient frame the
 * JavaScript executes in.
 *
 * @example
 * At any point in time, {@link Page | pages} expose their current frame
 * tree via the {@link Page.mainFrame} and {@link Frame.childFrames} methods.
 *
 * @example
 * An example of dumping frame tree:
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.goto('https://www.google.com/chrome/browser/canary.html');
 *   dumpFrameTree(page.mainFrame(), '');
 *   await browser.close();
 *
 *   function dumpFrameTree(frame, indent) {
 *     console.log(indent + frame.url());
 *     for (const child of frame.childFrames()) {
 *       dumpFrameTree(child, indent + '  ');
 *     }
 *   }
 * })();
 * ```
 *
 * @example
 * An example of getting text from an iframe element:
 *
 * ```ts
 * const frame = page.frames().find(frame => frame.name() === 'myframe');
 * const text = await frame.$eval('.selector', element => element.textContent);
 * console.log(text);
 * ```
 *
 * @remarks
 * Frame lifecycles are controlled by three events that are all dispatched on
 * the parent {@link Frame.page | page}:
 *
 * - {@link PageEmittedEvents.FrameAttached}
 * - {@link PageEmittedEvents.FrameNavigated}
 * - {@link PageEmittedEvents.FrameDetached}
 *
 * @public
 */
export declare class Frame extends EventEmitter {
    /* Excluded from this release type: _id */
    /* Excluded from this release type: _parentId */
    /* Excluded from this release type: worlds */
    /* Excluded from this release type: _name */
    /* Excluded from this release type: _hasStartedLoading */
    /* Excluded from this release type: __constructor */
    /**
     * The page associated with the frame.
     */
    page(): Page;
    /**
     * Is `true` if the frame is an out-of-process (OOP) frame. Otherwise,
     * `false`.
     */
    isOOPFrame(): boolean;
    /**
     * Navigates a frame to the given url.
     *
     * @remarks
     * Navigation to `about:blank` or navigation to the same URL with a different
     * hash will succeed and return `null`.
     *
     * :::warning
     *
     * Headless mode doesn't support navigation to a PDF document. See the {@link
     * https://bugs.chromium.org/p/chromium/issues/detail?id=761295 | upstream
     * issue}.
     *
     * :::
     *
     * @param url - the URL to navigate the frame to. This should include the
     * scheme, e.g. `https://`.
     * @param options - navigation options. `waitUntil` is useful to define when
     * the navigation should be considered successful - see the docs for
     * {@link PuppeteerLifeCycleEvent} for more details.
     *
     * @returns A promise which resolves to the main resource response. In case of
     * multiple redirects, the navigation will resolve with the response of the
     * last redirect.
     * @throws This method will throw an error if:
     *
     * - there's an SSL error (e.g. in case of self-signed certificates).
     * - target URL is invalid.
     * - the `timeout` is exceeded during navigation.
     * - the remote server does not respond or is unreachable.
     * - the main resource failed to load.
     *
     * This method will not throw an error when any valid HTTP status code is
     * returned by the remote server, including 404 "Not Found" and 500 "Internal
     * Server Error". The status code for such responses can be retrieved by
     * calling {@link HTTPResponse.status}.
     */
    goto(url: string, options?: {
        referer?: string;
        referrerPolicy?: string;
        timeout?: number;
        waitUntil?: PuppeteerLifeCycleEvent | PuppeteerLifeCycleEvent[];
    }): Promise<HTTPResponse | null>;
    /**
     * Waits for the frame to navigate. It is useful for when you run code which
     * will indirectly cause the frame to navigate.
     *
     * Usage of the
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/History_API | History API}
     * to change the URL is considered a navigation.
     *
     * @example
     *
     * ```ts
     * const [response] = await Promise.all([
     *   // The navigation promise resolves after navigation has finished
     *   frame.waitForNavigation(),
     *   // Clicking the link will indirectly cause a navigation
     *   frame.click('a.my-link'),
     * ]);
     * ```
     *
     * @param options - options to configure when the navigation is consided
     * finished.
     * @returns a promise that resolves when the frame navigates to a new URL.
     */
    waitForNavigation(options?: {
        timeout?: number;
        waitUntil?: PuppeteerLifeCycleEvent | PuppeteerLifeCycleEvent[];
    }): Promise<HTTPResponse | null>;
    /* Excluded from this release type: _client */
    /* Excluded from this release type: executionContext */
    /* Excluded from this release type: mainRealm */
    /* Excluded from this release type: isolatedRealm */
    /* Excluded from this release type: frameElement */
    /**
     * Behaves identically to {@link Page.evaluateHandle} except it's run within
     * the context of this frame.
     *
     * @see {@link Page.evaluateHandle} for details.
     */
    evaluateHandle<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
    /**
     * Behaves identically to {@link Page.evaluate} except it's run within the
     * the context of this frame.
     *
     * @see {@link Page.evaluate} for details.
     */
    evaluate<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * Creates a locator for the provided selector. See {@link Locator} for
     * details and supported actions.
     *
     * @remarks
     * Locators API is experimental and we will not follow semver for breaking
     * change in the Locators API.
     */
    locator<Selector extends string>(selector: Selector): Locator<NodeFor<Selector>>;
    /**
     * Creates a locator for the provided function. See {@link Locator} for
     * details and supported actions.
     *
     * @remarks
     * Locators API is experimental and we will not follow semver for breaking
     * change in the Locators API.
     */
    locator<Ret>(func: () => Awaitable<Ret>): Locator<Ret>;
    /**
     * Queries the frame for an element matching the given selector.
     *
     * @param selector - The selector to query for.
     * @returns A {@link ElementHandle | element handle} to the first element
     * matching the given selector. Otherwise, `null`.
     */
    $<Selector extends string>(selector: Selector): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * Queries the frame for all elements matching the given selector.
     *
     * @param selector - The selector to query for.
     * @returns An array of {@link ElementHandle | element handles} that point to
     * elements matching the given selector.
     */
    $$<Selector extends string>(selector: Selector): Promise<Array<ElementHandle<NodeFor<Selector>>>>;
    /**
     * Runs the given function on the first element matching the given selector in
     * the frame.
     *
     * If the given function returns a promise, then this method will wait till
     * the promise resolves.
     *
     * @example
     *
     * ```ts
     * const searchValue = await frame.$eval('#search', el => el.value);
     * ```
     *
     * @param selector - The selector to query for.
     * @param pageFunction - The function to be evaluated in the frame's context.
     * The first element matching the selector will be passed to the function as
     * its first argument.
     * @param args - Additional arguments to pass to `pageFunction`.
     * @returns A promise to the result of the function.
     */
    $eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<NodeFor<Selector>, Params> = EvaluateFuncWith<NodeFor<Selector>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * Runs the given function on an array of elements matching the given selector
     * in the frame.
     *
     * If the given function returns a promise, then this method will wait till
     * the promise resolves.
     *
     * @example
     *
     * ```js
     * const divsCounts = await frame.$$eval('div', divs => divs.length);
     * ```
     *
     * @param selector - The selector to query for.
     * @param pageFunction - The function to be evaluated in the frame's context.
     * An array of elements matching the given selector will be passed to the
     * function as its first argument.
     * @param args - Additional arguments to pass to `pageFunction`.
     * @returns A promise to the result of the function.
     */
    $$eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<Array<NodeFor<Selector>>, Params> = EvaluateFuncWith<Array<NodeFor<Selector>>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * @deprecated Use {@link Frame.$$} with the `xpath` prefix.
     *
     * Example: `await frame.$$('xpath/' + xpathExpression)`
     *
     * This method evaluates the given XPath expression and returns the results.
     * If `xpath` starts with `//` instead of `.//`, the dot will be appended
     * automatically.
     * @param expression - the XPath expression to evaluate.
     */
    $x(expression: string): Promise<Array<ElementHandle<Node>>>;
    /**
     * Waits for an element matching the given selector to appear in the frame.
     *
     * This method works across navigations.
     *
     * @example
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   let currentURL;
     *   page
     *     .mainFrame()
     *     .waitForSelector('img')
     *     .then(() => console.log('First URL with image: ' + currentURL));
     *
     *   for (currentURL of [
     *     'https://example.com',
     *     'https://google.com',
     *     'https://bbc.com',
     *   ]) {
     *     await page.goto(currentURL);
     *   }
     *   await browser.close();
     * })();
     * ```
     *
     * @param selector - The selector to query and wait for.
     * @param options - Options for customizing waiting behavior.
     * @returns An element matching the given selector.
     * @throws Throws if an element matching the given selector doesn't appear.
     */
    waitForSelector<Selector extends string>(selector: Selector, options?: WaitForSelectorOptions): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * @deprecated Use {@link Frame.waitForSelector} with the `xpath` prefix.
     *
     * Example: `await frame.waitForSelector('xpath/' + xpathExpression)`
     *
     * The method evaluates the XPath expression relative to the Frame.
     * If `xpath` starts with `//` instead of `.//`, the dot will be appended
     * automatically.
     *
     * Wait for the `xpath` to appear in page. If at the moment of calling the
     * method the `xpath` already exists, the method will return immediately. If
     * the xpath doesn't appear after the `timeout` milliseconds of waiting, the
     * function will throw.
     *
     * For a code example, see the example for {@link Frame.waitForSelector}. That
     * function behaves identically other than taking a CSS selector rather than
     * an XPath.
     *
     * @param xpath - the XPath expression to wait for.
     * @param options - options to configure the visibility of the element and how
     * long to wait before timing out.
     */
    waitForXPath(xpath: string, options?: WaitForSelectorOptions): Promise<ElementHandle<Node> | null>;
    /**
     * @example
     * The `waitForFunction` can be used to observe viewport size change:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     *
     * (async () => {
     * .  const browser = await puppeteer.launch();
     * .  const page = await browser.newPage();
     * .  const watchDog = page.mainFrame().waitForFunction('window.innerWidth < 100');
     * .  page.setViewport({width: 50, height: 50});
     * .  await watchDog;
     * .  await browser.close();
     * })();
     * ```
     *
     * To pass arguments from Node.js to the predicate of `page.waitForFunction` function:
     *
     * ```ts
     * const selector = '.foo';
     * await frame.waitForFunction(
     *   selector => !!document.querySelector(selector),
     *   {}, // empty options object
     *   selector
     * );
     * ```
     *
     * @param pageFunction - the function to evaluate in the frame context.
     * @param options - options to configure the polling method and timeout.
     * @param args - arguments to pass to the `pageFunction`.
     * @returns the promise which resolve when the `pageFunction` returns a truthy value.
     */
    waitForFunction<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, options?: FrameWaitForFunctionOptions, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
    /**
     * The full HTML contents of the frame, including the DOCTYPE.
     */
    content(): Promise<string>;
    /**
     * Set the content of the frame.
     *
     * @param html - HTML markup to assign to the page.
     * @param options - Options to configure how long before timing out and at
     * what point to consider the content setting successful.
     */
    setContent(html: string, options?: {
        timeout?: number;
        waitUntil?: PuppeteerLifeCycleEvent | PuppeteerLifeCycleEvent[];
    }): Promise<void>;
    /**
     * The frame's `name` attribute as specified in the tag.
     *
     * @remarks
     * If the name is empty, it returns the `id` attribute instead.
     *
     * @remarks
     * This value is calculated once when the frame is created, and will not
     * update if the attribute is changed later.
     */
    name(): string;
    /**
     * The frame's URL.
     */
    url(): string;
    /**
     * The parent frame, if any. Detached and main frames return `null`.
     */
    parentFrame(): Frame | null;
    /**
     * An array of child frames.
     */
    childFrames(): Frame[];
    /**
     * Is`true` if the frame has been detached. Otherwise, `false`.
     */
    isDetached(): boolean;
    /**
     * Adds a `<script>` tag into the page with the desired url or content.
     *
     * @param options - Options for the script.
     * @returns An {@link ElementHandle | element handle} to the injected
     * `<script>` element.
     */
    addScriptTag(options: FrameAddScriptTagOptions): Promise<ElementHandle<HTMLScriptElement>>;
    /**
     * Adds a `<link rel="stylesheet">` tag into the page with the desired URL or
     * a `<style type="text/css">` tag with the content.
     *
     * @returns An {@link ElementHandle | element handle} to the loaded `<link>`
     * or `<style>` element.
     */
    addStyleTag(options: Omit<FrameAddStyleTagOptions, 'url'>): Promise<ElementHandle<HTMLStyleElement>>;
    addStyleTag(options: FrameAddStyleTagOptions): Promise<ElementHandle<HTMLLinkElement>>;
    /**
     * Clicks the first element found that matches `selector`.
     *
     * @remarks
     * If `click()` triggers a navigation event and there's a separate
     * `page.waitForNavigation()` promise to be resolved, you may end up with a
     * race condition that yields unexpected results. The correct pattern for
     * click and wait for navigation is the following:
     *
     * ```ts
     * const [response] = await Promise.all([
     *   page.waitForNavigation(waitOptions),
     *   frame.click(selector, clickOptions),
     * ]);
     * ```
     *
     * @param selector - The selector to query for.
     */
    click(selector: string, options?: Readonly<ClickOptions>): Promise<void>;
    /**
     * Focuses the first element that matches the `selector`.
     *
     * @param selector - The selector to query for.
     * @throws Throws if there's no element matching `selector`.
     */
    focus(selector: string): Promise<void>;
    /**
     * Hovers the pointer over the center of the first element that matches the
     * `selector`.
     *
     * @param selector - The selector to query for.
     * @throws Throws if there's no element matching `selector`.
     */
    hover(selector: string): Promise<void>;
    /**
     * Selects a set of value on the first `<select>` element that matches the
     * `selector`.
     *
     * @example
     *
     * ```ts
     * frame.select('select#colors', 'blue'); // single selection
     * frame.select('select#colors', 'red', 'green', 'blue'); // multiple selections
     * ```
     *
     * @param selector - The selector to query for.
     * @param values - The array of values to select. If the `<select>` has the
     * `multiple` attribute, all values are considered, otherwise only the first
     * one is taken into account.
     * @returns the list of values that were successfully selected.
     * @throws Throws if there's no `<select>` matching `selector`.
     */
    select(selector: string, ...values: string[]): Promise<string[]>;
    /**
     * Taps the first element that matches the `selector`.
     *
     * @param selector - The selector to query for.
     * @throws Throws if there's no element matching `selector`.
     */
    tap(selector: string): Promise<void>;
    /**
     * Sends a `keydown`, `keypress`/`input`, and `keyup` event for each character
     * in the text.
     *
     * @remarks
     * To press a special key, like `Control` or `ArrowDown`, use
     * {@link Keyboard.press}.
     *
     * @example
     *
     * ```ts
     * await frame.type('#mytextarea', 'Hello'); // Types instantly
     * await frame.type('#mytextarea', 'World', {delay: 100}); // Types slower, like a user
     * ```
     *
     * @param selector - the selector for the element to type into. If there are
     * multiple the first will be used.
     * @param text - text to type into the element
     * @param options - takes one option, `delay`, which sets the time to wait
     * between key presses in milliseconds. Defaults to `0`.
     */
    type(selector: string, text: string, options?: Readonly<KeyboardTypeOptions>): Promise<void>;
    /**
     * @deprecated Replace with `new Promise(r => setTimeout(r, milliseconds));`.
     *
     * Causes your script to wait for the given number of milliseconds.
     *
     * @remarks
     * It's generally recommended to not wait for a number of seconds, but instead
     * use {@link Frame.waitForSelector}, {@link Frame.waitForXPath} or
     * {@link Frame.waitForFunction} to wait for exactly the conditions you want.
     *
     * @example
     *
     * Wait for 1 second:
     *
     * ```ts
     * await frame.waitForTimeout(1000);
     * ```
     *
     * @param milliseconds - the number of milliseconds to wait.
     */
    waitForTimeout(milliseconds: number): Promise<void>;
    /**
     * The frame's title.
     */
    title(): Promise<string>;
    /**
     * This method is typically coupled with an action that triggers a device
     * request from an api such as WebBluetooth.
     *
     * :::caution
     *
     * This must be called before the device request is made. It will not return a
     * currently active device prompt.
     *
     * :::
     *
     * @example
     *
     * ```ts
     * const [devicePrompt] = Promise.all([
     *   frame.waitForDevicePrompt(),
     *   frame.click('#connect-bluetooth'),
     * ]);
     * await devicePrompt.select(
     *   await devicePrompt.waitForDevice(({name}) => name.includes('My Device'))
     * );
     * ```
     */
    waitForDevicePrompt(options?: WaitTimeoutOptions): Promise<DeviceRequestPrompt>;
}

/* Excluded from this release type: Frame_2 */

/**
 * @public
 */
export declare interface FrameAddScriptTagOptions {
    /**
     * URL of the script to be added.
     */
    url?: string;
    /**
     * Path to a JavaScript file to be injected into the frame.
     *
     * @remarks
     * If `path` is a relative path, it is resolved relative to the current
     * working directory (`process.cwd()` in Node.js).
     */
    path?: string;
    /**
     * JavaScript to be injected into the frame.
     */
    content?: string;
    /**
     * Sets the `type` of the script. Use `module` in order to load an ES2015 module.
     */
    type?: string;
    /**
     * Sets the `id` of the script.
     */
    id?: string;
}

/**
 * @public
 */
export declare interface FrameAddStyleTagOptions {
    /**
     * the URL of the CSS file to be added.
     */
    url?: string;
    /**
     * The path to a CSS file to be injected into the frame.
     * @remarks
     * If `path` is a relative path, it is resolved relative to the current
     * working directory (`process.cwd()` in Node.js).
     */
    path?: string;
    /**
     * Raw CSS content to be injected into the frame.
     */
    content?: string;
}

/* Excluded from this release type: FrameManager */

/* Excluded from this release type: FrameManagerEmittedEvents */

/* Excluded from this release type: FrameTree */

/**
 * @public
 */
export declare interface FrameWaitForFunctionOptions {
    /**
     * An interval at which the `pageFunction` is executed, defaults to `raf`. If
     * `polling` is a number, then it is treated as an interval in milliseconds at
     * which the function would be executed. If `polling` is a string, then it can
     * be one of the following values:
     *
     * - `raf` - to constantly execute `pageFunction` in `requestAnimationFrame`
     *   callback. This is the tightest polling mode which is suitable to observe
     *   styling changes.
     *
     * - `mutation` - to execute `pageFunction` on every DOM mutation.
     */
    polling?: 'raf' | 'mutation' | number;
    /**
     * Maximum time to wait in milliseconds. Defaults to `30000` (30 seconds).
     * Pass `0` to disable the timeout. Puppeteer's default timeout can be changed
     * using {@link Page.setDefaultTimeout}.
     */
    timeout?: number;
    /**
     * A signal object that allows you to cancel a waitForFunction call.
     */
    signal?: AbortSignal;
}

/* Excluded from this release type: FunctionLocator */

/**
 * @public
 */
export declare interface GeolocationOptions {
    /**
     * Latitude between `-90` and `90`.
     */
    longitude: number;
    /**
     * Longitude between `-180` and `180`.
     */
    latitude: number;
    /**
     * Optional non-negative accuracy value.
     */
    accuracy?: number;
}

/* Excluded from this release type: getCapturedLogs */

/* Excluded from this release type: getFetch */

/* Excluded from this release type: getPageContent */

/* Excluded from this release type: getReadableAsBuffer */

/* Excluded from this release type: getReadableFromProtocolStream */

/* Excluded from this release type: getSourcePuppeteerURLIfAvailable */

/**
 * @public
 */
export declare type HandleFor<T> = T extends Node ? ElementHandle<T> : JSHandle<T>;

/* Excluded from this release type: HandleMapper */

/**
 * @public
 */
export declare type HandleOr<T> = HandleFor<T> | JSHandle<T> | T;

/* Excluded from this release type: HandlePredicate */

/**
 * @public
 */
export declare type Handler<T = unknown> = (event: T) => void;

/* Excluded from this release type: headersArray */

/**
 * Represents an HTTP request sent by a page.
 * @remarks
 *
 * Whenever the page sends a request, such as for a network resource, the
 * following events are emitted by Puppeteer's `page`:
 *
 * - `request`: emitted when the request is issued by the page.
 * - `requestfinished` - emitted when the response body is downloaded and the
 *   request is complete.
 *
 * If request fails at some point, then instead of `requestfinished` event the
 * `requestfailed` event is emitted.
 *
 * All of these events provide an instance of `HTTPRequest` representing the
 * request that occurred:
 *
 * ```
 * page.on('request', request => ...)
 * ```
 *
 * NOTE: HTTP Error responses, such as 404 or 503, are still successful
 * responses from HTTP standpoint, so request will complete with
 * `requestfinished` event.
 *
 * If request gets a 'redirect' response, the request is successfully finished
 * with the `requestfinished` event, and a new request is issued to a
 * redirected url.
 *
 * @public
 */
export declare class HTTPRequest {
    /* Excluded from this release type: _requestId */
    /* Excluded from this release type: _interceptionId */
    /* Excluded from this release type: _failureText */
    /* Excluded from this release type: _response */
    /* Excluded from this release type: _fromMemoryCache */
    /* Excluded from this release type: _redirectChain */
    /**
     * Warning! Using this client can break Puppeteer. Use with caution.
     *
     * @experimental
     */
    get client(): CDPSession;
    /* Excluded from this release type: __constructor */
    /**
     * The URL of the request
     */
    url(): string;
    /**
     * The `ContinueRequestOverrides` that will be used
     * if the interception is allowed to continue (ie, `abort()` and
     * `respond()` aren't called).
     */
    continueRequestOverrides(): ContinueRequestOverrides;
    /**
     * The `ResponseForRequest` that gets used if the
     * interception is allowed to respond (ie, `abort()` is not called).
     */
    responseForRequest(): Partial<ResponseForRequest> | null;
    /**
     * The most recent reason for aborting the request
     */
    abortErrorReason(): Protocol.Network.ErrorReason | null;
    /**
     * An InterceptResolutionState object describing the current resolution
     * action and priority.
     *
     * InterceptResolutionState contains:
     * action: InterceptResolutionAction
     * priority?: number
     *
     * InterceptResolutionAction is one of: `abort`, `respond`, `continue`,
     * `disabled`, `none`, or `already-handled`.
     */
    interceptResolutionState(): InterceptResolutionState;
    /**
     * Is `true` if the intercept resolution has already been handled,
     * `false` otherwise.
     */
    isInterceptResolutionHandled(): boolean;
    /**
     * Adds an async request handler to the processing queue.
     * Deferred handlers are not guaranteed to execute in any particular order,
     * but they are guaranteed to resolve before the request interception
     * is finalized.
     */
    enqueueInterceptAction(pendingHandler: () => void | PromiseLike<unknown>): void;
    /**
     * Awaits pending interception handlers and then decides how to fulfill
     * the request interception.
     */
    finalizeInterceptions(): Promise<void>;
    /**
     * Contains the request's resource type as it was perceived by the rendering
     * engine.
     */
    resourceType(): ResourceType;
    /**
     * The method used (`GET`, `POST`, etc.)
     */
    method(): string;
    /**
     * The request's post body, if any.
     */
    postData(): string | undefined;
    /**
     * An object with HTTP headers associated with the request. All
     * header names are lower-case.
     */
    headers(): Record<string, string>;
    /**
     * A matching `HTTPResponse` object, or null if the response has not
     * been received yet.
     */
    response(): HTTPResponse | null;
    /**
     * The frame that initiated the request, or null if navigating to
     * error pages.
     */
    frame(): Frame | null;
    /**
     * True if the request is the driver of the current frame's navigation.
     */
    isNavigationRequest(): boolean;
    /**
     * The initiator of the request.
     */
    initiator(): Protocol.Network.Initiator | undefined;
    /**
     * A `redirectChain` is a chain of requests initiated to fetch a resource.
     * @remarks
     *
     * `redirectChain` is shared between all the requests of the same chain.
     *
     * For example, if the website `http://example.com` has a single redirect to
     * `https://example.com`, then the chain will contain one request:
     *
     * ```ts
     * const response = await page.goto('http://example.com');
     * const chain = response.request().redirectChain();
     * console.log(chain.length); // 1
     * console.log(chain[0].url()); // 'http://example.com'
     * ```
     *
     * If the website `https://google.com` has no redirects, then the chain will be empty:
     *
     * ```ts
     * const response = await page.goto('https://google.com');
     * const chain = response.request().redirectChain();
     * console.log(chain.length); // 0
     * ```
     *
     * @returns the chain of requests - if a server responds with at least a
     * single redirect, this chain will contain all requests that were redirected.
     */
    redirectChain(): HTTPRequest[];
    /**
     * Access information about the request's failure.
     *
     * @remarks
     *
     * @example
     *
     * Example of logging all failed requests:
     *
     * ```ts
     * page.on('requestfailed', request => {
     *   console.log(request.url() + ' ' + request.failure().errorText);
     * });
     * ```
     *
     * @returns `null` unless the request failed. If the request fails this can
     * return an object with `errorText` containing a human-readable error
     * message, e.g. `net::ERR_FAILED`. It is not guaranteed that there will be
     * failure text if the request fails.
     */
    failure(): {
        errorText: string;
    } | null;
    /**
     * Continues request with optional request overrides.
     *
     * @remarks
     *
     * To use this, request
     * interception should be enabled with {@link Page.setRequestInterception}.
     *
     * Exception is immediately thrown if the request interception is not enabled.
     *
     * @example
     *
     * ```ts
     * await page.setRequestInterception(true);
     * page.on('request', request => {
     *   // Override headers
     *   const headers = Object.assign({}, request.headers(), {
     *     foo: 'bar', // set "foo" header
     *     origin: undefined, // remove "origin" header
     *   });
     *   request.continue({headers});
     * });
     * ```
     *
     * @param overrides - optional overrides to apply to the request.
     * @param priority - If provided, intercept is resolved using
     * cooperative handling rules. Otherwise, intercept is resolved
     * immediately.
     */
    continue(overrides?: ContinueRequestOverrides, priority?: number): Promise<void>;
    /**
     * Fulfills a request with the given response.
     *
     * @remarks
     *
     * To use this, request
     * interception should be enabled with {@link Page.setRequestInterception}.
     *
     * Exception is immediately thrown if the request interception is not enabled.
     *
     * @example
     * An example of fulfilling all requests with 404 responses:
     *
     * ```ts
     * await page.setRequestInterception(true);
     * page.on('request', request => {
     *   request.respond({
     *     status: 404,
     *     contentType: 'text/plain',
     *     body: 'Not Found!',
     *   });
     * });
     * ```
     *
     * NOTE: Mocking responses for dataURL requests is not supported.
     * Calling `request.respond` for a dataURL request is a noop.
     *
     * @param response - the response to fulfill the request with.
     * @param priority - If provided, intercept is resolved using
     * cooperative handling rules. Otherwise, intercept is resolved
     * immediately.
     */
    respond(response: Partial<ResponseForRequest>, priority?: number): Promise<void>;
    /**
     * Aborts a request.
     *
     * @remarks
     * To use this, request interception should be enabled with
     * {@link Page.setRequestInterception}. If it is not enabled, this method will
     * throw an exception immediately.
     *
     * @param errorCode - optional error code to provide.
     * @param priority - If provided, intercept is resolved using
     * cooperative handling rules. Otherwise, intercept is resolved
     * immediately.
     */
    abort(errorCode?: ErrorCode, priority?: number): Promise<void>;
}

/* Excluded from this release type: HTTPRequest_2 */

/**
 * The HTTPResponse class represents responses which are received by the
 * {@link Page} class.
 *
 * @public
 */
export declare class HTTPResponse {
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: _resolveBody */
    /**
     * The IP address and port number used to connect to the remote
     * server.
     */
    remoteAddress(): RemoteAddress;
    /**
     * The URL of the response.
     */
    url(): string;
    /**
     * True if the response was successful (status in the range 200-299).
     */
    ok(): boolean;
    /**
     * The status code of the response (e.g., 200 for a success).
     */
    status(): number;
    /**
     * The status text of the response (e.g. usually an "OK" for a
     * success).
     */
    statusText(): string;
    /**
     * An object with HTTP headers associated with the response. All
     * header names are lower-case.
     */
    headers(): Record<string, string>;
    /**
     * {@link SecurityDetails} if the response was received over the
     * secure connection, or `null` otherwise.
     */
    securityDetails(): SecurityDetails | null;
    /**
     * Timing information related to the response.
     */
    timing(): Protocol.Network.ResourceTiming | null;
    /**
     * Promise which resolves to a buffer with response body.
     */
    buffer(): Promise<Buffer>;
    /**
     * Promise which resolves to a text representation of response body.
     */
    text(): Promise<string>;
    /**
     * Promise which resolves to a JSON representation of response body.
     *
     * @remarks
     *
     * This method will throw if the response body is not parsable via
     * `JSON.parse`.
     */
    json(): Promise<any>;
    /**
     * A matching {@link HTTPRequest} object.
     */
    request(): HTTPRequest;
    /**
     * True if the response was served from either the browser's disk
     * cache or memory cache.
     */
    fromCache(): boolean;
    /**
     * True if the response was served by a service worker.
     */
    fromServiceWorker(): boolean;
    /**
     * A {@link Frame} that initiated this response, or `null` if
     * navigating to error pages.
     */
    frame(): Frame | null;
}

/* Excluded from this release type: importDebug */

/* Excluded from this release type: importFSPromises */

/* Excluded from this release type: InitializationStatus */

/* Excluded from this release type: InnerLazyParams */

/**
 * @public
 */
export declare type InnerParams<T extends unknown[]> = {
    [K in keyof T]: FlattenHandle<T[K]>;
};

/**
 * @public
 */
export declare enum InterceptResolutionAction {
    Abort = "abort",
    Respond = "respond",
    Continue = "continue",
    Disabled = "disabled",
    None = "none",
    AlreadyHandled = "already-handled"
}

/**
 * @public
 */
export declare interface InterceptResolutionState {
    action: InterceptResolutionAction;
    priority?: number;
}

/**
 * @public
 *
 * @deprecated please use {@link InterceptResolutionAction} instead.
 */
export declare type InterceptResolutionStrategy = InterceptResolutionAction;

/**
 * @public
 */
export declare interface InternalNetworkConditions extends NetworkConditions {
    offline: boolean;
}

/* Excluded from this release type: IntervalPoller */

/* Excluded from this release type: isDate */

/* Excluded from this release type: isErrnoException */

/* Excluded from this release type: isErrorLike */

/* Excluded from this release type: isNumber */

/* Excluded from this release type: IsolatedWorld */

/* Excluded from this release type: IsolatedWorldChart */

/* Excluded from this release type: IsPageTargetCallback */

/* Excluded from this release type: isPlainObject */

/* Excluded from this release type: isRegExp */

/* Excluded from this release type: isString */

/* Excluded from this release type: isTargetClosedError */

/**
 * @public
 */
export declare class JSCoverage {
    #private;
    constructor(client: CDPSession);
    /* Excluded from this release type: updateClient */
    start(options?: {
        resetOnNavigation?: boolean;
        reportAnonymousScripts?: boolean;
        includeRawScriptCoverage?: boolean;
        useBlockCoverage?: boolean;
    }): Promise<void>;
    stop(): Promise<JSCoverageEntry[]>;
}

/**
 * The CoverageEntry class for JavaScript
 * @public
 */
export declare interface JSCoverageEntry extends CoverageEntry {
    /**
     * Raw V8 script coverage entry.
     */
    rawScriptCoverage?: Protocol.Profiler.ScriptCoverage;
}

/**
 * Set of configurable options for JS coverage.
 * @public
 */
export declare interface JSCoverageOptions {
    /**
     * Whether to reset coverage on every navigation.
     */
    resetOnNavigation?: boolean;
    /**
     * Whether anonymous scripts generated by the page should be reported.
     */
    reportAnonymousScripts?: boolean;
    /**
     * Whether the result includes raw V8 script coverage entries.
     */
    includeRawScriptCoverage?: boolean;
    /**
     * Whether to collect coverage information at the block level.
     * If true, coverage will be collected at the block level (this is the default).
     * If false, coverage will be collected at the function level.
     */
    useBlockCoverage?: boolean;
}

/**
 * Represents a reference to a JavaScript object. Instances can be created using
 * {@link Page.evaluateHandle}.
 *
 * Handles prevent the referenced JavaScript object from being garbage-collected
 * unless the handle is purposely {@link JSHandle.dispose | disposed}. JSHandles
 * are auto-disposed when their associated frame is navigated away or the parent
 * context gets destroyed.
 *
 * Handles can be used as arguments for any evaluation function such as
 * {@link Page.$eval}, {@link Page.evaluate}, and {@link Page.evaluateHandle}.
 * They are resolved to their referenced object.
 *
 * @example
 *
 * ```ts
 * const windowHandle = await page.evaluateHandle(() => window);
 * ```
 *
 * @public
 */
export declare abstract class JSHandle<T = unknown> {
    /**
     * Used for nominally typing {@link JSHandle}.
     */
    _?: T;
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: disposed */
    /**
     * Evaluates the given function with the current handle as its first argument.
     */
    abstract evaluate<Params extends unknown[], Func extends EvaluateFuncWith<T, Params> = EvaluateFuncWith<T, Params>>(pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * Evaluates the given function with the current handle as its first argument.
     *
     */
    abstract evaluateHandle<Params extends unknown[], Func extends EvaluateFuncWith<T, Params> = EvaluateFuncWith<T, Params>>(pageFunction: Func | string, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
    /**
     * Fetches a single property from the referenced object.
     */
    abstract getProperty<K extends keyof T>(propertyName: HandleOr<K>): Promise<HandleFor<T[K]>>;
    abstract getProperty(propertyName: string): Promise<JSHandle<unknown>>;
    /**
     * Gets a map of handles representing the properties of the current handle.
     *
     * @example
     *
     * ```ts
     * const listHandle = await page.evaluateHandle(() => document.body.children);
     * const properties = await listHandle.getProperties();
     * const children = [];
     * for (const property of properties.values()) {
     *   const element = property.asElement();
     *   if (element) {
     *     children.push(element);
     *   }
     * }
     * children; // holds elementHandles to all children of document.body
     * ```
     */
    abstract getProperties(): Promise<Map<string, JSHandle<unknown>>>;
    /**
     * A vanilla object representing the serializable portions of the
     * referenced object.
     * @throws Throws if the object cannot be serialized due to circularity.
     *
     * @remarks
     * If the object has a `toJSON` function, it **will not** be called.
     */
    abstract jsonValue(): Promise<T>;
    /**
     * Either `null` or the handle itself if the handle is an
     * instance of {@link ElementHandle}.
     */
    abstract asElement(): ElementHandle<Node> | null;
    /**
     * Releases the object referenced by the handle for garbage collection.
     */
    abstract dispose(): Promise<void>;
    /**
     * Returns a string representation of the JSHandle.
     *
     * @remarks
     * Useful during debugging.
     */
    abstract toString(): string;
    /* Excluded from this release type: id */
    /**
     * Provides access to the
     * {@link https://chromedevtools.github.io/devtools-protocol/tot/Runtime/#type-RemoteObject | Protocol.Runtime.RemoteObject}
     * backing this handle.
     */
    abstract remoteObject(): Protocol.Runtime.RemoteObject;
}

/**
 * Keyboard provides an api for managing a virtual keyboard.
 * The high level api is {@link Keyboard."type"},
 * which takes raw characters and generates proper keydown, keypress/input,
 * and keyup events on your page.
 *
 * @remarks
 * For finer control, you can use {@link Keyboard.down},
 * {@link Keyboard.up}, and {@link Keyboard.sendCharacter}
 * to manually fire events as if they were generated from a real keyboard.
 *
 * On macOS, keyboard shortcuts like `⌘ A` -\> Select All do not work.
 * See {@link https://github.com/puppeteer/puppeteer/issues/1313 | #1313}.
 *
 * @example
 * An example of holding down `Shift` in order to select and delete some text:
 *
 * ```ts
 * await page.keyboard.type('Hello World!');
 * await page.keyboard.press('ArrowLeft');
 *
 * await page.keyboard.down('Shift');
 * for (let i = 0; i < ' World'.length; i++)
 *   await page.keyboard.press('ArrowLeft');
 * await page.keyboard.up('Shift');
 *
 * await page.keyboard.press('Backspace');
 * // Result text will end up saying 'Hello!'
 * ```
 *
 * @example
 * An example of pressing `A`
 *
 * ```ts
 * await page.keyboard.down('Shift');
 * await page.keyboard.press('KeyA');
 * await page.keyboard.up('Shift');
 * ```
 *
 * @public
 */
export declare class Keyboard {
    /* Excluded from this release type: __constructor */
    /**
     * Dispatches a `keydown` event.
     *
     * @remarks
     * If `key` is a single character and no modifier keys besides `Shift`
     * are being held down, a `keypress`/`input` event will also generated.
     * The `text` option can be specified to force an input event to be generated.
     * If `key` is a modifier key, `Shift`, `Meta`, `Control`, or `Alt`,
     * subsequent key presses will be sent with that modifier active.
     * To release the modifier key, use {@link Keyboard.up}.
     *
     * After the key is pressed once, subsequent calls to
     * {@link Keyboard.down} will have
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/repeat | repeat}
     * set to true. To release the key, use {@link Keyboard.up}.
     *
     * Modifier keys DO influence {@link Keyboard.down}.
     * Holding down `Shift` will type the text in upper case.
     *
     * @param key - Name of key to press, such as `ArrowLeft`.
     * See {@link KeyInput} for a list of all key names.
     *
     * @param options - An object of options. Accepts text which, if specified,
     * generates an input event with this text. Accepts commands which, if specified,
     * is the commands of keyboard shortcuts,
     * see {@link https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/editing/commands/editor_command_names.h | Chromium Source Code} for valid command names.
     */
    down(key: KeyInput, options?: Readonly<KeyDownOptions>): Promise<void>;
    /**
     * Dispatches a `keyup` event.
     *
     * @param key - Name of key to release, such as `ArrowLeft`.
     * See {@link KeyInput | KeyInput}
     * for a list of all key names.
     */
    up(key: KeyInput): Promise<void>;
    /**
     * Dispatches a `keypress` and `input` event.
     * This does not send a `keydown` or `keyup` event.
     *
     * @remarks
     * Modifier keys DO NOT effect {@link Keyboard.sendCharacter | Keyboard.sendCharacter}.
     * Holding down `Shift` will not type the text in upper case.
     *
     * @example
     *
     * ```ts
     * page.keyboard.sendCharacter('嗨');
     * ```
     *
     * @param char - Character to send into the page.
     */
    sendCharacter(char: string): Promise<void>;
    /**
     * Sends a `keydown`, `keypress`/`input`,
     * and `keyup` event for each character in the text.
     *
     * @remarks
     * To press a special key, like `Control` or `ArrowDown`,
     * use {@link Keyboard.press}.
     *
     * Modifier keys DO NOT effect `keyboard.type`.
     * Holding down `Shift` will not type the text in upper case.
     *
     * @example
     *
     * ```ts
     * await page.keyboard.type('Hello'); // Types instantly
     * await page.keyboard.type('World', {delay: 100}); // Types slower, like a user
     * ```
     *
     * @param text - A text to type into a focused element.
     * @param options - An object of options. Accepts delay which,
     * if specified, is the time to wait between `keydown` and `keyup` in milliseconds.
     * Defaults to 0.
     */
    type(text: string, options?: Readonly<KeyboardTypeOptions>): Promise<void>;
    /**
     * Shortcut for {@link Keyboard.down}
     * and {@link Keyboard.up}.
     *
     * @remarks
     * If `key` is a single character and no modifier keys besides `Shift`
     * are being held down, a `keypress`/`input` event will also generated.
     * The `text` option can be specified to force an input event to be generated.
     *
     * Modifier keys DO effect {@link Keyboard.press}.
     * Holding down `Shift` will type the text in upper case.
     *
     * @param key - Name of key to press, such as `ArrowLeft`.
     * See {@link KeyInput} for a list of all key names.
     *
     * @param options - An object of options. Accepts text which, if specified,
     * generates an input event with this text. Accepts delay which,
     * if specified, is the time to wait between `keydown` and `keyup` in milliseconds.
     * Defaults to 0. Accepts commands which, if specified,
     * is the commands of keyboard shortcuts,
     * see {@link https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/editing/commands/editor_command_names.h | Chromium Source Code} for valid command names.
     */
    press(key: KeyInput, options?: Readonly<KeyPressOptions>): Promise<void>;
}

/**
 * @public
 */
export declare interface KeyboardTypeOptions {
    delay?: number;
}

/* Excluded from this release type: KeyDefinition */

/* Excluded from this release type: _keyDefinitions */

/**
 * @public
 */
export declare interface KeyDownOptions {
    /**
     * @deprecated Do not use. This is automatically handled.
     */
    text?: string;
    /**
     * @deprecated Do not use. This is automatically handled.
     */
    commands?: string[];
}

/**
 * All the valid keys that can be passed to functions that take user input, such
 * as {@link Keyboard.press | keyboard.press }
 *
 * @public
 */
export declare type KeyInput = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' | 'Power' | 'Eject' | 'Abort' | 'Help' | 'Backspace' | 'Tab' | 'Numpad5' | 'NumpadEnter' | 'Enter' | '\r' | '\n' | 'ShiftLeft' | 'ShiftRight' | 'ControlLeft' | 'ControlRight' | 'AltLeft' | 'AltRight' | 'Pause' | 'CapsLock' | 'Escape' | 'Convert' | 'NonConvert' | 'Space' | 'Numpad9' | 'PageUp' | 'Numpad3' | 'PageDown' | 'End' | 'Numpad1' | 'Home' | 'Numpad7' | 'ArrowLeft' | 'Numpad4' | 'Numpad8' | 'ArrowUp' | 'ArrowRight' | 'Numpad6' | 'Numpad2' | 'ArrowDown' | 'Select' | 'Open' | 'PrintScreen' | 'Insert' | 'Numpad0' | 'Delete' | 'NumpadDecimal' | 'Digit0' | 'Digit1' | 'Digit2' | 'Digit3' | 'Digit4' | 'Digit5' | 'Digit6' | 'Digit7' | 'Digit8' | 'Digit9' | 'KeyA' | 'KeyB' | 'KeyC' | 'KeyD' | 'KeyE' | 'KeyF' | 'KeyG' | 'KeyH' | 'KeyI' | 'KeyJ' | 'KeyK' | 'KeyL' | 'KeyM' | 'KeyN' | 'KeyO' | 'KeyP' | 'KeyQ' | 'KeyR' | 'KeyS' | 'KeyT' | 'KeyU' | 'KeyV' | 'KeyW' | 'KeyX' | 'KeyY' | 'KeyZ' | 'MetaLeft' | 'MetaRight' | 'ContextMenu' | 'NumpadMultiply' | 'NumpadAdd' | 'NumpadSubtract' | 'NumpadDivide' | 'F1' | 'F2' | 'F3' | 'F4' | 'F5' | 'F6' | 'F7' | 'F8' | 'F9' | 'F10' | 'F11' | 'F12' | 'F13' | 'F14' | 'F15' | 'F16' | 'F17' | 'F18' | 'F19' | 'F20' | 'F21' | 'F22' | 'F23' | 'F24' | 'NumLock' | 'ScrollLock' | 'AudioVolumeMute' | 'AudioVolumeDown' | 'AudioVolumeUp' | 'MediaTrackNext' | 'MediaTrackPrevious' | 'MediaStop' | 'MediaPlayPause' | 'Semicolon' | 'Equal' | 'NumpadEqual' | 'Comma' | 'Minus' | 'Period' | 'Slash' | 'Backquote' | 'BracketLeft' | 'Backslash' | 'BracketRight' | 'Quote' | 'AltGraph' | 'Props' | 'Cancel' | 'Clear' | 'Shift' | 'Control' | 'Alt' | 'Accept' | 'ModeChange' | ' ' | 'Print' | 'Execute' | '\u0000' | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j' | 'k' | 'l' | 'm' | 'n' | 'o' | 'p' | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z' | 'Meta' | '*' | '+' | '-' | '/' | ';' | '=' | ',' | '.' | '`' | '[' | '\\' | ']' | "'" | 'Attn' | 'CrSel' | 'ExSel' | 'EraseEof' | 'Play' | 'ZoomOut' | ')' | '!' | '@' | '#' | '$' | '%' | '^' | '&' | '(' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I' | 'J' | 'K' | 'L' | 'M' | 'N' | 'O' | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z' | ':' | '<' | '_' | '>' | '?' | '~' | '{' | '|' | '}' | '"' | 'SoftLeft' | 'SoftRight' | 'Camera' | 'Call' | 'EndCall' | 'VolumeDown' | 'VolumeUp';

/**
 * @public
 */
export declare type KeyPressOptions = KeyDownOptions & KeyboardTypeOptions;

/**
 * A list of devices to be used with {@link Page.emulate}.
 *
 * @example
 *
 * ```ts
 * import {KnownDevices} from 'puppeteer';
 * const iPhone = KnownDevices['iPhone 6'];
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.emulate(iPhone);
 *   await page.goto('https://www.google.com');
 *   // other actions...
 *   await browser.close();
 * })();
 * ```
 *
 * @public
 */
export declare const KnownDevices: Readonly<Record<"Blackberry PlayBook" | "Blackberry PlayBook landscape" | "BlackBerry Z30" | "BlackBerry Z30 landscape" | "Galaxy Note 3" | "Galaxy Note 3 landscape" | "Galaxy Note II" | "Galaxy Note II landscape" | "Galaxy S III" | "Galaxy S III landscape" | "Galaxy S5" | "Galaxy S5 landscape" | "Galaxy S8" | "Galaxy S8 landscape" | "Galaxy S9+" | "Galaxy S9+ landscape" | "Galaxy Tab S4" | "Galaxy Tab S4 landscape" | "iPad" | "iPad landscape" | "iPad (gen 6)" | "iPad (gen 6) landscape" | "iPad (gen 7)" | "iPad (gen 7) landscape" | "iPad Mini" | "iPad Mini landscape" | "iPad Pro" | "iPad Pro landscape" | "iPad Pro 11" | "iPad Pro 11 landscape" | "iPhone 4" | "iPhone 4 landscape" | "iPhone 5" | "iPhone 5 landscape" | "iPhone 6" | "iPhone 6 landscape" | "iPhone 6 Plus" | "iPhone 6 Plus landscape" | "iPhone 7" | "iPhone 7 landscape" | "iPhone 7 Plus" | "iPhone 7 Plus landscape" | "iPhone 8" | "iPhone 8 landscape" | "iPhone 8 Plus" | "iPhone 8 Plus landscape" | "iPhone SE" | "iPhone SE landscape" | "iPhone X" | "iPhone X landscape" | "iPhone XR" | "iPhone XR landscape" | "iPhone 11" | "iPhone 11 landscape" | "iPhone 11 Pro" | "iPhone 11 Pro landscape" | "iPhone 11 Pro Max" | "iPhone 11 Pro Max landscape" | "iPhone 12" | "iPhone 12 landscape" | "iPhone 12 Pro" | "iPhone 12 Pro landscape" | "iPhone 12 Pro Max" | "iPhone 12 Pro Max landscape" | "iPhone 12 Mini" | "iPhone 12 Mini landscape" | "iPhone 13" | "iPhone 13 landscape" | "iPhone 13 Pro" | "iPhone 13 Pro landscape" | "iPhone 13 Pro Max" | "iPhone 13 Pro Max landscape" | "iPhone 13 Mini" | "iPhone 13 Mini landscape" | "JioPhone 2" | "JioPhone 2 landscape" | "Kindle Fire HDX" | "Kindle Fire HDX landscape" | "LG Optimus L70" | "LG Optimus L70 landscape" | "Microsoft Lumia 550" | "Microsoft Lumia 950" | "Microsoft Lumia 950 landscape" | "Nexus 10" | "Nexus 10 landscape" | "Nexus 4" | "Nexus 4 landscape" | "Nexus 5" | "Nexus 5 landscape" | "Nexus 5X" | "Nexus 5X landscape" | "Nexus 6" | "Nexus 6 landscape" | "Nexus 6P" | "Nexus 6P landscape" | "Nexus 7" | "Nexus 7 landscape" | "Nokia Lumia 520" | "Nokia Lumia 520 landscape" | "Nokia N9" | "Nokia N9 landscape" | "Pixel 2" | "Pixel 2 landscape" | "Pixel 2 XL" | "Pixel 2 XL landscape" | "Pixel 3" | "Pixel 3 landscape" | "Pixel 4" | "Pixel 4 landscape" | "Pixel 4a (5G)" | "Pixel 4a (5G) landscape" | "Pixel 5" | "Pixel 5 landscape" | "Moto G4" | "Moto G4 landscape", Device>>;

declare type Last<Arr extends NonEmptyReadonlyArray<unknown>> = Arr extends [
infer Head,
...infer Tail
] ? Tail extends NonEmptyReadonlyArray<unknown> ? Last<Tail> : Head : never;

/**
 * @public
 */
export declare const 
/**
* @public
*/
/**
 * @public
 */
launch: (options?: PuppeteerLaunchOptions) => Promise<Browser>;

/**
 * Generic launch options that can be passed when launching any browser.
 * @public
 */
export declare interface LaunchOptions {
    /**
     * Chrome Release Channel
     */
    channel?: ChromeReleaseChannel;
    /**
     * Path to a browser executable to use instead of the bundled Chromium. Note
     * that Puppeteer is only guaranteed to work with the bundled Chromium, so use
     * this setting at your own risk.
     */
    executablePath?: string;
    /**
     * If `true`, do not use `puppeteer.defaultArgs()` when creating a browser. If
     * an array is provided, these args will be filtered out. Use this with care -
     * you probably want the default arguments Puppeteer uses.
     * @defaultValue `false`
     */
    ignoreDefaultArgs?: boolean | string[];
    /**
     * Close the browser process on `Ctrl+C`.
     * @defaultValue `true`
     */
    handleSIGINT?: boolean;
    /**
     * Close the browser process on `SIGTERM`.
     * @defaultValue `true`
     */
    handleSIGTERM?: boolean;
    /**
     * Close the browser process on `SIGHUP`.
     * @defaultValue `true`
     */
    handleSIGHUP?: boolean;
    /**
     * Maximum time in milliseconds to wait for the browser to start.
     * Pass `0` to disable the timeout.
     * @defaultValue `30_000` (30 seconds).
     */
    timeout?: number;
    /**
     * If true, pipes the browser process stdout and stderr to `process.stdout`
     * and `process.stderr`.
     * @defaultValue `false`
     */
    dumpio?: boolean;
    /**
     * Specify environment variables that will be visible to the browser.
     * @defaultValue The contents of `process.env`.
     */
    env?: Record<string, string | undefined>;
    /**
     * Connect to a browser over a pipe instead of a WebSocket.
     * @defaultValue `false`
     */
    pipe?: boolean;
    /**
     * Which browser to launch.
     * @defaultValue `chrome`
     */
    product?: Product;
    /**
     * {@link https://searchfox.org/mozilla-release/source/modules/libpref/init/all.js | Additional preferences } that can be passed when launching with Firefox.
     */
    extraPrefsFirefox?: Record<string, unknown>;
    /**
     * Whether to wait for the initial page to be ready.
     * Useful when a user explicitly disables that (e.g. `--no-startup-window` for Chrome).
     * @defaultValue `true`
     */
    waitForInitialPage?: boolean;
}

/* Excluded from this release type: LazyArg */

/* Excluded from this release type: LifecycleWatcher */

/**
 * Locators describe a strategy of locating objects and performing an action on
 * them. If the action fails because the object is not ready for the action, the
 * whole operation is retried. Various preconditions for a successful action are
 * checked automatically.
 *
 * @public
 */
export declare abstract class Locator<T> extends EventEmitter {
    #private;
    /**
     * Creates a race between multiple locators but ensures that only a single one
     * acts.
     *
     * @public
     */
    static race<Locators extends readonly unknown[] | []>(locators: Locators): Locator<AwaitedLocator<Locators[number]>>;
    /**
     * Used for nominally typing {@link Locator}.
     */
    _?: T;
    /* Excluded from this release type: visibility */
    /* Excluded from this release type: _timeout */
    /* Excluded from this release type: operators */
    get timeout(): number;
    on<K extends keyof LocatorEventObject>(eventName: K, handler: (event: LocatorEventObject[K]) => void): this;
    once<K extends keyof LocatorEventObject>(eventName: K, handler: (event: LocatorEventObject[K]) => void): this;
    off<K extends keyof LocatorEventObject>(eventName: K, handler: (event: LocatorEventObject[K]) => void): this;
    setTimeout(timeout: number): Locator<T>;
    setVisibility<NodeType extends Node>(this: Locator<NodeType>, visibility: VisibilityOption): Locator<NodeType>;
    setWaitForEnabled<NodeType extends Node>(this: Locator<NodeType>, value: boolean): Locator<NodeType>;
    setEnsureElementIsInTheViewport<ElementType extends Element>(this: Locator<ElementType>, value: boolean): Locator<ElementType>;
    setWaitForStableBoundingBox<ElementType extends Element>(this: Locator<ElementType>, value: boolean): Locator<ElementType>;
    /* Excluded from this release type: copyOptions */
    /* Excluded from this release type: _clone */
    /* Excluded from this release type: _wait */
    /**
     * Clones the locator.
     */
    clone(): Locator<T>;
    /**
     * Waits for the locator to get a handle from the page.
     *
     * @public
     */
    waitHandle(options?: Readonly<ActionOptions>): Promise<HandleFor<T>>;
    /**
     * Waits for the locator to get the serialized value from the page.
     *
     * Note this requires the value to be JSON-serializable.
     *
     * @public
     */
    wait(options?: Readonly<ActionOptions>): Promise<T>;
    /**
     * Maps the locator using the provided mapper.
     *
     * @public
     */
    map<To>(mapper: Mapper<T, To>): Locator<To>;
    /**
     * Creates an expectation that is evaluated against located values.
     *
     * If the expectations do not match, then the locator will retry.
     *
     * @public
     */
    filter<S extends T>(predicate: Predicate<T, S>): Locator<S>;
    /* Excluded from this release type: filterHandle */
    /* Excluded from this release type: mapHandle */
    click<ElementType extends Element>(this: Locator<ElementType>, options?: Readonly<LocatorClickOptions>): Promise<void>;
    /**
     * Fills out the input identified by the locator using the provided value. The
     * type of the input is determined at runtime and the appropriate fill-out
     * method is chosen based on the type. contenteditable, selector, inputs are
     * supported.
     */
    fill<ElementType extends Element>(this: Locator<ElementType>, value: string, options?: Readonly<ActionOptions>): Promise<void>;
    hover<ElementType extends Element>(this: Locator<ElementType>, options?: Readonly<ActionOptions>): Promise<void>;
    scroll<ElementType extends Element>(this: Locator<ElementType>, options?: Readonly<LocatorScrollOptions>): Promise<void>;
}

/**
 * @public
 */
export declare type LocatorClickOptions = ClickOptions & ActionOptions;

/**
 * All the events that a locator instance may emit.
 *
 * @public
 */
export declare enum LocatorEmittedEvents {
    /**
     * Emitted every time before the locator performs an action on the located element(s).
     */
    Action = "action"
}

/**
 * @public
 */
export declare interface LocatorEventObject {
    [LocatorEmittedEvents.Action]: never;
}

/**
 * @public
 */
export declare interface LocatorOptions {
    /**
     * Whether to wait for the element to be `visible` or `hidden`. `null` to
     * disable visibility checks.
     */
    visibility: VisibilityOption;
    /**
     * Total timeout for the entire locator operation.
     *
     * Pass `0` to disable timeout.
     *
     * @defaultValue `Page.getDefaultTimeout()`
     */
    timeout: number;
    /**
     * Whether to scroll the element into viewport if not in the viewprot already.
     * @defaultValue `true`
     */
    ensureElementIsInTheViewport: boolean;
    /**
     * Whether to wait for input elements to become enabled before the action.
     * Applicable to `click` and `fill` actions.
     * @defaultValue `true`
     */
    waitForEnabled: boolean;
    /**
     * Whether to wait for the element's bounding box to be same between two
     * animation frames.
     * @defaultValue `true`
     */
    waitForStableBoundingBox: boolean;
}

/**
 * @public
 */
export declare interface LocatorScrollOptions extends ActionOptions {
    scrollTop?: number;
    scrollLeft?: number;
}

/**
 * @public
 */
export declare type LowerCasePaperFormat = 'letter' | 'legal' | 'tabloid' | 'ledger' | 'a0' | 'a1' | 'a2' | 'a3' | 'a4' | 'a5' | 'a6';

/* Excluded from this release type: MAIN_WORLD */

/* Excluded from this release type: MappedLocator */

/**
 * @public
 */
export declare type Mapper<From, To> = (value: From) => Awaitable<To>;

/**
 * @public
 */
export declare interface MediaFeature {
    name: string;
    value: string;
}

/**
 * @public
 */
export declare interface Metrics {
    Timestamp?: number;
    Documents?: number;
    Frames?: number;
    JSEventListeners?: number;
    Nodes?: number;
    LayoutCount?: number;
    RecalcStyleCount?: number;
    LayoutDuration?: number;
    RecalcStyleDuration?: number;
    ScriptDuration?: number;
    TaskDuration?: number;
    JSHeapUsedSize?: number;
    JSHeapTotalSize?: number;
}

/**
 * The Mouse class operates in main-frame CSS pixels
 * relative to the top-left corner of the viewport.
 * @remarks
 * Every `page` object has its own Mouse, accessible with [`page.mouse`](#pagemouse).
 *
 * @example
 *
 * ```ts
 * // Using ‘page.mouse’ to trace a 100x100 square.
 * await page.mouse.move(0, 0);
 * await page.mouse.down();
 * await page.mouse.move(0, 100);
 * await page.mouse.move(100, 100);
 * await page.mouse.move(100, 0);
 * await page.mouse.move(0, 0);
 * await page.mouse.up();
 * ```
 *
 * **Note**: The mouse events trigger synthetic `MouseEvent`s.
 * This means that it does not fully replicate the functionality of what a normal user
 * would be able to do with their mouse.
 *
 * For example, dragging and selecting text is not possible using `page.mouse`.
 * Instead, you can use the {@link https://developer.mozilla.org/en-US/docs/Web/API/DocumentOrShadowRoot/getSelection | `DocumentOrShadowRoot.getSelection()`} functionality implemented in the platform.
 *
 * @example
 * For example, if you want to select all content between nodes:
 *
 * ```ts
 * await page.evaluate(
 *   (from, to) => {
 *     const selection = from.getRootNode().getSelection();
 *     const range = document.createRange();
 *     range.setStartBefore(from);
 *     range.setEndAfter(to);
 *     selection.removeAllRanges();
 *     selection.addRange(range);
 *   },
 *   fromJSHandle,
 *   toJSHandle
 * );
 * ```
 *
 * If you then would want to copy-paste your selection, you can use the clipboard api:
 *
 * ```ts
 * // The clipboard api does not allow you to copy, unless the tab is focused.
 * await page.bringToFront();
 * await page.evaluate(() => {
 *   // Copy the selected content to the clipboard
 *   document.execCommand('copy');
 *   // Obtain the content of the clipboard as a string
 *   return navigator.clipboard.readText();
 * });
 * ```
 *
 * **Note**: If you want access to the clipboard API,
 * you have to give it permission to do so:
 *
 * ```ts
 * await browser
 *   .defaultBrowserContext()
 *   .overridePermissions('<your origin>', [
 *     'clipboard-read',
 *     'clipboard-write',
 *   ]);
 * ```
 *
 * @public
 */
export declare class Mouse {
    /* Excluded from this release type: __constructor */
    /**
     * Resets the mouse to the default state: No buttons pressed; position at
     * (0,0).
     */
    reset(): Promise<void>;
    /**
     * Moves the mouse to the given coordinate.
     *
     * @param x - Horizontal position of the mouse.
     * @param y - Vertical position of the mouse.
     * @param options - Options to configure behavior.
     */
    move(x: number, y: number, options?: Readonly<MouseMoveOptions>): Promise<void>;
    /**
     * Presses the mouse.
     *
     * @param options - Options to configure behavior.
     */
    down(options?: Readonly<MouseOptions>): Promise<void>;
    /**
     * Releases the mouse.
     *
     * @param options - Options to configure behavior.
     */
    up(options?: Readonly<MouseOptions>): Promise<void>;
    /**
     * Shortcut for `mouse.move`, `mouse.down` and `mouse.up`.
     *
     * @param x - Horizontal position of the mouse.
     * @param y - Vertical position of the mouse.
     * @param options - Options to configure behavior.
     */
    click(x: number, y: number, options?: Readonly<MouseClickOptions>): Promise<void>;
    /**
     * Dispatches a `mousewheel` event.
     * @param options - Optional: `MouseWheelOptions`.
     *
     * @example
     * An example of zooming into an element:
     *
     * ```ts
     * await page.goto(
     *   'https://mdn.mozillademos.org/en-US/docs/Web/API/Element/wheel_event$samples/Scaling_an_element_via_the_wheel?revision=1587366'
     * );
     *
     * const elem = await page.$('div');
     * const boundingBox = await elem.boundingBox();
     * await page.mouse.move(
     *   boundingBox.x + boundingBox.width / 2,
     *   boundingBox.y + boundingBox.height / 2
     * );
     *
     * await page.mouse.wheel({deltaY: -100});
     * ```
     */
    wheel(options?: Readonly<MouseWheelOptions>): Promise<void>;
    /**
     * Dispatches a `drag` event.
     * @param start - starting point for drag
     * @param target - point to drag to
     */
    drag(start: Point, target: Point): Promise<Protocol.Input.DragData>;
    /**
     * Dispatches a `dragenter` event.
     * @param target - point for emitting `dragenter` event
     * @param data - drag data containing items and operations mask
     */
    dragEnter(target: Point, data: Protocol.Input.DragData): Promise<void>;
    /**
     * Dispatches a `dragover` event.
     * @param target - point for emitting `dragover` event
     * @param data - drag data containing items and operations mask
     */
    dragOver(target: Point, data: Protocol.Input.DragData): Promise<void>;
    /**
     * Performs a dragenter, dragover, and drop in sequence.
     * @param target - point to drop on
     * @param data - drag data containing items and operations mask
     */
    drop(target: Point, data: Protocol.Input.DragData): Promise<void>;
    /**
     * Performs a drag, dragenter, dragover, and drop in sequence.
     * @param start - point to drag from
     * @param target - point to drop on
     * @param options - An object of options. Accepts delay which,
     * if specified, is the time to wait between `dragover` and `drop` in milliseconds.
     * Defaults to 0.
     */
    dragAndDrop(start: Point, target: Point, options?: {
        delay?: number;
    }): Promise<void>;
}

/**
 * Enum of valid mouse buttons.
 *
 * @public
 */
export declare const MouseButton: Readonly<{
    Left: "left";
    Right: "right";
    Middle: "middle";
    Back: "back";
    Forward: "forward";
}>;

/**
 * @public
 */
export declare type MouseButton = (typeof MouseButton)[keyof typeof MouseButton];

/**
 * @public
 */
export declare interface MouseClickOptions extends MouseOptions {
    /**
     * Time (in ms) to delay the mouse release after the mouse press.
     */
    delay?: number;
    /**
     * Number of clicks to perform.
     *
     * @defaultValue `1`
     */
    count?: number;
}

/**
 * @public
 */
export declare interface MouseMoveOptions {
    /**
     * Determines the number of movements to make from the current mouse position
     * to the new one.
     *
     * @defaultValue `1`
     */
    steps?: number;
}

/**
 * @public
 */
export declare interface MouseOptions {
    /**
     * Determines which button will be pressed.
     *
     * @defaultValue `'left'`
     */
    button?: MouseButton;
    /**
     * @deprecated Use {@link MouseClickOptions.count}.
     *
     * Determines the click count for the mouse event. This does not perform
     * multiple clicks.
     *
     * @defaultValue `1`
     */
    clickCount?: number;
}

/**
 * @public
 */
export declare interface MouseWheelOptions {
    deltaX?: number;
    deltaY?: number;
}

/* Excluded from this release type: MutationPoller */

/**
 * @public
 */
export declare interface NetworkConditions {
    download: number;
    upload: number;
    latency: number;
}

/**
 * @deprecated Import {@link PredefinedNetworkConditions}.
 *
 * @public
 */
export declare const networkConditions: Readonly<{
    'Slow 3G': NetworkConditions;
    'Fast 3G': NetworkConditions;
}>;

/* Excluded from this release type: NetworkEventManager */

/* Excluded from this release type: NetworkManager */

/* Excluded from this release type: NetworkManagerEmittedEvents */

/* Excluded from this release type: NetworkRequestId */

/**
 * @public
 */
export declare interface NewDocumentScriptEvaluation {
    identifier: string;
}

/**
 * @public
 */
export declare type NodeFor<ComplexSelector extends string> = TypeSelectorOfComplexSelector<ComplexSelector> extends infer TypeSelector ? TypeSelector extends keyof HTMLElementTagNameMap | keyof SVGElementTagNameMap ? ElementFor<TypeSelector> : Element : never;

/* Excluded from this release type: NodeLocator */

/* Excluded from this release type: NodeWebSocketTransport */

declare type NonEmptyReadonlyArray<T> = [T, ...(readonly T[])];

/**
 * @public
 */
export declare interface Offset {
    /**
     * x-offset for the clickable point relative to the top-left corner of the border box.
     */
    x: number;
    /**
     * y-offset for the clickable point relative to the top-left corner of the border box.
     */
    y: number;
}

/* Excluded from this release type: OtherTarget */

/**
 * Page provides methods to interact with a single tab or
 * {@link https://developer.chrome.com/extensions/background_pages | extension background page}
 * in the browser.
 *
 * :::note
 *
 * One Browser instance might have multiple Page instances.
 *
 * :::
 *
 * @example
 * This example creates a page, navigates it to a URL, and then saves a screenshot:
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.goto('https://example.com');
 *   await page.screenshot({path: 'screenshot.png'});
 *   await browser.close();
 * })();
 * ```
 *
 * The Page class extends from Puppeteer's {@link EventEmitter} class and will
 * emit various events which are documented in the {@link PageEmittedEvents} enum.
 *
 * @example
 * This example logs a message for a single page `load` event:
 *
 * ```ts
 * page.once('load', () => console.log('Page loaded!'));
 * ```
 *
 * To unsubscribe from events use the {@link Page.off} method:
 *
 * ```ts
 * function logRequest(interceptedRequest) {
 *   console.log('A request was made:', interceptedRequest.url());
 * }
 * page.on('request', logRequest);
 * // Sometime later...
 * page.off('request', logRequest);
 * ```
 *
 * @public
 */
export declare class Page extends EventEmitter {
    #private;
    /* Excluded from this release type: __constructor */
    /**
     * `true` if the service worker are being bypassed, `false` otherwise.
     */
    isServiceWorkerBypassed(): boolean;
    /**
     * `true` if drag events are being intercepted, `false` otherwise.
     */
    isDragInterceptionEnabled(): boolean;
    /**
     * `true` if the page has JavaScript enabled, `false` otherwise.
     */
    isJavaScriptEnabled(): boolean;
    /**
     * Listen to page events.
     *
     * :::note
     *
     * This method exists to define event typings and handle proper wireup of
     * cooperative request interception. Actual event listening and dispatching is
     * delegated to {@link EventEmitter}.
     *
     * :::
     */
    on<K extends keyof PageEventObject>(eventName: K, handler: (event: PageEventObject[K]) => void): this;
    once<K extends keyof PageEventObject>(eventName: K, handler: (event: PageEventObject[K]) => void): this;
    off<K extends keyof PageEventObject>(eventName: K, handler: (event: PageEventObject[K]) => void): this;
    /**
     * This method is typically coupled with an action that triggers file
     * choosing.
     *
     * :::caution
     *
     * This must be called before the file chooser is launched. It will not return
     * a currently active file chooser.
     *
     * :::
     *
     * @remarks
     * In the "headful" browser, this method results in the native file picker
     * dialog `not showing up` for the user.
     *
     * @example
     * The following example clicks a button that issues a file chooser
     * and then responds with `/tmp/myfile.pdf` as if a user has selected this file.
     *
     * ```ts
     * const [fileChooser] = await Promise.all([
     *   page.waitForFileChooser(),
     *   page.click('#upload-file-button'),
     *   // some button that triggers file selection
     * ]);
     * await fileChooser.accept(['/tmp/myfile.pdf']);
     * ```
     */
    waitForFileChooser(options?: WaitTimeoutOptions): Promise<FileChooser>;
    /**
     * Sets the page's geolocation.
     *
     * @remarks
     * Consider using {@link BrowserContext.overridePermissions} to grant
     * permissions for the page to read its geolocation.
     *
     * @example
     *
     * ```ts
     * await page.setGeolocation({latitude: 59.95, longitude: 30.31667});
     * ```
     */
    setGeolocation(options: GeolocationOptions): Promise<void>;
    /**
     * A target this page was created from.
     */
    target(): Target;
    /**
     * Get the browser the page belongs to.
     */
    browser(): Browser;
    /**
     * Get the browser context that the page belongs to.
     */
    browserContext(): BrowserContext;
    /**
     * The page's main frame.
     *
     * @remarks
     * Page is guaranteed to have a main frame which persists during navigations.
     */
    mainFrame(): Frame;
    /**
     * Creates a Chrome Devtools Protocol session attached to the page.
     */
    createCDPSession(): Promise<CDPSession>;
    /**
     * {@inheritDoc Keyboard}
     */
    get keyboard(): Keyboard;
    /**
     * {@inheritDoc Touchscreen}
     */
    get touchscreen(): Touchscreen;
    /**
     * {@inheritDoc Coverage}
     */
    get coverage(): Coverage;
    /**
     * {@inheritDoc Tracing}
     */
    get tracing(): Tracing;
    /**
     * {@inheritDoc Accessibility}
     */
    get accessibility(): Accessibility;
    /**
     * An array of all frames attached to the page.
     */
    frames(): Frame[];
    /**
     * All of the dedicated {@link
     * https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API |
     * WebWorkers} associated with the page.
     *
     * @remarks
     * This does not contain ServiceWorkers
     */
    workers(): WebWorker[];
    /**
     * Activating request interception enables {@link HTTPRequest.abort},
     * {@link HTTPRequest.continue} and {@link HTTPRequest.respond} methods. This
     * provides the capability to modify network requests that are made by a page.
     *
     * Once request interception is enabled, every request will stall unless it's
     * continued, responded or aborted; or completed using the browser cache.
     *
     * See the
     * {@link https://pptr.dev/next/guides/request-interception|Request interception guide}
     * for more details.
     *
     * @example
     * An example of a naïve request interceptor that aborts all image requests:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   await page.setRequestInterception(true);
     *   page.on('request', interceptedRequest => {
     *     if (
     *       interceptedRequest.url().endsWith('.png') ||
     *       interceptedRequest.url().endsWith('.jpg')
     *     )
     *       interceptedRequest.abort();
     *     else interceptedRequest.continue();
     *   });
     *   await page.goto('https://example.com');
     *   await browser.close();
     * })();
     * ```
     *
     * @param value - Whether to enable request interception.
     */
    setRequestInterception(value: boolean): Promise<void>;
    /**
     * Toggles ignoring of service worker for each request.
     *
     * @param bypass - Whether to bypass service worker and load from network.
     */
    setBypassServiceWorker(bypass: boolean): Promise<void>;
    /**
     * @param enabled - Whether to enable drag interception.
     *
     * @remarks
     * Activating drag interception enables the `Input.drag`,
     * methods This provides the capability to capture drag events emitted
     * on the page, which can then be used to simulate drag-and-drop.
     */
    setDragInterception(enabled: boolean): Promise<void>;
    /**
     * Sets the network connection to offline.
     *
     * It does not change the parameters used in {@link Page.emulateNetworkConditions}
     *
     * @param enabled - When `true`, enables offline mode for the page.
     */
    setOfflineMode(enabled: boolean): Promise<void>;
    /**
     * This does not affect WebSockets and WebRTC PeerConnections (see
     * https://crbug.com/563644). To set the page offline, you can use
     * {@link Page.setOfflineMode}.
     *
     * A list of predefined network conditions can be used by importing
     * {@link PredefinedNetworkConditions}.
     *
     * @example
     *
     * ```ts
     * import {PredefinedNetworkConditions} from 'puppeteer';
     * const slow3G = PredefinedNetworkConditions['Slow 3G'];
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   await page.emulateNetworkConditions(slow3G);
     *   await page.goto('https://www.google.com');
     *   // other actions...
     *   await browser.close();
     * })();
     * ```
     *
     * @param networkConditions - Passing `null` disables network condition
     * emulation.
     */
    emulateNetworkConditions(networkConditions: NetworkConditions | null): Promise<void>;
    /**
     * This setting will change the default maximum navigation time for the
     * following methods and related shortcuts:
     *
     * - {@link Page.goBack | page.goBack(options)}
     *
     * - {@link Page.goForward | page.goForward(options)}
     *
     * - {@link Page.goto | page.goto(url,options)}
     *
     * - {@link Page.reload | page.reload(options)}
     *
     * - {@link Page.setContent | page.setContent(html,options)}
     *
     * - {@link Page.waitForNavigation | page.waitForNavigation(options)}
     *   @param timeout - Maximum navigation time in milliseconds.
     */
    setDefaultNavigationTimeout(timeout: number): void;
    /**
     * @param timeout - Maximum time in milliseconds.
     */
    setDefaultTimeout(timeout: number): void;
    /**
     * Maximum time in milliseconds.
     */
    getDefaultTimeout(): number;
    /**
     * Creates a locator for the provided selector. See {@link Locator} for
     * details and supported actions.
     *
     * @remarks
     * Locators API is experimental and we will not follow semver for breaking
     * change in the Locators API.
     */
    locator<Selector extends string>(selector: Selector): Locator<NodeFor<Selector>>;
    /**
     * Creates a locator for the provided function. See {@link Locator} for
     * details and supported actions.
     *
     * @remarks
     * Locators API is experimental and we will not follow semver for breaking
     * change in the Locators API.
     */
    locator<Ret>(func: () => Awaitable<Ret>): Locator<Ret>;
    /* Excluded from this release type: locatorRace */
    /**
     * Runs `document.querySelector` within the page. If no element matches the
     * selector, the return value resolves to `null`.
     *
     * @param selector - A `selector` to query page for
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * to query page for.
     */
    $<Selector extends string>(selector: Selector): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * The method runs `document.querySelectorAll` within the page. If no elements
     * match the selector, the return value resolves to `[]`.
     * @remarks
     * Shortcut for {@link Frame.$$ | Page.mainFrame().$$(selector) }.
     * @param selector - A `selector` to query page for
     */
    $$<Selector extends string>(selector: Selector): Promise<Array<ElementHandle<NodeFor<Selector>>>>;
    /**
     * @remarks
     *
     * The only difference between {@link Page.evaluate | page.evaluate} and
     * `page.evaluateHandle` is that `evaluateHandle` will return the value
     * wrapped in an in-page object.
     *
     * If the function passed to `page.evaluateHandle` returns a Promise, the
     * function will wait for the promise to resolve and return its value.
     *
     * You can pass a string instead of a function (although functions are
     * recommended as they are easier to debug and use with TypeScript):
     *
     * @example
     *
     * ```ts
     * const aHandle = await page.evaluateHandle('document');
     * ```
     *
     * @example
     * {@link JSHandle} instances can be passed as arguments to the `pageFunction`:
     *
     * ```ts
     * const aHandle = await page.evaluateHandle(() => document.body);
     * const resultHandle = await page.evaluateHandle(
     *   body => body.innerHTML,
     *   aHandle
     * );
     * console.log(await resultHandle.jsonValue());
     * await resultHandle.dispose();
     * ```
     *
     * Most of the time this function returns a {@link JSHandle},
     * but if `pageFunction` returns a reference to an element,
     * you instead get an {@link ElementHandle} back:
     *
     * @example
     *
     * ```ts
     * const button = await page.evaluateHandle(() =>
     *   document.querySelector('button')
     * );
     * // can call `click` because `button` is an `ElementHandle`
     * await button.click();
     * ```
     *
     * The TypeScript definitions assume that `evaluateHandle` returns
     * a `JSHandle`, but if you know it's going to return an
     * `ElementHandle`, pass it as the generic argument:
     *
     * ```ts
     * const button = await page.evaluateHandle<ElementHandle>(...);
     * ```
     *
     * @param pageFunction - a function that is run within the page
     * @param args - arguments to be passed to the pageFunction
     */
    evaluateHandle<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
    /**
     * This method iterates the JavaScript heap and finds all objects with the
     * given prototype.
     *
     * @example
     *
     * ```ts
     * // Create a Map object
     * await page.evaluate(() => (window.map = new Map()));
     * // Get a handle to the Map object prototype
     * const mapPrototype = await page.evaluateHandle(() => Map.prototype);
     * // Query all map instances into an array
     * const mapInstances = await page.queryObjects(mapPrototype);
     * // Count amount of map objects in heap
     * const count = await page.evaluate(maps => maps.length, mapInstances);
     * await mapInstances.dispose();
     * await mapPrototype.dispose();
     * ```
     *
     * @param prototypeHandle - a handle to the object prototype.
     * @returns Promise which resolves to a handle to an array of objects with
     * this prototype.
     */
    queryObjects<Prototype>(prototypeHandle: JSHandle<Prototype>): Promise<JSHandle<Prototype[]>>;
    /**
     * This method runs `document.querySelector` within the page and passes the
     * result as the first argument to the `pageFunction`.
     *
     * @remarks
     *
     * If no element is found matching `selector`, the method will throw an error.
     *
     * If `pageFunction` returns a promise `$eval` will wait for the promise to
     * resolve and then return its value.
     *
     * @example
     *
     * ```ts
     * const searchValue = await page.$eval('#search', el => el.value);
     * const preloadHref = await page.$eval('link[rel=preload]', el => el.href);
     * const html = await page.$eval('.main-container', el => el.outerHTML);
     * ```
     *
     * If you are using TypeScript, you may have to provide an explicit type to the
     * first argument of the `pageFunction`.
     * By default it is typed as `Element`, but you may need to provide a more
     * specific sub-type:
     *
     * @example
     *
     * ```ts
     * // if you don't provide HTMLInputElement here, TS will error
     * // as `value` is not on `Element`
     * const searchValue = await page.$eval(
     *   '#search',
     *   (el: HTMLInputElement) => el.value
     * );
     * ```
     *
     * The compiler should be able to infer the return type
     * from the `pageFunction` you provide. If it is unable to, you can use the generic
     * type to tell the compiler what return type you expect from `$eval`:
     *
     * @example
     *
     * ```ts
     * // The compiler can infer the return type in this case, but if it can't
     * // or if you want to be more explicit, provide it as the generic type.
     * const searchValue = await page.$eval<string>(
     *   '#search',
     *   (el: HTMLInputElement) => el.value
     * );
     * ```
     *
     * @param selector - the
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * to query for
     * @param pageFunction - the function to be evaluated in the page context.
     * Will be passed the result of `document.querySelector(selector)` as its
     * first argument.
     * @param args - any additional arguments to pass through to `pageFunction`.
     *
     * @returns The result of calling `pageFunction`. If it returns an element it
     * is wrapped in an {@link ElementHandle}, else the raw value itself is
     * returned.
     */
    $eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<NodeFor<Selector>, Params> = EvaluateFuncWith<NodeFor<Selector>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * This method runs `Array.from(document.querySelectorAll(selector))` within
     * the page and passes the result as the first argument to the `pageFunction`.
     *
     * @remarks
     * If `pageFunction` returns a promise `$$eval` will wait for the promise to
     * resolve and then return its value.
     *
     * @example
     *
     * ```ts
     * // get the amount of divs on the page
     * const divCount = await page.$$eval('div', divs => divs.length);
     *
     * // get the text content of all the `.options` elements:
     * const options = await page.$$eval('div > span.options', options => {
     *   return options.map(option => option.textContent);
     * });
     * ```
     *
     * If you are using TypeScript, you may have to provide an explicit type to the
     * first argument of the `pageFunction`.
     * By default it is typed as `Element[]`, but you may need to provide a more
     * specific sub-type:
     *
     * @example
     *
     * ```ts
     * // if you don't provide HTMLInputElement here, TS will error
     * // as `value` is not on `Element`
     * await page.$$eval('input', (elements: HTMLInputElement[]) => {
     *   return elements.map(e => e.value);
     * });
     * ```
     *
     * The compiler should be able to infer the return type
     * from the `pageFunction` you provide. If it is unable to, you can use the generic
     * type to tell the compiler what return type you expect from `$$eval`:
     *
     * @example
     *
     * ```ts
     * // The compiler can infer the return type in this case, but if it can't
     * // or if you want to be more explicit, provide it as the generic type.
     * const allInputValues = await page.$$eval<string[]>(
     *   'input',
     *   (elements: HTMLInputElement[]) => elements.map(e => e.textContent)
     * );
     * ```
     *
     * @param selector - the
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * to query for
     * @param pageFunction - the function to be evaluated in the page context.
     * Will be passed the result of
     * `Array.from(document.querySelectorAll(selector))` as its first argument.
     * @param args - any additional arguments to pass through to `pageFunction`.
     *
     * @returns The result of calling `pageFunction`. If it returns an element it
     * is wrapped in an {@link ElementHandle}, else the raw value itself is
     * returned.
     */
    $$eval<Selector extends string, Params extends unknown[], Func extends EvaluateFuncWith<Array<NodeFor<Selector>>, Params> = EvaluateFuncWith<Array<NodeFor<Selector>>, Params>>(selector: Selector, pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * The method evaluates the XPath expression relative to the page document as
     * its context node. If there are no such elements, the method resolves to an
     * empty array.
     *
     * @remarks
     * Shortcut for {@link Frame.$x | Page.mainFrame().$x(expression) }.
     *
     * @param expression - Expression to evaluate
     */
    $x(expression: string): Promise<Array<ElementHandle<Node>>>;
    /**
     * If no URLs are specified, this method returns cookies for the current page
     * URL. If URLs are specified, only cookies for those URLs are returned.
     */
    cookies(...urls: string[]): Promise<Protocol.Network.Cookie[]>;
    deleteCookie(...cookies: Protocol.Network.DeleteCookiesRequest[]): Promise<void>;
    /**
     * @example
     *
     * ```ts
     * await page.setCookie(cookieObject1, cookieObject2);
     * ```
     */
    setCookie(...cookies: Protocol.Network.CookieParam[]): Promise<void>;
    /**
     * Adds a `<script>` tag into the page with the desired URL or content.
     *
     * @remarks
     * Shortcut for
     * {@link Frame.addScriptTag | page.mainFrame().addScriptTag(options)}.
     *
     * @param options - Options for the script.
     * @returns An {@link ElementHandle | element handle} to the injected
     * `<script>` element.
     */
    addScriptTag(options: FrameAddScriptTagOptions): Promise<ElementHandle<HTMLScriptElement>>;
    /**
     * Adds a `<link rel="stylesheet">` tag into the page with the desired URL or
     * a `<style type="text/css">` tag with the content.
     *
     * Shortcut for
     * {@link Frame.(addStyleTag:2) | page.mainFrame().addStyleTag(options)}.
     *
     * @returns An {@link ElementHandle | element handle} to the injected `<link>`
     * or `<style>` element.
     */
    addStyleTag(options: Omit<FrameAddStyleTagOptions, 'url'>): Promise<ElementHandle<HTMLStyleElement>>;
    addStyleTag(options: FrameAddStyleTagOptions): Promise<ElementHandle<HTMLLinkElement>>;
    /**
     * The method adds a function called `name` on the page's `window` object.
     * When called, the function executes `puppeteerFunction` in node.js and
     * returns a `Promise` which resolves to the return value of
     * `puppeteerFunction`.
     *
     * If the puppeteerFunction returns a `Promise`, it will be awaited.
     *
     * :::note
     *
     * Functions installed via `page.exposeFunction` survive navigations.
     *
     * :::note
     *
     * @example
     * An example of adding an `md5` function into the page:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * import crypto from 'crypto';
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   page.on('console', msg => console.log(msg.text()));
     *   await page.exposeFunction('md5', text =>
     *     crypto.createHash('md5').update(text).digest('hex')
     *   );
     *   await page.evaluate(async () => {
     *     // use window.md5 to compute hashes
     *     const myString = 'PUPPETEER';
     *     const myHash = await window.md5(myString);
     *     console.log(`md5 of ${myString} is ${myHash}`);
     *   });
     *   await browser.close();
     * })();
     * ```
     *
     * @example
     * An example of adding a `window.readfile` function into the page:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * import fs from 'fs';
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   page.on('console', msg => console.log(msg.text()));
     *   await page.exposeFunction('readfile', async filePath => {
     *     return new Promise((resolve, reject) => {
     *       fs.readFile(filePath, 'utf8', (err, text) => {
     *         if (err) reject(err);
     *         else resolve(text);
     *       });
     *     });
     *   });
     *   await page.evaluate(async () => {
     *     // use window.readfile to read contents of a file
     *     const content = await window.readfile('/etc/hosts');
     *     console.log(content);
     *   });
     *   await browser.close();
     * })();
     * ```
     *
     * @param name - Name of the function on the window object
     * @param pptrFunction - Callback function which will be called in Puppeteer's
     * context.
     */
    exposeFunction(name: string, pptrFunction: Function | {
        default: Function;
    }): Promise<void>;
    /**
     * The method removes a previously added function via ${@link Page.exposeFunction}
     * called `name` from the page's `window` object.
     */
    removeExposedFunction(name: string): Promise<void>;
    /**
     * Provide credentials for `HTTP authentication`.
     *
     * @remarks
     * To disable authentication, pass `null`.
     */
    authenticate(credentials: Credentials): Promise<void>;
    /**
     * The extra HTTP headers will be sent with every request the page initiates.
     *
     * :::tip
     *
     * All HTTP header names are lowercased. (HTTP headers are
     * case-insensitive, so this shouldn’t impact your server code.)
     *
     * :::
     *
     * :::note
     *
     * page.setExtraHTTPHeaders does not guarantee the order of headers in
     * the outgoing requests.
     *
     * :::
     *
     * @param headers - An object containing additional HTTP headers to be sent
     * with every request. All header values must be strings.
     */
    setExtraHTTPHeaders(headers: Record<string, string>): Promise<void>;
    /**
     * @param userAgent - Specific user agent to use in this page
     * @param userAgentData - Specific user agent client hint data to use in this
     * page
     * @returns Promise which resolves when the user agent is set.
     */
    setUserAgent(userAgent: string, userAgentMetadata?: Protocol.Emulation.UserAgentMetadata): Promise<void>;
    /**
     * Object containing metrics as key/value pairs.
     *
     * @returns
     *
     * - `Timestamp` : The timestamp when the metrics sample was taken.
     *
     * - `Documents` : Number of documents in the page.
     *
     * - `Frames` : Number of frames in the page.
     *
     * - `JSEventListeners` : Number of events in the page.
     *
     * - `Nodes` : Number of DOM nodes in the page.
     *
     * - `LayoutCount` : Total number of full or partial page layout.
     *
     * - `RecalcStyleCount` : Total number of page style recalculations.
     *
     * - `LayoutDuration` : Combined durations of all page layouts.
     *
     * - `RecalcStyleDuration` : Combined duration of all page style
     *   recalculations.
     *
     * - `ScriptDuration` : Combined duration of JavaScript execution.
     *
     * - `TaskDuration` : Combined duration of all tasks performed by the browser.
     *
     * - `JSHeapUsedSize` : Used JavaScript heap size.
     *
     * - `JSHeapTotalSize` : Total JavaScript heap size.
     *
     * @remarks
     * All timestamps are in monotonic time: monotonically increasing time
     * in seconds since an arbitrary point in the past.
     */
    metrics(): Promise<Metrics>;
    /**
     * The page's URL.
     * @remarks Shortcut for
     * {@link Frame.url | page.mainFrame().url()}.
     */
    url(): string;
    /**
     * The full HTML contents of the page, including the DOCTYPE.
     */
    content(): Promise<string>;
    /**
     * Set the content of the page.
     *
     * @param html - HTML markup to assign to the page.
     * @param options - Parameters that has some properties.
     * @remarks
     * The parameter `options` might have the following options.
     *
     * - `timeout` : Maximum time in milliseconds for resources to load, defaults
     *   to 30 seconds, pass `0` to disable timeout. The default value can be
     *   changed by using the {@link Page.setDefaultNavigationTimeout} or
     *   {@link Page.setDefaultTimeout} methods.
     *
     * - `waitUntil`: When to consider setting markup succeeded, defaults to
     *   `load`. Given an array of event strings, setting content is considered
     *   to be successful after all events have been fired. Events can be
     *   either:<br/>
     * - `load` : consider setting content to be finished when the `load` event
     *   is fired.<br/>
     * - `domcontentloaded` : consider setting content to be finished when the
     *   `DOMContentLoaded` event is fired.<br/>
     * - `networkidle0` : consider setting content to be finished when there are
     *   no more than 0 network connections for at least `500` ms.<br/>
     * - `networkidle2` : consider setting content to be finished when there are
     *   no more than 2 network connections for at least `500` ms.
     */
    setContent(html: string, options?: WaitForOptions): Promise<void>;
    /**
     * @param url - URL to navigate page to. The URL should include scheme, e.g.
     * `https://`
     * @param options - Navigation Parameter
     * @returns Promise which resolves to the main resource response. In case of
     * multiple redirects, the navigation will resolve with the response of the
     * last redirect.
     * @remarks
     * The argument `options` might have the following properties:
     *
     * - `timeout` : Maximum navigation time in milliseconds, defaults to 30
     *   seconds, pass 0 to disable timeout. The default value can be changed by
     *   using the {@link Page.setDefaultNavigationTimeout} or
     *   {@link Page.setDefaultTimeout} methods.
     *
     * - `waitUntil`:When to consider navigation succeeded, defaults to `load`.
     *   Given an array of event strings, navigation is considered to be
     *   successful after all events have been fired. Events can be either:<br/>
     * - `load` : consider navigation to be finished when the load event is
     *   fired.<br/>
     * - `domcontentloaded` : consider navigation to be finished when the
     *   DOMContentLoaded event is fired.<br/>
     * - `networkidle0` : consider navigation to be finished when there are no
     *   more than 0 network connections for at least `500` ms.<br/>
     * - `networkidle2` : consider navigation to be finished when there are no
     *   more than 2 network connections for at least `500` ms.
     *
     * - `referer` : Referer header value. If provided it will take preference
     *   over the referer header value set by
     *   {@link Page.setExtraHTTPHeaders |page.setExtraHTTPHeaders()}.<br/>
     * - `referrerPolicy` : ReferrerPolicy. If provided it will take preference
     *   over the referer-policy header value set by
     *   {@link Page.setExtraHTTPHeaders |page.setExtraHTTPHeaders()}.
     *
     * `page.goto` will throw an error if:
     *
     * - there's an SSL error (e.g. in case of self-signed certificates).
     * - target URL is invalid.
     * - the timeout is exceeded during navigation.
     * - the remote server does not respond or is unreachable.
     * - the main resource failed to load.
     *
     * `page.goto` will not throw an error when any valid HTTP status code is
     * returned by the remote server, including 404 "Not Found" and 500
     * "Internal Server Error". The status code for such responses can be
     * retrieved by calling response.status().
     *
     * NOTE: `page.goto` either throws an error or returns a main resource
     * response. The only exceptions are navigation to about:blank or navigation
     * to the same URL with a different hash, which would succeed and return null.
     *
     * NOTE: Headless mode doesn't support navigation to a PDF document. See the
     * {@link https://bugs.chromium.org/p/chromium/issues/detail?id=761295 |
     * upstream issue}.
     *
     * Shortcut for {@link Frame.goto | page.mainFrame().goto(url, options)}.
     */
    goto(url: string, options?: WaitForOptions & {
        referer?: string;
        referrerPolicy?: string;
    }): Promise<HTTPResponse | null>;
    /**
     * @param options - Navigation parameters which might have the following
     * properties:
     * @returns Promise which resolves to the main resource response. In case of
     * multiple redirects, the navigation will resolve with the response of the
     * last redirect.
     * @remarks
     * The argument `options` might have the following properties:
     *
     * - `timeout` : Maximum navigation time in milliseconds, defaults to 30
     *   seconds, pass 0 to disable timeout. The default value can be changed by
     *   using the {@link Page.setDefaultNavigationTimeout} or
     *   {@link Page.setDefaultTimeout} methods.
     *
     * - `waitUntil`: When to consider navigation succeeded, defaults to `load`.
     *   Given an array of event strings, navigation is considered to be
     *   successful after all events have been fired. Events can be either:<br/>
     * - `load` : consider navigation to be finished when the load event is
     *   fired.<br/>
     * - `domcontentloaded` : consider navigation to be finished when the
     *   DOMContentLoaded event is fired.<br/>
     * - `networkidle0` : consider navigation to be finished when there are no
     *   more than 0 network connections for at least `500` ms.<br/>
     * - `networkidle2` : consider navigation to be finished when there are no
     *   more than 2 network connections for at least `500` ms.
     */
    reload(options?: WaitForOptions): Promise<HTTPResponse | null>;
    /**
     * Waits for the page to navigate to a new URL or to reload. It is useful when
     * you run code that will indirectly cause the page to navigate.
     *
     * @example
     *
     * ```ts
     * const [response] = await Promise.all([
     *   page.waitForNavigation(), // The promise resolves after navigation has finished
     *   page.click('a.my-link'), // Clicking the link will indirectly cause a navigation
     * ]);
     * ```
     *
     * @remarks
     * Usage of the
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/History_API | History API}
     * to change the URL is considered a navigation.
     *
     * @param options - Navigation parameters which might have the following
     * properties:
     * @returns A `Promise` which resolves to the main resource response.
     *
     * - In case of multiple redirects, the navigation will resolve with the
     *   response of the last redirect.
     * - In case of navigation to a different anchor or navigation due to History
     *   API usage, the navigation will resolve with `null`.
     */
    waitForNavigation(options?: WaitForOptions): Promise<HTTPResponse | null>;
    /**
     * @param urlOrPredicate - A URL or predicate to wait for
     * @param options - Optional waiting parameters
     * @returns Promise which resolves to the matched request
     * @example
     *
     * ```ts
     * const firstRequest = await page.waitForRequest(
     *   'https://example.com/resource'
     * );
     * const finalRequest = await page.waitForRequest(
     *   request => request.url() === 'https://example.com'
     * );
     * return finalRequest.response()?.ok();
     * ```
     *
     * @remarks
     * Optional Waiting Parameters have:
     *
     * - `timeout`: Maximum wait time in milliseconds, defaults to `30` seconds, pass
     *   `0` to disable the timeout. The default value can be changed by using the
     *   {@link Page.setDefaultTimeout} method.
     */
    waitForRequest(urlOrPredicate: string | ((req: HTTPRequest) => boolean | Promise<boolean>), options?: {
        timeout?: number;
    }): Promise<HTTPRequest>;
    /**
     * @param urlOrPredicate - A URL or predicate to wait for.
     * @param options - Optional waiting parameters
     * @returns Promise which resolves to the matched response.
     * @example
     *
     * ```ts
     * const firstResponse = await page.waitForResponse(
     *   'https://example.com/resource'
     * );
     * const finalResponse = await page.waitForResponse(
     *   response =>
     *     response.url() === 'https://example.com' && response.status() === 200
     * );
     * const finalResponse = await page.waitForResponse(async response => {
     *   return (await response.text()).includes('<html>');
     * });
     * return finalResponse.ok();
     * ```
     *
     * @remarks
     * Optional Parameter have:
     *
     * - `timeout`: Maximum wait time in milliseconds, defaults to `30` seconds,
     *   pass `0` to disable the timeout. The default value can be changed by using
     *   the {@link Page.setDefaultTimeout} method.
     */
    waitForResponse(urlOrPredicate: string | ((res: HTTPResponse) => boolean | Promise<boolean>), options?: {
        timeout?: number;
    }): Promise<HTTPResponse>;
    /**
     * @param options - Optional waiting parameters
     * @returns Promise which resolves when network is idle
     */
    waitForNetworkIdle(options?: {
        idleTime?: number;
        timeout?: number;
    }): Promise<void>;
    /* Excluded from this release type: _waitForNetworkIdle */
    /**
     * Waits for a frame matching the given conditions to appear.
     *
     * @example
     *
     * ```ts
     * const frame = await page.waitForFrame(async frame => {
     *   return frame.name() === 'Test';
     * });
     * ```
     */
    waitForFrame(urlOrPredicate: string | ((frame: Frame) => Awaitable<boolean>), options?: WaitTimeoutOptions): Promise<Frame>;
    /**
     * This method navigate to the previous page in history.
     * @param options - Navigation parameters
     * @returns Promise which resolves to the main resource response. In case of
     * multiple redirects, the navigation will resolve with the response of the
     * last redirect. If can not go back, resolves to `null`.
     * @remarks
     * The argument `options` might have the following properties:
     *
     * - `timeout` : Maximum navigation time in milliseconds, defaults to 30
     *   seconds, pass 0 to disable timeout. The default value can be changed by
     *   using the {@link Page.setDefaultNavigationTimeout} or
     *   {@link Page.setDefaultTimeout} methods.
     *
     * - `waitUntil` : When to consider navigation succeeded, defaults to `load`.
     *   Given an array of event strings, navigation is considered to be
     *   successful after all events have been fired. Events can be either:<br/>
     * - `load` : consider navigation to be finished when the load event is
     *   fired.<br/>
     * - `domcontentloaded` : consider navigation to be finished when the
     *   DOMContentLoaded event is fired.<br/>
     * - `networkidle0` : consider navigation to be finished when there are no
     *   more than 0 network connections for at least `500` ms.<br/>
     * - `networkidle2` : consider navigation to be finished when there are no
     *   more than 2 network connections for at least `500` ms.
     */
    goBack(options?: WaitForOptions): Promise<HTTPResponse | null>;
    /**
     * This method navigate to the next page in history.
     * @param options - Navigation Parameter
     * @returns Promise which resolves to the main resource response. In case of
     * multiple redirects, the navigation will resolve with the response of the
     * last redirect. If can not go forward, resolves to `null`.
     * @remarks
     * The argument `options` might have the following properties:
     *
     * - `timeout` : Maximum navigation time in milliseconds, defaults to 30
     *   seconds, pass 0 to disable timeout. The default value can be changed by
     *   using the {@link Page.setDefaultNavigationTimeout} or
     *   {@link Page.setDefaultTimeout} methods.
     *
     * - `waitUntil`: When to consider navigation succeeded, defaults to `load`.
     *   Given an array of event strings, navigation is considered to be
     *   successful after all events have been fired. Events can be either:<br/>
     * - `load` : consider navigation to be finished when the load event is
     *   fired.<br/>
     * - `domcontentloaded` : consider navigation to be finished when the
     *   DOMContentLoaded event is fired.<br/>
     * - `networkidle0` : consider navigation to be finished when there are no
     *   more than 0 network connections for at least `500` ms.<br/>
     * - `networkidle2` : consider navigation to be finished when there are no
     *   more than 2 network connections for at least `500` ms.
     */
    goForward(options?: WaitForOptions): Promise<HTTPResponse | null>;
    /**
     * Brings page to front (activates tab).
     */
    bringToFront(): Promise<void>;
    /**
     * Emulates a given device's metrics and user agent.
     *
     * To aid emulation, Puppeteer provides a list of known devices that can be
     * via {@link KnownDevices}.
     *
     * @remarks
     * This method is a shortcut for calling two methods:
     * {@link Page.setUserAgent} and {@link Page.setViewport}.
     *
     * @remarks
     * This method will resize the page. A lot of websites don't expect phones to
     * change size, so you should emulate before navigating to the page.
     *
     * @example
     *
     * ```ts
     * import {KnownDevices} from 'puppeteer';
     * const iPhone = KnownDevices['iPhone 6'];
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   await page.emulate(iPhone);
     *   await page.goto('https://www.google.com');
     *   // other actions...
     *   await browser.close();
     * })();
     * ```
     */
    emulate(device: Device): Promise<void>;
    /**
     * @param enabled - Whether or not to enable JavaScript on the page.
     * @remarks
     * NOTE: changing this value won't affect scripts that have already been run.
     * It will take full effect on the next navigation.
     */
    setJavaScriptEnabled(enabled: boolean): Promise<void>;
    /**
     * Toggles bypassing page's Content-Security-Policy.
     * @param enabled - sets bypassing of page's Content-Security-Policy.
     * @remarks
     * NOTE: CSP bypassing happens at the moment of CSP initialization rather than
     * evaluation. Usually, this means that `page.setBypassCSP` should be called
     * before navigating to the domain.
     */
    setBypassCSP(enabled: boolean): Promise<void>;
    /**
     * @param type - Changes the CSS media type of the page. The only allowed
     * values are `screen`, `print` and `null`. Passing `null` disables CSS media
     * emulation.
     * @example
     *
     * ```ts
     * await page.evaluate(() => matchMedia('screen').matches);
     * // → true
     * await page.evaluate(() => matchMedia('print').matches);
     * // → false
     *
     * await page.emulateMediaType('print');
     * await page.evaluate(() => matchMedia('screen').matches);
     * // → false
     * await page.evaluate(() => matchMedia('print').matches);
     * // → true
     *
     * await page.emulateMediaType(null);
     * await page.evaluate(() => matchMedia('screen').matches);
     * // → true
     * await page.evaluate(() => matchMedia('print').matches);
     * // → false
     * ```
     */
    emulateMediaType(type?: string): Promise<void>;
    /**
     * Enables CPU throttling to emulate slow CPUs.
     * @param factor - slowdown factor (1 is no throttle, 2 is 2x slowdown, etc).
     */
    emulateCPUThrottling(factor: number | null): Promise<void>;
    /**
     * @param features - `<?Array<Object>>` Given an array of media feature
     * objects, emulates CSS media features on the page. Each media feature object
     * must have the following properties:
     * @example
     *
     * ```ts
     * await page.emulateMediaFeatures([
     *   {name: 'prefers-color-scheme', value: 'dark'},
     * ]);
     * await page.evaluate(
     *   () => matchMedia('(prefers-color-scheme: dark)').matches
     * );
     * // → true
     * await page.evaluate(
     *   () => matchMedia('(prefers-color-scheme: light)').matches
     * );
     * // → false
     *
     * await page.emulateMediaFeatures([
     *   {name: 'prefers-reduced-motion', value: 'reduce'},
     * ]);
     * await page.evaluate(
     *   () => matchMedia('(prefers-reduced-motion: reduce)').matches
     * );
     * // → true
     * await page.evaluate(
     *   () => matchMedia('(prefers-reduced-motion: no-preference)').matches
     * );
     * // → false
     *
     * await page.emulateMediaFeatures([
     *   {name: 'prefers-color-scheme', value: 'dark'},
     *   {name: 'prefers-reduced-motion', value: 'reduce'},
     * ]);
     * await page.evaluate(
     *   () => matchMedia('(prefers-color-scheme: dark)').matches
     * );
     * // → true
     * await page.evaluate(
     *   () => matchMedia('(prefers-color-scheme: light)').matches
     * );
     * // → false
     * await page.evaluate(
     *   () => matchMedia('(prefers-reduced-motion: reduce)').matches
     * );
     * // → true
     * await page.evaluate(
     *   () => matchMedia('(prefers-reduced-motion: no-preference)').matches
     * );
     * // → false
     *
     * await page.emulateMediaFeatures([{name: 'color-gamut', value: 'p3'}]);
     * await page.evaluate(() => matchMedia('(color-gamut: srgb)').matches);
     * // → true
     * await page.evaluate(() => matchMedia('(color-gamut: p3)').matches);
     * // → true
     * await page.evaluate(() => matchMedia('(color-gamut: rec2020)').matches);
     * // → false
     * ```
     */
    emulateMediaFeatures(features?: MediaFeature[]): Promise<void>;
    /**
     * @param timezoneId - Changes the timezone of the page. See
     * {@link https://source.chromium.org/chromium/chromium/deps/icu.git/+/faee8bc70570192d82d2978a71e2a615788597d1:source/data/misc/metaZones.txt | ICU’s metaZones.txt}
     * for a list of supported timezone IDs. Passing
     * `null` disables timezone emulation.
     */
    emulateTimezone(timezoneId?: string): Promise<void>;
    /**
     * Emulates the idle state.
     * If no arguments set, clears idle state emulation.
     *
     * @example
     *
     * ```ts
     * // set idle emulation
     * await page.emulateIdleState({isUserActive: true, isScreenUnlocked: false});
     *
     * // do some checks here
     * ...
     *
     * // clear idle emulation
     * await page.emulateIdleState();
     * ```
     *
     * @param overrides - Mock idle state. If not set, clears idle overrides
     */
    emulateIdleState(overrides?: {
        isUserActive: boolean;
        isScreenUnlocked: boolean;
    }): Promise<void>;
    /**
     * Simulates the given vision deficiency on the page.
     *
     * @example
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     *
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   await page.goto('https://v8.dev/blog/10-years');
     *
     *   await page.emulateVisionDeficiency('achromatopsia');
     *   await page.screenshot({path: 'achromatopsia.png'});
     *
     *   await page.emulateVisionDeficiency('deuteranopia');
     *   await page.screenshot({path: 'deuteranopia.png'});
     *
     *   await page.emulateVisionDeficiency('blurredVision');
     *   await page.screenshot({path: 'blurred-vision.png'});
     *
     *   await browser.close();
     * })();
     * ```
     *
     * @param type - the type of deficiency to simulate, or `'none'` to reset.
     */
    emulateVisionDeficiency(type?: Protocol.Emulation.SetEmulatedVisionDeficiencyRequest['type']): Promise<void>;
    /**
     * `page.setViewport` will resize the page. A lot of websites don't expect
     * phones to change size, so you should set the viewport before navigating to
     * the page.
     *
     * In the case of multiple pages in a single browser, each page can have its
     * own viewport size.
     * @example
     *
     * ```ts
     * const page = await browser.newPage();
     * await page.setViewport({
     *   width: 640,
     *   height: 480,
     *   deviceScaleFactor: 1,
     * });
     * await page.goto('https://example.com');
     * ```
     *
     * @param viewport -
     * @remarks
     * Argument viewport have following properties:
     *
     * - `width`: page width in pixels. required
     *
     * - `height`: page height in pixels. required
     *
     * - `deviceScaleFactor`: Specify device scale factor (can be thought of as
     *   DPR). Defaults to `1`.
     *
     * - `isMobile`: Whether the meta viewport tag is taken into account. Defaults
     *   to `false`.
     *
     * - `hasTouch`: Specifies if viewport supports touch events. Defaults to `false`
     *
     * - `isLandScape`: Specifies if viewport is in landscape mode. Defaults to false.
     *
     * NOTE: in certain cases, setting viewport will reload the page in order to
     * set the isMobile or hasTouch properties.
     */
    setViewport(viewport: Viewport): Promise<void>;
    /**
     * Current page viewport settings.
     *
     * @returns
     *
     * - `width`: page's width in pixels
     *
     * - `height`: page's height in pixels
     *
     * - `deviceScaleFactor`: Specify device scale factor (can be though of as
     *   dpr). Defaults to `1`.
     *
     * - `isMobile`: Whether the meta viewport tag is taken into account. Defaults
     *   to `false`.
     *
     * - `hasTouch`: Specifies if viewport supports touch events. Defaults to
     *   `false`.
     *
     * - `isLandScape`: Specifies if viewport is in landscape mode. Defaults to
     *   `false`.
     */
    viewport(): Viewport | null;
    /**
     * Evaluates a function in the page's context and returns the result.
     *
     * If the function passed to `page.evaluate` returns a Promise, the
     * function will wait for the promise to resolve and return its value.
     *
     * @example
     *
     * ```ts
     * const result = await frame.evaluate(() => {
     *   return Promise.resolve(8 * 7);
     * });
     * console.log(result); // prints "56"
     * ```
     *
     * You can pass a string instead of a function (although functions are
     * recommended as they are easier to debug and use with TypeScript):
     *
     * @example
     *
     * ```ts
     * const aHandle = await page.evaluate('1 + 2');
     * ```
     *
     * To get the best TypeScript experience, you should pass in as the
     * generic the type of `pageFunction`:
     *
     * ```ts
     * const aHandle = await page.evaluate(() => 2);
     * ```
     *
     * @example
     *
     * {@link ElementHandle} instances (including {@link JSHandle}s) can be passed
     * as arguments to the `pageFunction`:
     *
     * ```ts
     * const bodyHandle = await page.$('body');
     * const html = await page.evaluate(body => body.innerHTML, bodyHandle);
     * await bodyHandle.dispose();
     * ```
     *
     * @param pageFunction - a function that is run within the page
     * @param args - arguments to be passed to the pageFunction
     *
     * @returns the return value of `pageFunction`.
     */
    evaluate<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * Adds a function which would be invoked in one of the following scenarios:
     *
     * - whenever the page is navigated
     *
     * - whenever the child frame is attached or navigated. In this case, the
     *   function is invoked in the context of the newly attached frame.
     *
     * The function is invoked after the document was created but before any of
     * its scripts were run. This is useful to amend the JavaScript environment,
     * e.g. to seed `Math.random`.
     * @param pageFunction - Function to be evaluated in browser context
     * @param args - Arguments to pass to `pageFunction`
     * @example
     * An example of overriding the navigator.languages property before the page loads:
     *
     * ```ts
     * // preload.js
     *
     * // overwrite the `languages` property to use a custom getter
     * Object.defineProperty(navigator, 'languages', {
     *   get: function () {
     *     return ['en-US', 'en', 'bn'];
     *   },
     * });
     *
     * // In your puppeteer script, assuming the preload.js file is
     * // in same folder of our script.
     * const preloadFile = fs.readFileSync('./preload.js', 'utf8');
     * await page.evaluateOnNewDocument(preloadFile);
     * ```
     */
    evaluateOnNewDocument<Params extends unknown[], Func extends (...args: Params) => unknown = (...args: Params) => unknown>(pageFunction: Func | string, ...args: Params): Promise<NewDocumentScriptEvaluation>;
    /**
     * Removes script that injected into page by Page.evaluateOnNewDocument.
     *
     * @param identifier - script identifier
     */
    removeScriptToEvaluateOnNewDocument(identifier: string): Promise<void>;
    /**
     * Toggles ignoring cache for each request based on the enabled state. By
     * default, caching is enabled.
     * @param enabled - sets the `enabled` state of cache
     * @defaultValue `true`
     */
    setCacheEnabled(enabled?: boolean): Promise<void>;
    /* Excluded from this release type: _maybeWriteBufferToFile */
    /**
     * Captures screenshot of the current page.
     *
     * @remarks
     * Options object which might have the following properties:
     *
     * - `path` : The file path to save the image to. The screenshot type
     *   will be inferred from file extension. If `path` is a relative path, then
     *   it is resolved relative to
     *   {@link https://nodejs.org/api/process.html#process_process_cwd
     *   | current working directory}.
     *   If no path is provided, the image won't be saved to the disk.
     *
     * - `type` : Specify screenshot type, can be `jpeg`, `png` or `webp`.
     *   Defaults to 'png'.
     *
     * - `quality` : The quality of the image, between 0-100. Not
     *   applicable to `png` images.
     *
     * - `fullPage` : When true, takes a screenshot of the full
     *   scrollable page. Defaults to `false`.
     *
     * - `clip` : An object which specifies clipping region of the page.
     *   Should have the following fields:<br/>
     * - `x` : x-coordinate of top-left corner of clip area.<br/>
     * - `y` : y-coordinate of top-left corner of clip area.<br/>
     * - `width` : width of clipping area.<br/>
     * - `height` : height of clipping area.
     *
     * - `omitBackground` : Hides default white background and allows
     *   capturing screenshots with transparency. Defaults to `false`.
     *
     * - `encoding` : The encoding of the image, can be either base64 or
     *   binary. Defaults to `binary`.
     *
     * - `captureBeyondViewport` : When true, captures screenshot
     *   {@link https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
     *   | beyond the viewport}. When false, falls back to old behaviour,
     *   and cuts the screenshot by the viewport size. Defaults to `true`.
     *
     * - `fromSurface` : When true, captures screenshot
     *   {@link https://chromedevtools.github.io/devtools-protocol/tot/Page/#method-captureScreenshot
     *   | from the surface rather than the view}. When false, works only in
     *   headful mode and ignores page viewport (but not browser window's
     *   bounds). Defaults to `true`.
     *
     * @returns Promise which resolves to buffer or a base64 string (depending on
     * the value of `encoding`) with captured screenshot.
     */
    screenshot(options: ScreenshotOptions & {
        encoding: 'base64';
    }): Promise<string>;
    screenshot(options?: ScreenshotOptions & {
        encoding?: 'binary';
    }): Promise<Buffer>;
    screenshot(options?: ScreenshotOptions): Promise<Buffer | string>;
    /* Excluded from this release type: _getPDFOptions */
    /**
     * Generates a PDF of the page with the `print` CSS media type.
     * @remarks
     *
     * To generate a PDF with the `screen` media type, call
     * {@link Page.emulateMediaType | `page.emulateMediaType('screen')`} before
     * calling `page.pdf()`.
     *
     * By default, `page.pdf()` generates a pdf with modified colors for printing.
     * Use the
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/-webkit-print-color-adjust | `-webkit-print-color-adjust`}
     * property to force rendering of exact colors.
     *
     * @param options - options for generating the PDF.
     */
    createPDFStream(options?: PDFOptions): Promise<Readable>;
    /**
     * {@inheritDoc Page.createPDFStream}
     */
    pdf(options?: PDFOptions): Promise<Buffer>;
    /**
     * The page's title
     *
     * @remarks
     * Shortcut for {@link Frame.title | page.mainFrame().title()}.
     */
    title(): Promise<string>;
    close(options?: {
        runBeforeUnload?: boolean;
    }): Promise<void>;
    /**
     * Indicates that the page has been closed.
     * @returns
     */
    isClosed(): boolean;
    /**
     * {@inheritDoc Mouse}
     */
    get mouse(): Mouse;
    /**
     * This method fetches an element with `selector`, scrolls it into view if
     * needed, and then uses {@link Page | Page.mouse} to click in the center of the
     * element. If there's no element matching `selector`, the method throws an
     * error.
     * @remarks Bear in mind that if `click()` triggers a navigation event and
     * there's a separate `page.waitForNavigation()` promise to be resolved, you
     * may end up with a race condition that yields unexpected results. The
     * correct pattern for click and wait for navigation is the following:
     *
     * ```ts
     * const [response] = await Promise.all([
     *   page.waitForNavigation(waitOptions),
     *   page.click(selector, clickOptions),
     * ]);
     * ```
     *
     * Shortcut for {@link Frame.click | page.mainFrame().click(selector[, options]) }.
     * @param selector - A `selector` to search for element to click. If there are
     * multiple elements satisfying the `selector`, the first will be clicked
     * @param options - `Object`
     * @returns Promise which resolves when the element matching `selector` is
     * successfully clicked. The Promise will be rejected if there is no element
     * matching `selector`.
     */
    click(selector: string, options?: Readonly<ClickOptions>): Promise<void>;
    /**
     * This method fetches an element with `selector` and focuses it. If there's no
     * element matching `selector`, the method throws an error.
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector }
     * of an element to focus. If there are multiple elements satisfying the
     * selector, the first will be focused.
     * @returns Promise which resolves when the element matching selector is
     * successfully focused. The promise will be rejected if there is no element
     * matching selector.
     * @remarks
     * Shortcut for {@link Frame.focus | page.mainFrame().focus(selector)}.
     */
    focus(selector: string): Promise<void>;
    /**
     * This method fetches an element with `selector`, scrolls it into view if
     * needed, and then uses {@link Page | Page.mouse}
     * to hover over the center of the element.
     * If there's no element matching `selector`, the method throws an error.
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * to search for element to hover. If there are multiple elements satisfying
     * the selector, the first will be hovered.
     * @returns Promise which resolves when the element matching `selector` is
     * successfully hovered. Promise gets rejected if there's no element matching
     * `selector`.
     * @remarks
     * Shortcut for {@link Page.hover | page.mainFrame().hover(selector)}.
     */
    hover(selector: string): Promise<void>;
    /**
     * Triggers a `change` and `input` event once all the provided options have been
     * selected. If there's no `<select>` element matching `selector`, the method
     * throws an error.
     *
     * @example
     *
     * ```ts
     * page.select('select#colors', 'blue'); // single selection
     * page.select('select#colors', 'red', 'green', 'blue'); // multiple selections
     * ```
     *
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | Selector}
     * to query the page for
     * @param values - Values of options to select. If the `<select>` has the
     * `multiple` attribute, all values are considered, otherwise only the first one
     * is taken into account.
     * @returns
     *
     * @remarks
     * Shortcut for {@link Frame.select | page.mainFrame().select()}
     */
    select(selector: string, ...values: string[]): Promise<string[]>;
    /**
     * This method fetches an element with `selector`, scrolls it into view if
     * needed, and then uses {@link Page | Page.touchscreen}
     * to tap in the center of the element.
     * If there's no element matching `selector`, the method throws an error.
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | Selector}
     * to search for element to tap. If there are multiple elements satisfying the
     * selector, the first will be tapped.
     * @returns
     * @remarks
     * Shortcut for {@link Frame.tap | page.mainFrame().tap(selector)}.
     */
    tap(selector: string): Promise<void>;
    /**
     * Sends a `keydown`, `keypress/input`, and `keyup` event for each character
     * in the text.
     *
     * To press a special key, like `Control` or `ArrowDown`, use {@link Keyboard.press}.
     * @example
     *
     * ```ts
     * await page.type('#mytextarea', 'Hello');
     * // Types instantly
     * await page.type('#mytextarea', 'World', {delay: 100});
     * // Types slower, like a user
     * ```
     *
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * of an element to type into. If there are multiple elements satisfying the
     * selector, the first will be used.
     * @param text - A text to type into a focused element.
     * @param options - have property `delay` which is the Time to wait between
     * key presses in milliseconds. Defaults to `0`.
     * @returns
     * @remarks
     */
    type(selector: string, text: string, options?: Readonly<KeyboardTypeOptions>): Promise<void>;
    /**
     * @deprecated Replace with `new Promise(r => setTimeout(r, milliseconds));`.
     *
     * Causes your script to wait for the given number of milliseconds.
     *
     * @remarks
     * It's generally recommended to not wait for a number of seconds, but instead
     * use {@link Frame.waitForSelector}, {@link Frame.waitForXPath} or
     * {@link Frame.waitForFunction} to wait for exactly the conditions you want.
     *
     * @example
     *
     * Wait for 1 second:
     *
     * ```ts
     * await page.waitForTimeout(1000);
     * ```
     *
     * @param milliseconds - the number of milliseconds to wait.
     */
    waitForTimeout(milliseconds: number): Promise<void>;
    /**
     * Wait for the `selector` to appear in page. If at the moment of calling the
     * method the `selector` already exists, the method will return immediately. If
     * the `selector` doesn't appear after the `timeout` milliseconds of waiting, the
     * function will throw.
     *
     * @example
     * This method works across navigations:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   let currentURL;
     *   page
     *     .waitForSelector('img')
     *     .then(() => console.log('First URL with image: ' + currentURL));
     *   for (currentURL of [
     *     'https://example.com',
     *     'https://google.com',
     *     'https://bbc.com',
     *   ]) {
     *     await page.goto(currentURL);
     *   }
     *   await browser.close();
     * })();
     * ```
     *
     * @param selector - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Selectors | selector}
     * of an element to wait for
     * @param options - Optional waiting parameters
     * @returns Promise which resolves when element specified by selector string
     * is added to DOM. Resolves to `null` if waiting for hidden: `true` and
     * selector is not found in DOM.
     * @remarks
     * The optional Parameter in Arguments `options` are:
     *
     * - `visible`: A boolean wait for element to be present in DOM and to be
     *   visible, i.e. to not have `display: none` or `visibility: hidden` CSS
     *   properties. Defaults to `false`.
     *
     * - `hidden`: Wait for element to not be found in the DOM or to be hidden,
     *   i.e. have `display: none` or `visibility: hidden` CSS properties. Defaults to
     *   `false`.
     *
     * - `timeout`: maximum time to wait for in milliseconds. Defaults to `30000`
     *   (30 seconds). Pass `0` to disable timeout. The default value can be changed
     *   by using the {@link Page.setDefaultTimeout} method.
     */
    waitForSelector<Selector extends string>(selector: Selector, options?: WaitForSelectorOptions): Promise<ElementHandle<NodeFor<Selector>> | null>;
    /**
     * Wait for the `xpath` to appear in page. If at the moment of calling the
     * method the `xpath` already exists, the method will return immediately. If
     * the `xpath` doesn't appear after the `timeout` milliseconds of waiting, the
     * function will throw.
     *
     * @example
     * This method works across navigation
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   let currentURL;
     *   page
     *     .waitForXPath('//img')
     *     .then(() => console.log('First URL with image: ' + currentURL));
     *   for (currentURL of [
     *     'https://example.com',
     *     'https://google.com',
     *     'https://bbc.com',
     *   ]) {
     *     await page.goto(currentURL);
     *   }
     *   await browser.close();
     * })();
     * ```
     *
     * @param xpath - A
     * {@link https://developer.mozilla.org/en-US/docs/Web/XPath | xpath} of an
     * element to wait for
     * @param options - Optional waiting parameters
     * @returns Promise which resolves when element specified by xpath string is
     * added to DOM. Resolves to `null` if waiting for `hidden: true` and xpath is
     * not found in DOM, otherwise resolves to `ElementHandle`.
     * @remarks
     * The optional Argument `options` have properties:
     *
     * - `visible`: A boolean to wait for element to be present in DOM and to be
     *   visible, i.e. to not have `display: none` or `visibility: hidden` CSS
     *   properties. Defaults to `false`.
     *
     * - `hidden`: A boolean wait for element to not be found in the DOM or to be
     *   hidden, i.e. have `display: none` or `visibility: hidden` CSS properties.
     *   Defaults to `false`.
     *
     * - `timeout`: A number which is maximum time to wait for in milliseconds.
     *   Defaults to `30000` (30 seconds). Pass `0` to disable timeout. The default
     *   value can be changed by using the {@link Page.setDefaultTimeout} method.
     */
    waitForXPath(xpath: string, options?: WaitForSelectorOptions): Promise<ElementHandle<Node> | null>;
    /**
     * Waits for a function to finish evaluating in the page's context.
     *
     * @example
     * The {@link Page.waitForFunction} can be used to observe viewport size change:
     *
     * ```ts
     * import puppeteer from 'puppeteer';
     * (async () => {
     *   const browser = await puppeteer.launch();
     *   const page = await browser.newPage();
     *   const watchDog = page.waitForFunction('window.innerWidth < 100');
     *   await page.setViewport({width: 50, height: 50});
     *   await watchDog;
     *   await browser.close();
     * })();
     * ```
     *
     * @example
     * To pass arguments from node.js to the predicate of
     * {@link Page.waitForFunction} function:
     *
     * ```ts
     * const selector = '.foo';
     * await page.waitForFunction(
     *   selector => !!document.querySelector(selector),
     *   {},
     *   selector
     * );
     * ```
     *
     * @example
     * The predicate of {@link Page.waitForFunction} can be asynchronous too:
     *
     * ```ts
     * const username = 'github-username';
     * await page.waitForFunction(
     *   async username => {
     *     const githubResponse = await fetch(
     *       `https://api.github.com/users/${username}`
     *     );
     *     const githubUser = await githubResponse.json();
     *     // show the avatar
     *     const img = document.createElement('img');
     *     img.src = githubUser.avatar_url;
     *     // wait 3 seconds
     *     await new Promise((resolve, reject) => setTimeout(resolve, 3000));
     *     img.remove();
     *   },
     *   {},
     *   username
     * );
     * ```
     *
     * @param pageFunction - Function to be evaluated in browser context
     * @param options - Options for configuring waiting behavior.
     */
    waitForFunction<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, options?: FrameWaitForFunctionOptions, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
    /**
     * This method is typically coupled with an action that triggers a device
     * request from an api such as WebBluetooth.
     *
     * :::caution
     *
     * This must be called before the device request is made. It will not return a
     * currently active device prompt.
     *
     * :::
     *
     * @example
     *
     * ```ts
     * const [devicePrompt] = Promise.all([
     *   page.waitForDevicePrompt(),
     *   page.click('#connect-bluetooth'),
     * ]);
     * await devicePrompt.select(
     *   await devicePrompt.waitForDevice(({name}) => name.includes('My Device'))
     * );
     * ```
     */
    waitForDevicePrompt(options?: WaitTimeoutOptions): Promise<DeviceRequestPrompt>;
}

/* Excluded from this release type: PageBinding */

/* Excluded from this release type: pageBindingInitString */

/**
 * All the events that a page instance may emit.
 *
 * @public
 */
export declare const enum PageEmittedEvents {
    /**
     * Emitted when the page closes.
     */
    Close = "close",
    /**
     * Emitted when JavaScript within the page calls one of console API methods,
     * e.g. `console.log` or `console.dir`. Also emitted if the page throws an
     * error or a warning.
     *
     * @remarks
     * A `console` event provides a {@link ConsoleMessage} representing the
     * console message that was logged.
     *
     * @example
     * An example of handling `console` event:
     *
     * ```ts
     * page.on('console', msg => {
     *   for (let i = 0; i < msg.args().length; ++i)
     *     console.log(`${i}: ${msg.args()[i]}`);
     * });
     * page.evaluate(() => console.log('hello', 5, {foo: 'bar'}));
     * ```
     */
    Console = "console",
    /**
     * Emitted when a JavaScript dialog appears, such as `alert`, `prompt`,
     * `confirm` or `beforeunload`. Puppeteer can respond to the dialog via
     * {@link Dialog.accept} or {@link Dialog.dismiss}.
     */
    Dialog = "dialog",
    /**
     * Emitted when the JavaScript
     * {@link https://developer.mozilla.org/en-US/docs/Web/Events/DOMContentLoaded | DOMContentLoaded }
     * event is dispatched.
     */
    DOMContentLoaded = "domcontentloaded",
    /**
     * Emitted when the page crashes. Will contain an `Error`.
     */
    Error = "error",
    /** Emitted when a frame is attached. Will contain a {@link Frame}. */
    FrameAttached = "frameattached",
    /** Emitted when a frame is detached. Will contain a {@link Frame}. */
    FrameDetached = "framedetached",
    /**
     * Emitted when a frame is navigated to a new URL. Will contain a
     * {@link Frame}.
     */
    FrameNavigated = "framenavigated",
    /**
     * Emitted when the JavaScript
     * {@link https://developer.mozilla.org/en-US/docs/Web/Events/load | load}
     * event is dispatched.
     */
    Load = "load",
    /**
     * Emitted when the JavaScript code makes a call to `console.timeStamp`. For
     * the list of metrics see {@link Page.metrics | page.metrics}.
     *
     * @remarks
     * Contains an object with two properties:
     *
     * - `title`: the title passed to `console.timeStamp`
     * - `metrics`: object containing metrics as key/value pairs. The values will
     *   be `number`s.
     */
    Metrics = "metrics",
    /**
     * Emitted when an uncaught exception happens within the page. Contains an
     * `Error`.
     */
    PageError = "pageerror",
    /**
     * Emitted when the page opens a new tab or window.
     *
     * Contains a {@link Page} corresponding to the popup window.
     *
     * @example
     *
     * ```ts
     * const [popup] = await Promise.all([
     *   new Promise(resolve => page.once('popup', resolve)),
     *   page.click('a[target=_blank]'),
     * ]);
     * ```
     *
     * ```ts
     * const [popup] = await Promise.all([
     *   new Promise(resolve => page.once('popup', resolve)),
     *   page.evaluate(() => window.open('https://example.com')),
     * ]);
     * ```
     */
    Popup = "popup",
    /**
     * Emitted when a page issues a request and contains a {@link HTTPRequest}.
     *
     * @remarks
     * The object is readonly. See {@link Page.setRequestInterception} for
     * intercepting and mutating requests.
     */
    Request = "request",
    /**
     * Emitted when a request ended up loading from cache. Contains a
     * {@link HTTPRequest}.
     *
     * @remarks
     * For certain requests, might contain undefined.
     * {@link https://crbug.com/750469}
     */
    RequestServedFromCache = "requestservedfromcache",
    /**
     * Emitted when a request fails, for example by timing out.
     *
     * Contains a {@link HTTPRequest}.
     *
     * @remarks
     * HTTP Error responses, such as 404 or 503, are still successful responses
     * from HTTP standpoint, so request will complete with `requestfinished` event
     * and not with `requestfailed`.
     */
    RequestFailed = "requestfailed",
    /**
     * Emitted when a request finishes successfully. Contains a
     * {@link HTTPRequest}.
     */
    RequestFinished = "requestfinished",
    /**
     * Emitted when a response is received. Contains a {@link HTTPResponse}.
     */
    Response = "response",
    /**
     * Emitted when a dedicated
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API | WebWorker}
     * is spawned by the page.
     */
    WorkerCreated = "workercreated",
    /**
     * Emitted when a dedicated
     * {@link https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API | WebWorker}
     * is destroyed by the page.
     */
    WorkerDestroyed = "workerdestroyed"
}

/**
 * Denotes the objects received by callback functions for page events.
 *
 * See {@link PageEmittedEvents} for more detail on the events and when they are
 * emitted.
 *
 * @public
 */
export declare interface PageEventObject {
    close: never;
    console: ConsoleMessage;
    dialog: Dialog;
    domcontentloaded: never;
    error: Error;
    frameattached: Frame;
    framedetached: Frame;
    framenavigated: Frame;
    load: never;
    metrics: {
        title: string;
        metrics: Metrics;
    };
    pageerror: Error;
    popup: Page;
    request: HTTPRequest;
    response: HTTPResponse;
    requestfailed: HTTPRequest;
    requestfinished: HTTPRequest;
    requestservedfromcache: HTTPRequest;
    workercreated: WebWorker;
    workerdestroyed: WebWorker;
}

/* Excluded from this release type: PageTarget */

/**
 * All the valid paper format types when printing a PDF.
 *
 * @remarks
 *
 * The sizes of each format are as follows:
 *
 * - `Letter`: 8.5in x 11in
 *
 * - `Legal`: 8.5in x 14in
 *
 * - `Tabloid`: 11in x 17in
 *
 * - `Ledger`: 17in x 11in
 *
 * - `A0`: 33.1in x 46.8in
 *
 * - `A1`: 23.4in x 33.1in
 *
 * - `A2`: 16.54in x 23.4in
 *
 * - `A3`: 11.7in x 16.54in
 *
 * - `A4`: 8.27in x 11.7in
 *
 * - `A5`: 5.83in x 8.27in
 *
 * - `A6`: 4.13in x 5.83in
 *
 * @public
 */
export declare type PaperFormat = Uppercase<LowerCasePaperFormat> | Capitalize<LowerCasePaperFormat> | LowerCasePaperFormat;

/* Excluded from this release type: PaperFormatDimensions */

/* Excluded from this release type: paperFormats */

/* Excluded from this release type: ParsedPDFOptions */

/* Excluded from this release type: ParsedPDFOptionsInterface */

/**
 * Copyright 2020 Google Inc. All rights reserved.
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
/**
 * @public
 */
export declare interface PDFMargin {
    top?: string | number;
    bottom?: string | number;
    left?: string | number;
    right?: string | number;
}

/**
 * Valid options to configure PDF generation via {@link Page.pdf}.
 * @public
 */
export declare interface PDFOptions {
    /**
     * Scales the rendering of the web page. Amount must be between `0.1` and `2`.
     * @defaultValue `1`
     */
    scale?: number;
    /**
     * Whether to show the header and footer.
     * @defaultValue `false`
     */
    displayHeaderFooter?: boolean;
    /**
     * HTML template for the print header. Should be valid HTML with the following
     * classes used to inject values into them:
     *
     * - `date` formatted print date
     *
     * - `title` document title
     *
     * - `url` document location
     *
     * - `pageNumber` current page number
     *
     * - `totalPages` total pages in the document
     */
    headerTemplate?: string;
    /**
     * HTML template for the print footer. Has the same constraints and support
     * for special classes as {@link PDFOptions | PDFOptions.headerTemplate}.
     */
    footerTemplate?: string;
    /**
     * Set to `true` to print background graphics.
     * @defaultValue `false`
     */
    printBackground?: boolean;
    /**
     * Whether to print in landscape orientation.
     * @defaultValue `false`
     */
    landscape?: boolean;
    /**
     * Paper ranges to print, e.g. `1-5, 8, 11-13`.
     * @defaultValue The empty string, which means all pages are printed.
     */
    pageRanges?: string;
    /**
     * @remarks
     * If set, this takes priority over the `width` and `height` options.
     * @defaultValue `letter`.
     */
    format?: PaperFormat;
    /**
     * Sets the width of paper. You can pass in a number or a string with a unit.
     */
    width?: string | number;
    /**
     * Sets the height of paper. You can pass in a number or a string with a unit.
     */
    height?: string | number;
    /**
     * Give any CSS `@page` size declared in the page priority over what is
     * declared in the `width` or `height` or `format` option.
     * @defaultValue `false`, which will scale the content to fit the paper size.
     */
    preferCSSPageSize?: boolean;
    /**
     * Set the PDF margins.
     * @defaultValue `undefined` no margins are set.
     */
    margin?: PDFMargin;
    /**
     * The path to save the file to.
     *
     * @remarks
     *
     * If the path is relative, it's resolved relative to the current working directory.
     *
     * @defaultValue `undefined`, which means the PDF will not be written to disk.
     */
    path?: string;
    /**
     * Hides default white background and allows generating pdfs with transparency.
     * @defaultValue `false`
     */
    omitBackground?: boolean;
    /**
     * Timeout in milliseconds. Pass `0` to disable timeout.
     * @defaultValue `30_000`
     */
    timeout?: number;
}

/**
 * @public
 */
export declare type Permission = 'geolocation' | 'midi' | 'notifications' | 'camera' | 'microphone' | 'background-sync' | 'ambient-light-sensor' | 'accelerometer' | 'gyroscope' | 'magnetometer' | 'accessibility-events' | 'clipboard-read' | 'clipboard-write' | 'clipboard-sanitized-write' | 'payment-handler' | 'persistent-storage' | 'idle-detection' | 'midi-sysex';

/* Excluded from this release type: PipeTransport */

/**
 * @public
 */
export declare interface Point {
    x: number;
    y: number;
}

/* Excluded from this release type: Poller */

/**
 * A list of network conditions to be used with
 * {@link Page.emulateNetworkConditions}.
 *
 * @example
 *
 * ```ts
 * import {PredefinedNetworkConditions} from 'puppeteer';
 * const slow3G = PredefinedNetworkConditions['Slow 3G'];
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.emulateNetworkConditions(slow3G);
 *   await page.goto('https://www.google.com');
 *   // other actions...
 *   await browser.close();
 * })();
 * ```
 *
 * @public
 */
export declare const PredefinedNetworkConditions: Readonly<{
    'Slow 3G': NetworkConditions;
    'Fast 3G': NetworkConditions;
}>;

/**
 * @public
 */
export declare type Predicate<From, To extends From = From> = ((value: From) => value is To) | ((value: From) => Awaitable<boolean>);

/**
 * Copyright 2020 Google Inc. All rights reserved.
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
/**
 * Supported products.
 * @public
 */
export declare type Product = 'chrome' | 'firefox';

/**
 * Describes a launcher - a class that is able to create and launch a browser instance.
 *
 * @public
 */
export declare class ProductLauncher {
    #private;
    /* Excluded from this release type: puppeteer */
    /* Excluded from this release type: actualBrowserRevision */
    /* Excluded from this release type: __constructor */
    get product(): Product;
    launch(options?: PuppeteerNodeLaunchOptions): Promise<Browser>;
    executablePath(channel?: ChromeReleaseChannel): string;
    defaultArgs(object: BrowserLaunchArgumentOptions): string[];
    /* Excluded from this release type: getActualBrowserRevision */
    /* Excluded from this release type: computeLaunchArguments */
    /* Excluded from this release type: cleanUserDataDir */
    /* Excluded from this release type: closeBrowser */
    /* Excluded from this release type: waitForPageTarget */
    /* Excluded from this release type: createCDPSocketConnection */
    /* Excluded from this release type: createCDPPipeConnection */
    /* Excluded from this release type: createBiDiOverCDPBrowser */
    /* Excluded from this release type: createBiDiBrowser */
    /* Excluded from this release type: getProfilePath */
    /* Excluded from this release type: resolveExecutablePath */
}

export { Protocol }

/**
 * ProtocolError is emitted whenever there is an error from the protocol.
 *
 * @public
 */
export declare class ProtocolError extends CustomError {
    #private;
    set code(code: number | undefined);
    /**
     * @readonly
     * @public
     */
    get code(): number | undefined;
    set originalMessage(originalMessage: string);
    /**
     * @readonly
     * @public
     */
    get originalMessage(): string;
}

/**
 * @public
 */
export declare type ProtocolLifeCycleEvent = 'load' | 'DOMContentLoaded' | 'networkIdle' | 'networkAlmostIdle';

export { ProtocolMapping }

/**
 * The main Puppeteer class.
 *
 * IMPORTANT: if you are using Puppeteer in a Node environment, you will get an
 * instance of {@link PuppeteerNode} when you import or require `puppeteer`.
 * That class extends `Puppeteer`, so has all the methods documented below as
 * well as all that are defined on {@link PuppeteerNode}.
 *
 * @public
 */
export declare class Puppeteer {
    /* Excluded from this release type: customQueryHandlers */
    /**
     * Registers a {@link CustomQueryHandler | custom query handler}.
     *
     * @remarks
     * After registration, the handler can be used everywhere where a selector is
     * expected by prepending the selection string with `<name>/`. The name is only
     * allowed to consist of lower- and upper case latin letters.
     *
     * @example
     *
     * ```
     * puppeteer.registerCustomQueryHandler('text', { … });
     * const aHandle = await page.$('text/…');
     * ```
     *
     * @param name - The name that the custom query handler will be registered
     * under.
     * @param queryHandler - The {@link CustomQueryHandler | custom query handler}
     * to register.
     *
     * @public
     */
    static registerCustomQueryHandler(name: string, queryHandler: CustomQueryHandler): void;
    /**
     * Unregisters a custom query handler for a given name.
     */
    static unregisterCustomQueryHandler(name: string): void;
    /**
     * Gets the names of all custom query handlers.
     */
    static customQueryHandlerNames(): string[];
    /**
     * Unregisters all custom query handlers.
     */
    static clearCustomQueryHandlers(): void;
    /* Excluded from this release type: _isPuppeteerCore */
    /* Excluded from this release type: _changedProduct */
    /* Excluded from this release type: __constructor */
    /**
     * This method attaches Puppeteer to an existing browser instance.
     *
     * @remarks
     *
     * @param options - Set of configurable options to set on the browser.
     * @returns Promise which resolves to browser instance.
     */
    connect(options: ConnectOptions): Promise<Browser>;
}

/**
 * @public
 */
declare const puppeteer: PuppeteerNode;
export default puppeteer;

/* Excluded from this release type: PUPPETEER_REVISIONS */

/* Excluded from this release type: PUPPETEER_WORLD */

/**
 * @deprecated Do not use.
 *
 * @public
 */
export declare interface PuppeteerErrors {
    TimeoutError: typeof TimeoutError;
    ProtocolError: typeof ProtocolError;
}

/* Excluded from this release type: PuppeteerEventListener */

/**
 * @public
 */
export declare interface PuppeteerLaunchOptions extends LaunchOptions, BrowserLaunchArgumentOptions, BrowserConnectOptions {
    product?: Product;
    extraPrefsFirefox?: Record<string, unknown>;
}

/**
 * @public
 */
export declare type PuppeteerLifeCycleEvent = 'load' | 'domcontentloaded' | 'networkidle0' | 'networkidle2';

/**
 * Extends the main {@link Puppeteer} class with Node specific behaviour for
 * fetching and downloading browsers.
 *
 * If you're using Puppeteer in a Node environment, this is the class you'll get
 * when you run `require('puppeteer')` (or the equivalent ES `import`).
 *
 * @remarks
 * The most common method to use is {@link PuppeteerNode.launch | launch}, which
 * is used to launch and connect to a new browser instance.
 *
 * See {@link Puppeteer | the main Puppeteer class} for methods common to all
 * environments, such as {@link Puppeteer.connect}.
 *
 * @example
 * The following is a typical example of using Puppeteer to drive automation:
 *
 * ```ts
 * import puppeteer from 'puppeteer';
 *
 * (async () => {
 *   const browser = await puppeteer.launch();
 *   const page = await browser.newPage();
 *   await page.goto('https://www.google.com');
 *   // other actions...
 *   await browser.close();
 * })();
 * ```
 *
 * Once you have created a `page` you have access to a large API to interact
 * with the page, navigate, or find certain elements in that page.
 * The {@link Page | `page` documentation} lists all the available methods.
 *
 * @public
 */
export declare class PuppeteerNode extends Puppeteer {
    #private;
    /* Excluded from this release type: defaultBrowserRevision */
    /* Excluded from this release type: configuration */
    /* Excluded from this release type: __constructor */
    /**
     * This method attaches Puppeteer to an existing browser instance.
     *
     * @param options - Set of configurable options to set on the browser.
     * @returns Promise which resolves to browser instance.
     */
    connect(options: ConnectOptions): Promise<Browser>;
    /**
     * Launches a browser instance with given arguments and options when
     * specified.
     *
     * When using with `puppeteer-core`,
     * {@link LaunchOptions | options.executablePath} or
     * {@link LaunchOptions | options.channel} must be provided.
     *
     * @example
     * You can use {@link LaunchOptions | options.ignoreDefaultArgs}
     * to filter out `--mute-audio` from default arguments:
     *
     * ```ts
     * const browser = await puppeteer.launch({
     *   ignoreDefaultArgs: ['--mute-audio'],
     * });
     * ```
     *
     * @remarks
     * Puppeteer can also be used to control the Chrome browser, but it works best
     * with the version of Chrome for Testing downloaded by default.
     * There is no guarantee it will work with any other version. If Google Chrome
     * (rather than Chrome for Testing) is preferred, a
     * {@link https://www.google.com/chrome/browser/canary.html | Chrome Canary}
     * or
     * {@link https://www.chromium.org/getting-involved/dev-channel | Dev Channel}
     * build is suggested. See
     * {@link https://www.howtogeek.com/202825/what%E2%80%99s-the-difference-between-chromium-and-chrome/ | this article}
     * for a description of the differences between Chromium and Chrome.
     * {@link https://chromium.googlesource.com/chromium/src/+/lkgr/docs/chromium_browser_vs_google_chrome.md | This article}
     * describes some differences for Linux users. See
     * {@link https://goo.gle/chrome-for-testing | this doc} for the description
     * of Chrome for Testing.
     *
     * @param options - Options to configure launching behavior.
     */
    launch(options?: PuppeteerLaunchOptions): Promise<Browser>;
    /**
     * The default executable path.
     */
    executablePath(channel?: ChromeReleaseChannel): string;
    /* Excluded from this release type: browserRevision */
    /* Excluded from this release type: defaultDownloadPath */
    /**
     * The name of the browser that was last launched.
     */
    get lastLaunchedProduct(): Product;
    /**
     * The name of the browser that will be launched by default. For
     * `puppeteer`, this is influenced by your configuration. Otherwise, it's
     * `chrome`.
     */
    get defaultProduct(): Product;
    /**
     * @deprecated Do not use as this field as it does not take into account
     * multiple browsers of different types. Use
     * {@link PuppeteerNode.defaultProduct | defaultProduct} or
     * {@link PuppeteerNode.lastLaunchedProduct | lastLaunchedProduct}.
     *
     * @returns The name of the browser that is under automation.
     */
    get product(): string;
    /**
     * @param options - Set of configurable options to set on the browser.
     *
     * @returns The default flags that Chromium will be launched with.
     */
    defaultArgs(options?: BrowserLaunchArgumentOptions): string[];
    /**
     * Removes all non-current Firefox and Chrome binaries in the cache directory
     * identified by the provided Puppeteer configuration. The current browser
     * version is determined by resolving PUPPETEER_REVISIONS from Puppeteer
     * unless `configuration.browserRevision` is provided.
     *
     * @remarks
     *
     * Note that the method does not check if any other Puppeteer versions
     * installed on the host that use the same cache directory require the
     * non-current binaries.
     *
     * @public
     */
    trimCache(): Promise<void>;
}

/**
 * Utility type exposed to enable users to define options that can be passed to
 * `puppeteer.launch` without having to list the set of all types.
 * @public
 */
export declare type PuppeteerNodeLaunchOptions = BrowserLaunchArgumentOptions & LaunchOptions & BrowserConnectOptions;

/* Excluded from this release type: PuppeteerURL */

/* Excluded from this release type: PuppeteerUtil */

/* Excluded from this release type: PuppeteerUtilWrapper */

/**
 * @public
 */
export declare type Quad = [Point, Point, Point, Point];

/* Excluded from this release type: QueryHandler */

/* Excluded from this release type: QuerySelector */

/* Excluded from this release type: QuerySelectorAll */

/* Excluded from this release type: QueuedEventGroup */

/* Excluded from this release type: RaceLocator */

/* Excluded from this release type: RAFPoller */

/* Excluded from this release type: Realm */

/* Excluded from this release type: RedirectInfo */

/**
 * @deprecated Import {@link Puppeteer} and use the static method
 * {@link Puppeteer.registerCustomQueryHandler}
 *
 * @public
 */
export declare function registerCustomQueryHandler(name: string, handler: CustomQueryHandler): void;

/* Excluded from this release type: releaseObject */

/**
 * @public
 */
export declare interface RemoteAddress {
    ip?: string;
    port?: number;
}

/* Excluded from this release type: removeEventListeners */

/* Excluded from this release type: ResolvedLaunchArgs */

/**
 * Resource types for HTTPRequests as perceived by the rendering engine.
 *
 * @public
 */
export declare type ResourceType = Lowercase<Protocol.Network.ResourceType>;

/**
 * Required response data to fulfill a request with.
 *
 * @public
 */
export declare interface ResponseForRequest {
    status: number;
    /**
     * Optional response headers. All values are converted to strings.
     */
    headers: Record<string, unknown>;
    contentType: string;
    body: string | Buffer;
}

/* Excluded from this release type: RETRY_DELAY */

/**
 * @public
 */
export declare interface ScreenshotClip {
    x: number;
    y: number;
    width: number;
    height: number;
    /**
     * @defaultValue `1`
     */
    scale?: number;
}

/**
 * @public
 */
export declare interface ScreenshotOptions {
    /**
     * @defaultValue `false`
     */
    optimizeForSpeed?: boolean;
    /**
     * @defaultValue `png`
     */
    type?: 'png' | 'jpeg' | 'webp';
    /**
     * The file path to save the image to. The screenshot type will be inferred
     * from file extension. If path is a relative path, then it is resolved
     * relative to current working directory. If no path is provided, the image
     * won't be saved to the disk.
     */
    path?: string;
    /**
     * When `true`, takes a screenshot of the full page.
     * @defaultValue `false`
     */
    fullPage?: boolean;
    /**
     * An object which specifies the clipping region of the page.
     */
    clip?: ScreenshotClip;
    /**
     * Quality of the image, between 0-100. Not applicable to `png` images.
     */
    quality?: number;
    /**
     * Hides default white background and allows capturing screenshots with transparency.
     * @defaultValue `false`
     */
    omitBackground?: boolean;
    /**
     * Encoding of the image.
     * @defaultValue `binary`
     */
    encoding?: 'base64' | 'binary';
    /**
     * Capture the screenshot beyond the viewport.
     * @defaultValue `true`
     */
    captureBeyondViewport?: boolean;
    /**
     * Capture the screenshot from the surface, rather than the view.
     * @defaultValue `true`
     */
    fromSurface?: boolean;
}

/**
 * The SecurityDetails class represents the security details of a
 * response that was received over a secure connection.
 *
 * @public
 */
export declare class SecurityDetails {
    #private;
    /* Excluded from this release type: __constructor */
    /**
     * The name of the issuer of the certificate.
     */
    issuer(): string;
    /**
     * {@link https://en.wikipedia.org/wiki/Unix_time | Unix timestamp}
     * marking the start of the certificate's validity.
     */
    validFrom(): number;
    /**
     * {@link https://en.wikipedia.org/wiki/Unix_time | Unix timestamp}
     * marking the end of the certificate's validity.
     */
    validTo(): number;
    /**
     * The security protocol being used, e.g. "TLS 1.2".
     */
    protocol(): string;
    /**
     * The name of the subject to which the certificate was issued.
     */
    subjectName(): string;
    /**
     * The list of {@link https://en.wikipedia.org/wiki/Subject_Alternative_Name | subject alternative names (SANs)} of the certificate.
     */
    subjectAlternativeNames(): string[];
}

/**
 * Represents a Node and the properties of it that are relevant to Accessibility.
 * @public
 */
export declare interface SerializedAXNode {
    /**
     * The {@link https://www.w3.org/TR/wai-aria/#usage_intro | role} of the node.
     */
    role: string;
    /**
     * A human readable name for the node.
     */
    name?: string;
    /**
     * The current value of the node.
     */
    value?: string | number;
    /**
     * An additional human readable description of the node.
     */
    description?: string;
    /**
     * Any keyboard shortcuts associated with this node.
     */
    keyshortcuts?: string;
    /**
     * A human readable alternative to the role.
     */
    roledescription?: string;
    /**
     * A description of the current value.
     */
    valuetext?: string;
    disabled?: boolean;
    expanded?: boolean;
    focused?: boolean;
    modal?: boolean;
    multiline?: boolean;
    /**
     * Whether more than one child can be selected.
     */
    multiselectable?: boolean;
    readonly?: boolean;
    required?: boolean;
    selected?: boolean;
    /**
     * Whether the checkbox is checked, or in a
     * {@link https://www.w3.org/TR/wai-aria-practices/examples/checkbox/checkbox-2/checkbox-2.html | mixed state}.
     */
    checked?: boolean | 'mixed';
    /**
     * Whether the node is checked or in a mixed state.
     */
    pressed?: boolean | 'mixed';
    /**
     * The level of a heading.
     */
    level?: number;
    valuemin?: number;
    valuemax?: number;
    autocomplete?: string;
    haspopup?: string;
    /**
     * Whether and in what way this node's value is invalid.
     */
    invalid?: string;
    orientation?: string;
    /**
     * Children of this node, if there are any.
     */
    children?: SerializedAXNode[];
}

/* Excluded from this release type: setLogCapture */

/* Excluded from this release type: setPageContent */

/**
 * @public
 */
export declare interface SnapshotOptions {
    /**
     * Prune uninteresting nodes from the tree.
     * @defaultValue `true`
     */
    interestingOnly?: boolean;
    /**
     * Root node to get the accessibility tree for
     * @defaultValue The root node of the entire page.
     */
    root?: ElementHandle<Node>;
}

declare type Split<Input extends string, Delimiter extends string, Acc extends string[] = []> = Input extends `${infer Prefix}${Delimiter}${infer Suffix}` ? Split<Suffix, Delimiter, [...Acc, Prefix]> : [...Acc, Input];

declare type SplitWithDelemiters<Input extends string, Delemiters extends readonly string[]> = Delemiters extends [infer FirstDelemiter, ...infer RestDelemiters] ? FirstDelemiter extends string ? RestDelemiters extends readonly string[] ? FlatmapSplitWithDelemiters<Split<Input, FirstDelemiter>, RestDelemiters> : never : never : [Input];

/* Excluded from this release type: STATUS_TEXTS */

/* Excluded from this release type: supportedMetrics */

/**
 * Target represents a
 * {@link https://chromedevtools.github.io/devtools-protocol/tot/Target/ | CDP target}.
 * In CDP a target is something that can be debugged such a frame, a page or a
 * worker.
 * @public
 */
export declare class Target {
    /* Excluded from this release type: __constructor */
    /**
     * If the target is not of type `"service_worker"` or `"shared_worker"`, returns `null`.
     */
    worker(): Promise<WebWorker | null>;
    /**
     * If the target is not of type `"page"`, `"webview"` or `"background_page"`,
     * returns `null`.
     */
    page(): Promise<Page | null>;
    url(): string;
    /**
     * Creates a Chrome Devtools Protocol session attached to the target.
     */
    createCDPSession(): Promise<CDPSession>;
    /**
     * Identifies what kind of target this is.
     *
     * @remarks
     *
     * See {@link https://developer.chrome.com/extensions/background_pages | docs} for more info about background pages.
     */
    type(): TargetType;
    /**
     * Get the browser the target belongs to.
     */
    browser(): Browser;
    /**
     * Get the browser context the target belongs to.
     */
    browserContext(): BrowserContext;
    /**
     * Get the target that opened this target. Top-level targets return `null`.
     */
    opener(): Target | undefined;
}

/* Excluded from this release type: TargetCloseError */

/* Excluded from this release type: TargetFactory */

/**
 * @public
 */
export declare type TargetFilterCallback = (target: Target) => boolean;

/* Excluded from this release type: TargetInterceptor */

/* Excluded from this release type: TargetManager */

/* Excluded from this release type: TargetManagerEmittedEvents */

/**
 * @public
 */
export declare enum TargetType {
    PAGE = "page",
    BACKGROUND_PAGE = "background_page",
    SERVICE_WORKER = "service_worker",
    SHARED_WORKER = "shared_worker",
    BROWSER = "browser",
    WEBVIEW = "webview",
    OTHER = "other",
    /* Excluded from this release type: TAB */
}

/* Excluded from this release type: TaskManager */

/* Excluded from this release type: TaskQueue */

/* Excluded from this release type: TextContent */

/**
 * TimeoutError is emitted whenever certain operations are terminated due to
 * timeout.
 *
 * @remarks
 * Example operations are {@link Page.waitForSelector | page.waitForSelector} or
 * {@link PuppeteerNode.launch | puppeteer.launch}.
 *
 * @public
 */
export declare class TimeoutError extends CustomError {
}

/* Excluded from this release type: TimeoutSettings */

/**
 * The Touchscreen class exposes touchscreen events.
 * @public
 */
export declare class Touchscreen {
    /* Excluded from this release type: __constructor */
    /**
     * Dispatches a `touchstart` and `touchend` event.
     * @param x - Horizontal position of the tap.
     * @param y - Vertical position of the tap.
     */
    tap(x: number, y: number): Promise<void>;
    /**
     * Dispatches a `touchstart` event.
     * @param x - Horizontal position of the tap.
     * @param y - Vertical position of the tap.
     */
    touchStart(x: number, y: number): Promise<void>;
    /**
     * Dispatches a `touchMove` event.
     * @param x - Horizontal position of the move.
     * @param y - Vertical position of the move.
     */
    touchMove(x: number, y: number): Promise<void>;
    /**
     * Dispatches a `touchend` event.
     */
    touchEnd(): Promise<void>;
}

/**
 * The Tracing class exposes the tracing audit interface.
 * @remarks
 * You can use `tracing.start` and `tracing.stop` to create a trace file
 * which can be opened in Chrome DevTools or {@link https://chromedevtools.github.io/timeline-viewer/ | timeline viewer}.
 *
 * @example
 *
 * ```ts
 * await page.tracing.start({path: 'trace.json'});
 * await page.goto('https://www.google.com');
 * await page.tracing.stop();
 * ```
 *
 * @public
 */
export declare class Tracing {
    #private;
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: updateClient */
    /**
     * Starts a trace for the current page.
     * @remarks
     * Only one trace can be active at a time per browser.
     *
     * @param options - Optional `TracingOptions`.
     */
    start(options?: TracingOptions): Promise<void>;
    /**
     * Stops a trace started with the `start` method.
     * @returns Promise which resolves to buffer with trace data.
     */
    stop(): Promise<Buffer | undefined>;
}

/**
 * @public
 */
export declare interface TracingOptions {
    path?: string;
    screenshots?: boolean;
    categories?: string[];
}

declare type TypeSelectorOfComplexSelector<ComplexSelector extends string> = CompoundSelectorsOfComplexSelector<ComplexSelector> extends infer CompoundSelectors ? CompoundSelectors extends NonEmptyReadonlyArray<string> ? Last<CompoundSelectors> extends infer LastCompoundSelector ? LastCompoundSelector extends string ? TypeSelectorOfCompoundSelector<LastCompoundSelector> : never : never : unknown : never;

declare type TypeSelectorOfCompoundSelector<CompoundSelector extends string> = SplitWithDelemiters<CompoundSelector, BeginSubclassSelectorTokens> extends infer CompoundSelectorTokens ? CompoundSelectorTokens extends [infer TypeSelector, ...any[]] ? TypeSelector extends '' ? unknown : TypeSelector : never : never;

/* Excluded from this release type: unitToPixels */

/**
 * @deprecated Import {@link Puppeteer} and use the static method
 * {@link Puppeteer.unregisterCustomQueryHandler}
 *
 * @public
 */
export declare function unregisterCustomQueryHandler(name: string): void;

/* Excluded from this release type: UTILITY_WORLD_NAME */

/* Excluded from this release type: validateDialogType */

/* Excluded from this release type: valueFromRemoteObject */

/**
 * Copyright 2020 Google Inc. All rights reserved.
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
/**
 *
 * Sets the viewport of the page.
 * @public
 */
export declare interface Viewport {
    /**
     * The page width in pixels.
     */
    width: number;
    /**
     * The page height in pixels.
     */
    height: number;
    /**
     * Specify device scale factor.
     * See {@link https://developer.mozilla.org/en-US/docs/Web/API/Window/devicePixelRatio | devicePixelRatio} for more info.
     *
     * @remarks
     * Setting this value to `0` will set the deviceScaleFactor to the system default.
     *
     * @defaultValue `1`
     */
    deviceScaleFactor?: number;
    /**
     * Whether the `meta viewport` tag is taken into account.
     * @defaultValue `false`
     */
    isMobile?: boolean;
    /**
     * Specifies if the viewport is in landscape mode.
     * @defaultValue `false`
     */
    isLandscape?: boolean;
    /**
     * Specify if the viewport supports touch events.
     * @defaultValue `false`
     */
    hasTouch?: boolean;
}

/**
 * @public
 */
export declare type VisibilityOption = 'hidden' | 'visible' | null;

/* Excluded from this release type: waitForEvent */

/**
 * @public
 */
export declare interface WaitForOptions {
    /**
     * Maximum wait time in milliseconds. Pass 0 to disable the timeout.
     *
     * The default value can be changed by using the
     * {@link Page.setDefaultTimeout} or {@link Page.setDefaultNavigationTimeout}
     * methods.
     *
     * @defaultValue `30000`
     */
    timeout?: number;
    waitUntil?: PuppeteerLifeCycleEvent | PuppeteerLifeCycleEvent[];
}

/**
 * @public
 */
export declare interface WaitForSelectorOptions {
    /**
     * Wait for the selected element to be present in DOM and to be visible, i.e.
     * to not have `display: none` or `visibility: hidden` CSS properties.
     *
     * @defaultValue `false`
     */
    visible?: boolean;
    /**
     * Wait for the selected element to not be found in the DOM or to be hidden,
     * i.e. have `display: none` or `visibility: hidden` CSS properties.
     *
     * @defaultValue `false`
     */
    hidden?: boolean;
    /**
     * Maximum time to wait in milliseconds. Pass `0` to disable timeout.
     *
     * The default value can be changed by using {@link Page.setDefaultTimeout}
     *
     * @defaultValue `30_000` (30 seconds)
     */
    timeout?: number;
    /**
     * A signal object that allows you to cancel a waitForSelector call.
     */
    signal?: AbortSignal;
}

/**
 * @public
 */
export declare interface WaitForTargetOptions {
    /**
     * Maximum wait time in milliseconds. Pass `0` to disable the timeout.
     * @defaultValue `30_000`
     */
    timeout?: number;
}

/* Excluded from this release type: WaitTask */

/* Excluded from this release type: WaitTaskOptions */

/**
 * @public
 */
export declare interface WaitTimeoutOptions {
    /**
     * Maximum wait time in milliseconds. Pass 0 to disable the timeout.
     *
     * The default value can be changed by using the
     * {@link Page.setDefaultTimeout} method.
     *
     * @defaultValue `30000`
     */
    timeout?: number;
}

/* Excluded from this release type: waitWithTimeout */

/* Excluded from this release type: WEB_PERMISSION_TO_PROTOCOL_PERMISSION */

/**
 * This class represents a
 * {@link https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API | WebWorker}.
 *
 * @remarks
 * The events `workercreated` and `workerdestroyed` are emitted on the page
 * object to signal the worker lifecycle.
 *
 * @example
 *
 * ```ts
 * page.on('workercreated', worker =>
 *   console.log('Worker created: ' + worker.url())
 * );
 * page.on('workerdestroyed', worker =>
 *   console.log('Worker destroyed: ' + worker.url())
 * );
 *
 * console.log('Current workers:');
 * for (const worker of page.workers()) {
 *   console.log('  ' + worker.url());
 * }
 * ```
 *
 * @public
 */
export declare class WebWorker extends EventEmitter {
    #private;
    /* Excluded from this release type: __constructor */
    /* Excluded from this release type: executionContext */
    /**
     * The URL of this web worker.
     */
    url(): string;
    /**
     * The CDP session client the WebWorker belongs to.
     */
    get client(): CDPSession;
    /**
     * If the function passed to the `worker.evaluate` returns a Promise, then
     * `worker.evaluate` would wait for the promise to resolve and return its
     * value. If the function passed to the `worker.evaluate` returns a
     * non-serializable value, then `worker.evaluate` resolves to `undefined`.
     * DevTools Protocol also supports transferring some additional values that
     * are not serializable by `JSON`: `-0`, `NaN`, `Infinity`, `-Infinity`, and
     * bigint literals.
     * Shortcut for `await worker.executionContext()).evaluate(pageFunction, ...args)`.
     *
     * @param pageFunction - Function to be evaluated in the worker context.
     * @param args - Arguments to pass to `pageFunction`.
     * @returns Promise which resolves to the return value of `pageFunction`.
     */
    evaluate<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<Awaited<ReturnType<Func>>>;
    /**
     * The only difference between `worker.evaluate` and `worker.evaluateHandle`
     * is that `worker.evaluateHandle` returns in-page object (JSHandle). If the
     * function passed to the `worker.evaluateHandle` returns a `Promise`, then
     * `worker.evaluateHandle` would wait for the promise to resolve and return
     * its value. Shortcut for
     * `await worker.executionContext()).evaluateHandle(pageFunction, ...args)`
     *
     * @param pageFunction - Function to be evaluated in the page context.
     * @param args - Arguments to pass to `pageFunction`.
     * @returns Promise which resolves to the return value of `pageFunction`.
     */
    evaluateHandle<Params extends unknown[], Func extends EvaluateFunc<Params> = EvaluateFunc<Params>>(pageFunction: Func | string, ...args: Params): Promise<HandleFor<Awaited<ReturnType<Func>>>>;
}

/* Excluded from this release type: withSourcePuppeteerURLIfNone */

/* Excluded from this release type: WorkerTarget */

export { }
