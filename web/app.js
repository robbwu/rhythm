import TransposeModule from "./transpose_wasm.js";

const sourceInput = document.getElementById("source");
const stdinInput = document.getElementById("stdinInput");
const noLoopToggle = document.getElementById("noLoop");
const compileButton = document.getElementById("compile");
const compileRunButton = document.getElementById("compileRun");
const jsOutput = document.getElementById("jsOutput");
const stdoutBlock = document.getElementById("stdout");
const stderrBlock = document.getElementById("stderr");
const stderrSection = document.getElementById("stderrSection");
const statusBar = document.getElementById("status");

let modulePromise = null;

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

function clearOutputs() {
  stdoutBlock.textContent = "";
  setStderrOutput("");
}

async function loadModule() {
  if (!modulePromise) {
    modulePromise = TransposeModule();
  }
  return modulePromise;
}

async function compileSource({ run }) {
  setButtonsDisabled(true);
  clearOutputs();

  try {
    const module = await loadModule();
    module.setNoLoop(noLoopToggle.checked);

    const source = sourceInput.value;
    if (!source.trim()) {
      jsOutput.value = "";
      setStatus("No Rhythm source code to compile.", "info");
      return;
    }

    const js = module.compile(source);
    jsOutput.value = js;

    if (!run) {
      setStatus("Compilation succeeded.", "success");
      return;
    }

    await executeJavascript(js);
  } catch (error) {
    const message = error && error.message ? error.message : String(error);
    setStatus(`Compilation failed: ${message}`, "error");
    setStderrOutput(message);
  } finally {
    setButtonsDisabled(false);
  }
}

async function executeJavascript(js) {
  setStatus("Running program&hellip;", "info");

  const stdin = normaliseInput(stdinInput.value);
  window.__rhythmIO = { stdin: [...stdin], stdout: [], stderr: [] };

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
  }
}

function flushOutputs(io) {
  const stdoutLines = Array.isArray(io.stdout) ? io.stdout : [];
  const stderrLines = Array.isArray(io.stderr) ? io.stderr : [];

  stdoutBlock.textContent = stdoutLines.join("");
  setStderrOutput(stderrLines.join("\n"));
}

function initialiseUI() {
  sourceInput.value = `print("Hello, Rhythm!");\n`;
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

