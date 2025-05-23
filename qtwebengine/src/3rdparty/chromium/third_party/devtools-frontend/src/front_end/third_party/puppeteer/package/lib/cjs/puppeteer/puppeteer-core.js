"use strict";
/**
 * Copyright 2017 Google Inc. All rights reserved.
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
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __exportStar = (this && this.__exportStar) || function(m, exports) {
    for (var p in m) if (p !== "default" && !Object.prototype.hasOwnProperty.call(exports, p)) __createBinding(exports, m, p);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.launch = exports.executablePath = exports.defaultArgs = exports.connect = void 0;
__exportStar(require("./api/api.js"), exports);
__exportStar(require("./common/common.js"), exports);
__exportStar(require("./node/node.js"), exports);
__exportStar(require("./revisions.js"), exports);
__exportStar(require("./util/util.js"), exports);
/**
 * @deprecated Use the query handler API defined on {@link Puppeteer}
 */
__exportStar(require("./common/CustomQueryHandler.js"), exports);
const PuppeteerNode_js_1 = require("./node/PuppeteerNode.js");
/**
 * @public
 */
const puppeteer = new PuppeteerNode_js_1.PuppeteerNode({
    isPuppeteerCore: true,
});
/**
 * @public
 */
exports.connect = puppeteer.connect, 
/**
 * @public
 */
exports.defaultArgs = puppeteer.defaultArgs, 
/**
 * @public
 */
exports.executablePath = puppeteer.executablePath, 
/**
 * @public
 */
exports.launch = puppeteer.launch;
exports.default = puppeteer;
//# sourceMappingURL=puppeteer-core.js.map