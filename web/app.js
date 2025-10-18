import { EditorView, basicSetup } from "codemirror";
import { javascript } from "@codemirror/lang-javascript";
import { keymap } from "@codemirror/view";
import { indentWithTab } from "@codemirror/commands";

const editorContainer = document.getElementById("editor");
const stdinInput = document.getElementById("stdinInput");
const noLoopToggle = document.getElementById("noLoop");
const compileButton = document.getElementById("compile");
const compileRunButton = document.getElementById("compileRun");
const jsOutput = document.getElementById("jsOutput");
const stdoutBlock = document.getElementById("stdout");
const stderrBlock = document.getElementById("stderr");
const stderrSection = document.getElementById("stderrSection");
const statusBar = document.getElementById("status");
const executionTimeLabel = document.getElementById("executionTime");

const buildVersion = (() => {
  if (typeof document === "undefined") {
    return `${Date.now()}`;
  }

  const fromMeta = document.querySelector("meta[name='rhythm-build']")?.content;
  if (fromMeta && fromMeta.trim().length > 0) {
    return fromMeta.trim();
  }

  const scriptEl = document.querySelector("script[type='module'][src*='app.js']");
  if (scriptEl) {
    try {
      const scriptUrl = new URL(scriptEl.src, window.location.href);
      const fromQuery = scriptUrl.searchParams.get("v");
      if (fromQuery && fromQuery.trim().length > 0) {
        return fromQuery.trim();
      }
    } catch (error) {
      // Ignore malformed URLs and continue to other fallbacks.
    }
  }

  const lastModified = document.lastModified;
  if (lastModified && lastModified.length > 0) {
    const numeric = lastModified.replace(/\D/g, "");
    if (numeric.length > 0) {
      return numeric;
    }
  }

  return `${Date.now()}`;
})();

const transposeBaseUrl = new URL("./", import.meta.url);
let modulePromise = null;
let transposeFactoryPromise = null;
const INSTANTIATION_LINK_ERROR_PATTERN =
  /function import requires a callable|WebAssembly\.instantiate.*LinkError|Import #0 "env": module is not an object or function/i;
const DYNAMIC_IMPORT_FAILURE_PATTERN =
  /Failed to fetch dynamically imported module|Importing a module script failed|Failed to load module script/i;

function versionedAssetUrl(relativePath) {
  const url = new URL(relativePath, transposeBaseUrl);
  if (buildVersion && buildVersion.length > 0) {
    url.searchParams.set("v", buildVersion);
  }
  return url;
}

async function loadTransposeFactory() {
  if (!transposeFactoryPromise) {
    transposeFactoryPromise = (async () => {
      const moduleUrl = versionedAssetUrl("transpose_wasm.js");
      try {
        const namespace = await import(moduleUrl.href);
        const factory = namespace?.default ?? namespace;
        if (typeof factory !== "function") {
          throw new Error(
            "transpose_wasm.js did not export a WebAssembly factory function."
          );
        }
        return factory;
      } catch (error) {
        transposeFactoryPromise = null;
        if (isDynamicImportFailure(error)) {
          const help =
            "Failed to load the WebAssembly wrapper. Please hard-refresh the page (Shift+Reload) to clear cached files.";
          const wrapped = new Error(help);
          wrapped.cause = error;
          throw wrapped;
        }
        throw error;
      }
    })();
  }
  return transposeFactoryPromise;
}

