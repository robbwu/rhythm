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
const executionTimeLabel = document.getElementById("executionTime");

let modulePromise = null;
let baselineTryBlockContent = null;

function parseTranspiledStructure(js) {
  if (typeof js !== "string" || js.length === 0) {
    return {
      tryBlockContent: "",
    };
  }

  const tryIndex = js.indexOf("try {");
  if (tryIndex === -1) {
    return {
      tryBlockContent: "",
    };
  }

  const catchIndex = js.indexOf("} catch", tryIndex);
  if (catchIndex === -1) {
    return {
      tryBlockContent: "",
    };
  }

  const blockStart = tryIndex + "try {".length;
  let blockContent = js.slice(blockStart, catchIndex);
  if (blockContent.startsWith("\n")) {
    blockContent = blockContent.slice(1);
  }

  return {
    tryBlockContent: blockContent,
  };
}

function dedentBlock(text) {
  if (!text) {
    return "";
  }

  const lines = text.split("\n");
  let minIndent = Infinity;

  for (const line of lines) {
    const trimmed = line.trim();
    if (trimmed.length === 0) {
      continue;
    }
    const match = line.match(/^[ \t]*/);
    const indent = match ? match[0].length : 0;
    if (indent < minIndent) {
      minIndent = indent;
    }
  }

  if (!Number.isFinite(minIndent)) {
    return text;
  }

  const dedented = lines
    .map((line) => line.slice(Math.min(minIndent, line.length)))
    .join("\n");

  return dedented;
}

function extractUserProgram(js) {
  const { tryBlockContent } = parseTranspiledStructure(js);
  if (!tryBlockContent) {
    return js;
  }

  let userBlock = tryBlockContent;
  if (typeof baselineTryBlockContent === "string" && baselineTryBlockContent.length > 0) {
    if (userBlock.startsWith(baselineTryBlockContent)) {
      userBlock = userBlock.slice(baselineTryBlockContent.length);
    } else {
      const trimmedBaseline = baselineTryBlockContent.trimEnd();
      if (trimmedBaseline.length > 0 && userBlock.startsWith(trimmedBaseline)) {
        userBlock = userBlock.slice(trimmedBaseline.length);
      }
    }
  }

  userBlock = userBlock.replace(/^\n+/, "");
  const dedented = dedentBlock(userBlock);
  const cleaned = dedented.trim();
  if (cleaned.length === 0) {
    return "";
  }

  return cleaned;
}

function ensureBaseline(module) {
  if (baselineTryBlockContent !== null) {
    return;
  }

  try {
    const baselineJs = module.compile(" ");
    const { tryBlockContent } = parseTranspiledStructure(baselineJs);
    baselineTryBlockContent = tryBlockContent || "";
  } catch (error) {
    baselineTryBlockContent = "";
  }
}

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
    modulePromise = TransposeModule().then((module) => {
      ensureBaseline(module);
      return module;
    });
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

    const source = sourceInput.value;
    if (!source.trim()) {
      jsOutput.value = "";
      setStatus("No Rhythm source code to compile.", "info");
      return;
    }

    const js = module.compile(source);
    const userJs = extractUserProgram(js);
    jsOutput.value = userJs;

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

