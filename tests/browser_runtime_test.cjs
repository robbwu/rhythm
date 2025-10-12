#!/usr/bin/env node
const { execFileSync } = require('child_process');
const path = require('path');
const vm = require('vm');

function assert(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function normaliseOutput(values) {
  return values.map((value) => {
    if (value === null || typeof value === 'undefined') {
      return '';
    }
    return String(value);
  });
}

function run() {
  if (process.argv.length < 4) {
    console.error('Usage: node browser_runtime_test.cjs <transpose_bin> <source_file>');
    process.exit(2);
  }

  const transposeBin = process.argv[2];
  const sourceFile = process.argv[3];
  const js = execFileSync(transposeBin, ['--emit-js', sourceFile], {
    encoding: 'utf8',
    cwd: path.resolve(__dirname, '..'),
  });

  const stdout = [];
  const stderr = [];

  const context = {
    console: {
      log: (value) => stdout.push(String(value)),
      error: (value) => stderr.push(String(value)),
    },
    __rhythmIO: {
      stdin: [],
      stdout: [],
      stderr: [],
    },
  };

  context.globalThis = context;
  context.window = context;

  vm.createContext(context);
  vm.runInContext(js, context, { filename: 'transpiled_rhythm.js' });

  const capturedStdout = normaliseOutput(context.__rhythmIO.stdout);
  const capturedStderr = normaliseOutput(context.__rhythmIO.stderr);

  assert(capturedStderr.length === 0, `Expected no stderr output, got: ${capturedStderr.join('\n')}`);
  assert(
    capturedStdout.includes('121393'),
    `Expected stdout to include Fibonacci result 121393, got: ${capturedStdout.join(', ')}`,
  );
  assert(
    capturedStdout.includes('OK'),
    `Expected stdout to include "OK", got: ${capturedStdout.join(', ')}`,
  );
}

try {
  run();
} catch (error) {
  console.error(error && error.stack ? error.stack : String(error));
  process.exit(1);
}