function isDynamicImportFailure(error) {
  if (!error) {
    return false;
  }
  const message = typeof error === "string" ? error : error.message;
  if (!message) {
    return false;
  }
  return DYNAMIC_IMPORT_FAILURE_PATTERN.test(message);
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

let editorView = null;

// Emacs-style keybindings
const emacsKeymap = keymap.of([
  indentWithTab,
  {
    key: "Ctrl-a",
    run: (view) => {
      const selection = view.state.selection.main;
      const line = view.state.doc.lineAt(selection.head);
      view.dispatch({
        selection: { anchor: line.from }
      });
      return true;
    }
  },
  {
    key: "Ctrl-e",
    run: (view) => {
      const selection = view.state.selection.main;
      const line = view.state.doc.lineAt(selection.head);
      view.dispatch({
        selection: { anchor: line.to }
      });
      return true;
    }
  },
  {
    key: "Ctrl-k",
    run: (view) => {
      const selection = view.state.selection.main;
      const line = view.state.doc.lineAt(selection.head);
      const from = selection.head;
      const to = selection.head === line.to ? Math.min(line.to + 1, view.state.doc.length) : line.to;
      view.dispatch({
        changes: { from, to }
      });
      return true;
    }
  }
]);

function setStatus(message, tone = "info") {
  statusBar.textContent = message;
  statusBar.dataset.tone = tone;
}

function setButtonsDisabled(disabled) {
  compileButton.disabled = disabled;
  compileRunButton.disabled = disabled;
}

function normaliseInput(text) {
  if (!text) {
    return [];
  }
  const normalised = text.replace(/\r\n/g, "\n");
  return normalised.split("\n");
}

function setStderrOutput(text) {
  const content = text || "";
  stderrBlock.textContent = content;

  if (stderrSection) {
    const hasContent = content.trim().length > 0;
    stderrSection.classList.toggle("hidden", !hasContent);
  }
}

function setExecutionTime(durationMs) {
  if (!executionTimeLabel) {
    return;
  }

  if (typeof durationMs !== "number" || !Number.isFinite(durationMs)) {
    executionTimeLabel.textContent = "";
    executionTimeLabel.classList.add("hidden");
    return;
  }

  executionTimeLabel.textContent = `Execution time: ${durationMs.toFixed(2)} ms`;
  executionTimeLabel.classList.remove("hidden");
}

function clearOutputs() {
  stdoutBlock.textContent = "";
  setStderrOutput("");
}

async function loadModule() {
  if (!modulePromise) {
    modulePromise = (async () => {
      try {
        const factory = await loadTransposeFactory();
        return await factory({
          locateFile(path) {
            const url = versionedAssetUrl(path);
            return url.href;
          }
        });
      } catch (error) {
        modulePromise = null;
        if (isInstantiationLinkError(error)) {
          const help =
            "WebAssembly module failed to load. Please hard-refresh the page (Shift+Reload) to clear cached files.";
          const wrapped = new Error(help);
          wrapped.cause = error;
          throw wrapped;
        }
        throw error;
      }
    })();
  }
  return modulePromise;
}

async function compileSource({ run }) {
  setButtonsDisabled(true);
  clearOutputs();
  setExecutionTime(null);

  try {
    const module = await loadModule();
    module.setNoLoop(noLoopToggle.checked);

    const source = editorView.state.doc.toString();
    if (!source.trim()) {
      jsOutput.value = "";
      setStatus("No Rhythm source code to compile.", "info");
      return;
    }

    const js = module.compile(source);

    // For display purposes, show only the user's code without runtime bloat
    const userCode = module.compileUserCodeOnly(source);
    jsOutput.value = userCode;

    if (!run) {
      setStatus("Compilation succeeded.", "success");
      setExecutionTime(null);
      return;
    }

    await executeJavascript(js);
  } catch (error) {
    const message = error && error.message ? error.message : String(error);
    setStatus(`Compilation failed: ${message}`, "error");
    setStderrOutput(message);
    setExecutionTime(null);
  } finally {
    setButtonsDisabled(false);
  }
}

async function executeJavascript(js) {
  setStatus("Running program&hellip;", "info");
  setExecutionTime(null);

  const stdin = normaliseInput(stdinInput.value);
  window.__rhythmIO = { stdin: [...stdin], stdout: [], stderr: [] };
  const start = performance.now();

  try {
    const runner = new Function(`${js}\n//# sourceURL=rhythm_transpiled.js`);
    runner();

    flushOutputs(window.__rhythmIO);
    const hasErrors = window.__rhythmIO.stderr.length > 0;
    setStatus(
      hasErrors
        ? "Program finished with runtime errors."
        : "Program executed successfully.",
      hasErrors ? "error" : "success"
    );
  } catch (error) {
    const io = window.__rhythmIO || { stdout: [], stderr: [] };
    flushOutputs(io);
    const stderrMessage = io.stderr.length
      ? io.stderr.join("\n")
      : error.message || String(error);
    setStderrOutput(stderrMessage);
    setStatus("Program terminated with an exception.", "error");
  } finally {
    const elapsed = performance.now() - start;
    setExecutionTime(elapsed);
  }
}

function flushOutputs(io) {
  const stdoutLines = Array.isArray(io.stdout) ? io.stdout : [];
  const stderrLines = Array.isArray(io.stderr) ? io.stderr : [];

  stdoutBlock.textContent = stdoutLines.join("");
  setStderrOutput(stderrLines.join("\n"));
}

function initialiseUI() {
  // Initialize CodeMirror editor
  editorView = new EditorView({
    doc: 'print("Hello, Rhythm!");',
    extensions: [
      basicSetup,
      javascript(),
      emacsKeymap,
      EditorView.domEventHandlers({
        keydown(event, view) {
          if (event.key === "Tab") {
            event.preventDefault();
            return false;
          }
        }
      })
    ],
    parent: editorContainer
  });

  stdinInput.value = "";
  setButtonsDisabled(true);

  loadModule()
    .then(() => {
      setStatus("Transpiler ready.", "success");
      setButtonsDisabled(false);
    })
    .catch((error) => {
      setStatus(
        `Failed to initialise WebAssembly module: ${error.message || error}`,
        "error"
      );
    });

  compileButton.addEventListener("click", () => compileSource({ run: false }));
  compileRunButton.addEventListener("click", () => compileSource({ run: true }));
}

initialiseUI();

