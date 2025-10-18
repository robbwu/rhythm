#!/usr/bin/env node
import crypto from "node:crypto";
import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import process from "node:process";

const INSTANTIATION_LINK_ERROR_PATTERN =
  /function import requires a callable|WebAssembly\.instantiate.*LinkError|Import #0 "env": module is not an object or function/i;

function installBrowserLikeGlobals({ jsPath, buildDir }) {
  const moduleUrl = pathToFileURL(jsPath).href;
  const globalScope = globalThis;

  if (typeof globalScope.window === "undefined") {
    globalScope.window = globalScope;
  }
  globalScope.window.self = globalScope.window;

  if (typeof globalScope.self === "undefined") {
    globalScope.self = globalScope;
  }

  const documentStub = globalScope.document ?? {};
  documentStub.currentScript = documentStub.currentScript ?? { src: moduleUrl };
  documentStub.readyState = documentStub.readyState ?? "complete";
  documentStub.body = documentStub.body ?? { appendChild() {} };
  documentStub.createElement = documentStub.createElement ?? ((tagName) => {
    const element = {
      setAttribute() {},
      addEventListener() {},
      remove() {}
    };
    if (typeof tagName === "string" && tagName.toLowerCase() === "script") {
      element.src = "";
    }
    if (typeof tagName === "string" && tagName.toLowerCase() === "link") {
      element.rel = "";
      element.href = "";
    }
    return element;
  });
  documentStub.getElementsByTagName = documentStub.getElementsByTagName ?? (() => []);
  documentStub.querySelector = documentStub.querySelector ?? (() => null);
  globalScope.document = documentStub;
  globalScope.window.document = globalScope.document;

  if (typeof globalScope.navigator === "undefined") {
    globalScope.navigator = { userAgent: "node.js", language: "en-US" };
  }
  globalScope.window.navigator = globalScope.navigator;

  if (typeof globalScope.location === "undefined") {
    globalScope.location = new URL(pathToFileURL(path.join(buildDir, "index.html")).href);
  } else if (!globalScope.location.href) {
    globalScope.location = new URL(globalScope.location.href, pathToFileURL(buildDir).href);
  }
  globalScope.window.location = globalScope.location;

  if (typeof globalScope.crypto === "undefined" && crypto?.webcrypto) {
    globalScope.crypto = crypto.webcrypto;
  }
}

function isInstantiationLinkError(error) {
  if (!error) {
    return false;
  }
  const message = typeof error === "string" ? error : error.message;
  if (!message) {
    return false;
  }
  return INSTANTIATION_LINK_ERROR_PATTERN.test(message);
}

async function resolveBuildDir(arg) {
  const resolved = path.resolve(arg ?? "build/web");
  await fs.access(resolved);
  return resolved;
}

async function loadFactory(jsPath) {
  const moduleUrl = pathToFileURL(jsPath).href;
  installBrowserLikeGlobals({ jsPath, buildDir: path.dirname(jsPath) });
  const namespace = await import(moduleUrl);
  const factory = namespace?.default ?? namespace;
  if (typeof factory !== "function") {
    throw new Error(`Expected ${jsPath} to export a function, got ${typeof factory}`);
  }
  return factory;
}

async function main() {
  const buildDir = await resolveBuildDir(process.argv[2] ?? "build/web");
  const jsPath = path.join(buildDir, "transpose_wasm.js");
  const wasmPath = path.join(buildDir, "transpose_wasm.wasm");

  await fs.access(jsPath);
  await fs.access(wasmPath);

  const wasmBinary = await fs.readFile(wasmPath);
  const factory = await loadFactory(jsPath);

  let module;
  try {
    module = await factory({
      wasmBinary,
      locateFile(requestedPath) {
        if (requestedPath.endsWith(".wasm")) {
          return wasmPath;
        }
        return path.join(buildDir, requestedPath);
      },
      print(line) {
        if (line) {
          process.stdout.write(`[transpose-wasm stdout] ${line}\n`);
        }
      },
      printErr(line) {
        if (line) {
          process.stderr.write(`[transpose-wasm stderr] ${line}\n`);
        }
      }
    });
  } catch (error) {
    if (isInstantiationLinkError(error)) {
      const hint =
        "The WebAssembly wrapper and binary appear to be out of sync. Rebuild the web bundle (emcmake cmake && ninja transpose_wasm).";
      const wrapped = new Error(hint);
      wrapped.cause = error;
      throw wrapped;
    }
    throw error;
  }

  const sampleSource = 'print("Hello, Rhythm!\n");\n';
  const js = module.compile(sampleSource);
  if (typeof js !== "string" || js.length === 0) {
    throw new Error("compile() returned an unexpected result");
  }
  const userJs = module.compileUserCodeOnly(sampleSource);
  if (typeof userJs !== "string") {
    throw new Error("compileUserCodeOnly() returned an unexpected result");
  }
  module.setNoLoop(false);

  process.stdout.write("transpose_wasm smoke test succeeded.\n");
  process.stdout.write(`Generated JavaScript length: ${js.length}\n`);
}

main().catch((error) => {
  process.stderr.write(`transpose_wasm smoke test failed: ${error.message ?? error}\n`);
  if (error?.stack) {
    process.stderr.write(`${error.stack}\n`);
  }
  process.exitCode = 1;
});
