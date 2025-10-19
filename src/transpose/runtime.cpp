#include "transpose/runtime.hpp"

namespace transpose {

std::string runtimePrelude() {
    static const char* const parts[] = {
        R"JS(
const __rt = (() => {
  const globalScope = typeof globalThis !== 'undefined'
    ? globalThis
    : (typeof self !== 'undefined' ? self : this);
  const isNode = typeof process !== 'undefined' && process.versions && process.versions.node;
  const fs = isNode ? require('fs') : null;

  const io = (() => {
    if (isNode) {
      return null;
    }
    const existing = globalScope.__rhythmIO;
    if (existing && typeof existing === 'object') {
      if (!Array.isArray(existing.stdin)) existing.stdin = [];
      if (!Array.isArray(existing.stdout)) existing.stdout = [];
      if (!Array.isArray(existing.stderr)) existing.stderr = [];
      return existing;
    }
    const fresh = { stdin: [], stdout: [], stderr: [] };
    globalScope.__rhythmIO = fresh;
    return fresh;
  })();

  const STDIN_FD = 0;
  let stdinBuffer = '';
  let stdinEOF = false;

  function readChunk() {
    if (!isNode) {
      return 0;
    }
    const chunk = Buffer.alloc(4096);
    let bytesRead = 0;
    try {
      bytesRead = fs.readSync(STDIN_FD, chunk, 0, chunk.length, null);
    } catch (err) {
      if (err && err.code === 'EAGAIN') {
        return 0;
      }
      throw err;
    }
    if (bytesRead === 0) {
      stdinEOF = true;
      return 0;
    }
    stdinBuffer += chunk.toString('utf8', 0, bytesRead);
    return bytesRead;
  }

  function normalizeBrowserInput() {
    if (!io) {
      return;
    }
    if (Array.isArray(io.stdin)) {
      return;
    }
    if (typeof io.stdin === 'string') {
      const trimmed = io.stdin;
      io.stdin = trimmed.length === 0 ? [] : trimmed.split(/\r?\n/);
      return;
    }
    io.stdin = [];
  }

  function readLine() {
    if (!isNode) {
      normalizeBrowserInput();
      if (!io || io.stdin.length === 0) {
        return false;
      }
      const value = io.stdin.shift();
      if (value === false) {
        return false;
      }
      if (value === null || typeof value === 'undefined') {
        return '';
      }
      return String(value);
    }
    while (true) {
      const newlineIndex = stdinBuffer.indexOf('\n');
      if (newlineIndex !== -1) {
        let line = stdinBuffer.slice(0, newlineIndex);
        stdinBuffer = stdinBuffer.slice(newlineIndex + 1);
        if (line.endsWith('\r')) {
          line = line.slice(0, -1);
        }
        return line;
      }
      if (stdinEOF) {
        if (stdinBuffer.length === 0) {
          return false;
        }
        const line = stdinBuffer;
        stdinBuffer = '';
        return line;
      }
      if (readChunk() === 0 && stdinEOF) {
        if (stdinBuffer.length === 0) {
          return false;
        }
        const line = stdinBuffer;
        stdinBuffer = '';
        return line;
      }
    }
  }

  function readAllRemaining() {
    if (!isNode) {
      normalizeBrowserInput();
      if (!io || io.stdin.length === 0) {
        return '';
      }
      const remaining = io.stdin.map((value) => (value === null || typeof value === 'undefined') ? '' : String(value)).join('\n');
      io.stdin.length = 0;
      return remaining;
    }
    let result = '';
    if (stdinBuffer.length > 0) {
      result += stdinBuffer;
      stdinBuffer = '';
    }
    while (!stdinEOF) {
      if (readChunk() === 0) {
        break;
      }
      result += stdinBuffer;
      stdinBuffer = '';
    }
    return result;
  }

  function runtimeError(line, message) {
    const prefix = typeof line === 'number' ? `[line ${line}] ` : '';
    const error = new Error(prefix + message);
    error.__isRhythmError = true;
    return error;
  }

  function emitBrowserStdout(text, options = {}) {
    const appendNewline = Boolean(options.appendNewline);
    const payload = appendNewline ? text + '\n' : text;
    if (!io) {
      if (typeof console !== 'undefined' && console.log) {
        console.log(text);
      }
      return;
    }
    io.stdout.push(payload);
    if (typeof console !== 'undefined' && console.log) {
      console.log(text);
    }
  }

  function emitBrowserStderr(text) {
    if (!io) {
      if (typeof console !== 'undefined' && console.error) {
        console.error(text);
      }
      return;
    }
    io.stderr.push(text);
    if (typeof console !== 'undefined' && console.error) {
      console.error(text);
    }
  }

  function handleError(err) {
    const message = err && err.message ? err.message : String(err);
    if (isNode) {
      if (typeof console !== 'undefined' && console.error) {
        console.error(message);
      }
      if (typeof process !== 'undefined' && process.exit) {
        process.exit(1);
      }
      return;
    }
    emitBrowserStderr(message);
  }

  function isTruthy(value) {
    return !(value === false || value === null);
  }

  function ensureNumber(value, line, message) {
    if (typeof value !== 'number' || Number.isNaN(value)) {
      throw runtimeError(line, message);
    }
    return value;
  }

  function ensureInteger(value, line, message) {
    ensureNumber(value, line, message);
    if (!Number.isInteger(value)) {
      throw runtimeError(line, message);
    }
    return value;
  }

  function attachCallableMetadata(fn, arity, display) {
    Object.defineProperty(fn, '__loxCallable', { value: true, configurable: true });
    Object.defineProperty(fn, '__loxArity', { value: arity, configurable: true });
    Object.defineProperty(fn, '__loxDisplay', { value: display, configurable: true });
    return fn;
  }

  function makeFunction(name, fn, arity) {
    attachCallableMetadata(fn, arity, `<fn ${name}>`);
    Object.defineProperty(fn, '__loxName', { value: name, configurable: true });
    return fn;
  }

  function makeAnonFunction(fn, arity) {
    attachCallableMetadata(fn, arity, '<anonymous fn>');
    return fn;
  }

  function makeNative(name, fn, arity, display = '<native fn>') {
    attachCallableMetadata(fn, arity, display);
    Object.defineProperty(fn, '__loxName', { value: name, configurable: true });
    return fn;
  }

  function makeArray(elements) {
    return [...elements];
  }

  function makeMap(entries) {
    const map = new Map();
    for (const [key, value] of entries) {
      if (value !== null) {
        map.set(key, value);
      }
    }
    Object.defineProperty(map, '__loxMap', { value: true });
    return map;
  }

  function isMap(value) {
    return value instanceof Map && value.__loxMap === true;
  }

  function getProperty(target, name, line) {
    if (!isMap(target)) {
      throw runtimeError(line, 'Only maps can have properties accessed with dot notation');
    }
    return target.has(name) ? target.get(name) : null;
  }

  function getIndex(target, index, line) {
    if (Array.isArray(target)) {
      ensureNumber(index, line, 'array index must be a number');
      ensureInteger(index, line, 'index must be an integer');
      const idx = index;
      if (idx < 0 || idx >= target.length) {
        throw runtimeError(line, `Index out of bounds: ${idx} (size: ${target.length})`);
      }
      return target[idx];
    }
    if (isMap(target)) {
      return target.has(index) ? target.get(index) : null;
    }
    throw runtimeError(line, 'subscript must be of an array or map');
  }

  function setIndex(target, index, value, line) {
    if (Array.isArray(target)) {
      ensureNumber(index, line, 'array index must be a number');
      ensureInteger(index, line, 'index must be an integer');
      const idx = index;
      if (idx < 0 || idx >= target.length) {
        throw runtimeError(line, `Index out of bounds: ${idx} (size: ${target.length})`);
      }
      target[idx] = value;
      return value;
    }
    if (isMap(target)) {
      if (value === null) {
        target.delete(index);
        return null;
      }
      target.set(index, value);
      return value;
    }
    throw runtimeError(line, 'Only arrays and maps can be subscripted.');
  }

  function postfixAdjustVariable(getter, setter, line, delta) {
    const current = getter();
    const numeric = ensureNumber(current, line, 'Postfix operator requires a number');
    const next = numeric + delta;
    setter(next);
    return current;
  }

  function postfixAdjustIndex(target, index, line, delta) {
    if (Array.isArray(target)) {
      ensureNumber(index, line, 'array index must be a number');
      ensureInteger(index, line, 'index must be an integer');
      const idx = index;
      if (idx < 0 || idx >= target.length) {
        throw runtimeError(line, `Index out of bounds: ${idx} (size: ${target.length})`);
      }
      const current = target[idx];
      const numeric = ensureNumber(current, line, 'Postfix operator requires a number');
      const next = numeric + delta;
      target[idx] = next;
      return current;
    }
    if (isMap(target)) {
      if (!target.has(index)) {
        throw runtimeError(line, 'Postfix operator requires an existing numeric value');
      }
      const current = target.get(index);
      const numeric = ensureNumber(current, line, 'Postfix operator requires a number');
      const next = numeric + delta;
      target.set(index, next);
      return current;
    }
    throw runtimeError(line, 'subscript must be of an array or map');
  }

  function isCallable(fn) {
    return typeof fn === 'function' && fn.__loxCallable === true;
  }

  function callFunction(fn, args, line) {
    if (!isCallable(fn)) {
      throw runtimeError(line, 'Can only call functions and classes.');
    }
    const arity = fn.__loxArity;
    if (arity !== -1 && arity !== args.length) {
      throw runtimeError(line, `expected ${arity} arguments but got ${args.length}`);
    }
    return fn(...args);
  }

  function unaryMinus(value, line) {
    return -ensureNumber(value, line, 'operand must be a number');
  }

  function unaryNot(value) {
    return !isTruthy(value);
  }

  function binaryPlus(left, right, line) {
    if (typeof left === 'number' && typeof right === 'number') {
      return left + right;
    }
    if (typeof left === 'string' && typeof right === 'string') {
      return left + right;
    }
    throw runtimeError(line, '+ can only be between two numbers or two strings');
  }

)JS",
        R"JS(  function binaryNumberOp(left, right, line, op, message) {
    return op(ensureNumber(left, line, message), ensureNumber(right, line, message));
  }

  function binaryMinus(left, right, line) {
    return binaryNumberOp(left, right, line, (a, b) => a - b, 'operands must be numbers');
  }

  function binaryDivide(left, right, line) {
    return binaryNumberOp(left, right, line, (a, b) => a / b, 'operands must be numbers');
  }

  function binaryMultiply(left, right, line) {
    return binaryNumberOp(left, right, line, (a, b) => a * b, 'operands must be numbers');
  }

  function binaryMod(left, right, line) {
    const a = ensureNumber(left, line, '% operation is between numbers');
    const b = ensureNumber(right, line, '% operation is between numbers');
    if (!Number.isInteger(a) || !Number.isInteger(b)) {
      throw runtimeError(line, '% operation is between integers');
    }
    return a % b;
  }

  function compareOp(left, right, line, comparator) {
    const l = ensureNumber(left, line, 'operands must be numbers');
    const r = ensureNumber(right, line, 'operands must be numbers');
    return comparator(l, r);
  }

  function greaterThan(left, right, line) {
    return compareOp(left, right, line, (a, b) => a > b);
  }

  function greaterEqual(left, right, line) {
    return compareOp(left, right, line, (a, b) => a >= b);
  }

  function lessThan(left, right, line) {
    return compareOp(left, right, line, (a, b) => a < b);
  }

  function lessEqual(left, right, line) {
    return compareOp(left, right, line, (a, b) => a <= b);
  }

  function logicalAnd(leftThunk, rightThunk) {
    const left = leftThunk();
    if (!isTruthy(left)) {
      return left;
    }
    return rightThunk();
  }

  function logicalOr(leftThunk, rightThunk) {
    const left = leftThunk();
    if (isTruthy(left)) {
      return left;
    }
    return rightThunk();
  }

  function equals(left, right) {
    return left === right;
  }

  function notEquals(left, right) {
    return left !== right;
  }

  function formatValue(value) {
    if (value === null) {
      return 'nil';
    }
    if (typeof value === 'number') {
      return String(value);
    }
    if (typeof value === 'boolean') {
      return value ? 'true' : 'false';
    }
    if (typeof value === 'string') {
      return value;
    }
    if (Array.isArray(value)) {
      if (value.length === 0) {
        return '[]';
      }
      let out = '[';
      for (const element of value) {
        out += formatValue(element) + ', ';
      }
      return out + ']';
    }
    if (isMap(value)) {
      if (value.size === 0) {
        return '{}';
      }
      let out = '{';
      for (const [key, val] of value) {
        out += formatValue(key) + ': ' + formatValue(val) + ', ';
      }
      return out + '}';
    }
    if (isCallable(value)) {
      return value.__loxDisplay || '<fn>';
    }
    return String(value);
  }

  function print(value) {
    const text = formatValue(value);
    if (isNode) {
      console.log(text);
    } else {
      emitBrowserStdout(text, { appendNewline: true });
    }
    return null;
  }

  function unescapeString(raw) {
    let result = '';
    for (let i = 0; i < raw.length; ++i) {
      const ch = raw[i];
      if (ch === '\\' && i + 1 < raw.length) {
        const next = raw[++i];
        switch (next) {
          case 'n':
            result += '\n';
            break;
          case 't':
            result += '\t';
            break;
          case 'r':
            result += '\r';
            break;
          case '\\':
            result += '\\';
            break;
          default:
            result += '\\' + next;
            break;
        }
      } else {
        result += ch;
      }
    }
    return result;
  }

  function formatPrintf(fmtRaw, values) {
    const fmt = unescapeString(fmtRaw);
    let argIndex = 0;
    let out = '';
    for (let i = 0; i < fmt.length; ++i) {
      const ch = fmt[i];
      if (ch !== '%') {
        out += ch;
        continue;
      }
      if (i + 1 >= fmt.length) {
        throw runtimeError(null, 'lone % at end of format string');
      }
      const spec = fmt[++i];
      if (spec === '%') {
        out += '%';
        continue;
      }
      if (argIndex >= values.length) {
        throw runtimeError(null, 'too few arguments for printf');
      }
      const value = values[argIndex++];
      switch (spec) {
        case 'd':
        case 'i': {
          if (typeof value === 'number') {
            out += String(Math.trunc(value));
            break;
          }
          if (typeof value === 'boolean') {
            out += value ? '1' : '0';
            break;
          }
          throw runtimeError(null, '%d expects number/bool');
        }
        case 'f': {
          if (typeof value !== 'number') {
            throw runtimeError(null, '%f expects number');
          }
          out += value.toFixed(6);
          break;
        }
        case 'e': {
          if (typeof value !== 'number') {
            throw runtimeError(null, '%e expects number');
          }
          out += value.toExponential(6);
          break;
        }
        case 's': {
          if (typeof value === 'string') {
            out += value;
          } else if (value === null) {
            out += 'nil';
          } else {
            out += formatValue(value);
          }
          break;
        }
        case 'c': {
          if (typeof value === 'number') {
            out += String.fromCharCode(value);
            break;
          }
          if (typeof value === 'string' && value.length === 1) {
            out += value;
            break;
          }
          throw runtimeError(null, '%c expects char');
        }
        default:
          throw runtimeError(null, `unsupported %${spec}`);
      }
    }
    if (argIndex !== values.length) {
      throw runtimeError(null, 'too many arguments for printf');
    }
    return out;
  }

  function createGlobals() {
    const globals = Object.create(null);

    globals.clock = makeNative('clock', () => Date.now() / 1000, 0);

)JS",
        R"JS(    globals.printf = makeNative('printf', (...args) => {
      if (args.length === 0) {
        throw runtimeError(null, 'printf needs a format string');
      }
      if (typeof args[0] !== 'string') {
        throw runtimeError(null, 'first printf argument must be a string');
      }
      const formatted = formatPrintf(args[0], args.slice(1));
      if (isNode && typeof process !== 'undefined' && process.stdout && typeof process.stdout.write === 'function') {
        process.stdout.write(formatted);
      } else {
        emitBrowserStdout(formatted);
      }
      return null;
    }, -1, '<native printf>');

    globals.sprintf = makeNative('sprintf', (...args) => {
      if (args.length === 0) {
        throw runtimeError(null, 'printf needs a format string');
      }
      if (typeof args[0] !== 'string') {
        throw runtimeError(null, 'first printf argument must be a string');
      }
      return formatPrintf(args[0], args.slice(1));
    }, -1, '<native printf>');

    globals.len = makeNative('len', (value) => {
      if (Array.isArray(value)) {
        return value.length;
      }
      if (isMap(value)) {
        return value.size;
      }
      if (typeof value === 'string') {
        return value.length;
      }
      throw runtimeError(null, 'len() argument must be array or map');
    }, 1);

    globals.push = makeNative('push', (array, value) => {
      if (!Array.isArray(array)) {
        throw runtimeError(null, 'push(array, v) needs array as first argument');
      }
      array.push(value);
      return value;
    }, 2);

    globals.pop = makeNative('pop', (array) => {
      if (!Array.isArray(array)) {
        throw runtimeError(null, 'pop(array): array must be an array');
      }
      if (array.length === 0) {
        throw runtimeError(null, 'pop() from empty array');
      }
      return array.pop();
    }, 1);

    globals.readline = makeNative('readline', () => {
      const line = readLine();
      return line === false ? false : line;
    }, 0);

    globals.slurp = makeNative('slurp', () => {
      return readAllRemaining();
    }, 0);

    globals.split = makeNative('split', (value, delim) => {
      if (typeof value !== 'string' || typeof delim !== 'string') {
        throw runtimeError(null, 'split(string, string) expected');
      }
      return makeArray(value.split(delim));
    }, 2);

    globals.assert = makeNative('assert', (condition, message = null) => {
      if (isTruthy(condition)) {
        return null;
      }
      const extra = message !== null ? ` ${formatValue(message)}` : '';
      throw runtimeError(null, 'assert failed;' + extra);
    }, -1);

    globals.for_each = makeNative('for_each', (map, fn) => {
      if (!isMap(map)) {
        throw runtimeError(null, 'for_each(m, f), m must be a map');
      }
      if (!isCallable(fn)) {
        throw runtimeError(null, 'for_each(m, f), f must be a function');
      }
      if (fn.__loxArity !== -1 && fn.__loxArity !== 2) {
        throw runtimeError(null, 'for_each(m, f), f must take 2 arguments (k,v)');
      }
      for (const [key, value] of map) {
        callFunction(fn, [key, value], null);
      }
      return null;
    }, 2);

    globals.tonumber = makeNative('tonumber', (value) => {
      if (typeof value === 'number') {
        return value;
      }
      if (typeof value === 'boolean') {
        return value ? 1 : 0;
      }
      if (typeof value === 'string') {
        const num = Number(value);
        if (Number.isNaN(num)) {
          throw runtimeError(null, 'tonumber() could not convert string to number');
        }
        return num;
      }
      throw runtimeError(null, 'tonumber() requires 1 argument');
    }, 1);

    globals.keys = makeNative('keys', (map) => {
      if (!isMap(map)) {
        throw runtimeError(null, 'keys(map) requires map argument');
      }
      return makeArray(Array.from(map.keys()));
    }, 1);

    const math1 = [
      ['floor', Math.floor],
      ['ceil', Math.ceil],
      ['sin', Math.sin],
      ['cos', Math.cos],
      ['tan', Math.tan],
      ['asin', Math.asin],
      ['acos', Math.acos],
      ['atan', Math.atan],
      ['log', Math.log],
      ['log10', Math.log10 || ((x) => Math.log(x) / Math.LN10)],
      ['sqrt', Math.sqrt],
      ['exp', Math.exp],
      ['fabs', Math.abs],
    ];
    for (const [name, fn] of math1) {
      globals[name] = makeNative(name, (value) => {
        return fn(ensureNumber(value, null, `${name}() argument must be a number`));
      }, 1);
    }

    globals.pow = makeNative('pow', (a, b) => {
      return Math.pow(ensureNumber(a, null, 'pow() arguments must be numbers'), ensureNumber(b, null, 'pow() arguments must be numbers'));
    }, 2);

    globals.atan2 = makeNative('atan2', (a, b) => {
      return Math.atan2(ensureNumber(a, null, 'atan2() arguments must be numbers'), ensureNumber(b, null, 'atan2() arguments must be numbers'));
    }, 2);

    globals.fmod = makeNative('fmod', (a, b) => {
      const dividend = ensureNumber(a, null, 'fmod() arguments must be numbers');
      const divisor = ensureNumber(b, null, 'fmod() arguments must be numbers');
      return dividend % divisor;
    }, 2);

    globals.from_json = makeNative('from_json', (text) => {
      if (typeof text !== 'string') {
        throw runtimeError(null, 'from_json() requires string argument');
      }
      const data = JSON.parse(text);
      const convert = (value) => {
        if (value === null) {
          return null;
        }
        if (typeof value === 'boolean' || typeof value === 'number' || typeof value === 'string') {
          return value;
        }
        if (Array.isArray(value)) {
          return makeArray(value.map(convert));
        }
        if (typeof value === 'object') {
          const entries = Object.entries(value).map(([k, v]) => [k, convert(v)]);
          return makeMap(entries);
        }
        throw runtimeError(null, 'unsupported JSON type');
      };
      return convert(data);
    }, 1);

    globals.to_json = makeNative('to_json', (value) => {
      const convert = (val) => {
        if (val === null) {
          return null;
        }
)JS",
        R"JS(        if (typeof val === 'boolean' || typeof val === 'number' || typeof val === 'string') {
          return val;
        }
        if (Array.isArray(val)) {
          return val.map(convert);
        }
        if (isMap(val)) {
          const obj = {};
          for (const [key, v] of val) {
            if (typeof key === 'string') {
              obj[key] = convert(v);
            } else if (typeof key === 'number') {
              obj[String(key)] = convert(v);
            } else if (typeof key === 'boolean') {
              obj[key ? 'true' : 'false'] = convert(v);
            } else if (key === null) {
              obj.nil = convert(v);
            } else {
              throw runtimeError(null, 'unsupported map key type for JSON serialization');
            }
          }
          return obj;
        }
        throw runtimeError(null, 'cannot serialize function to JSON');
      };
      return JSON.stringify(convert(value));
    }, 1);

    globals.inf = makeNative('inf', () => Number.POSITIVE_INFINITY, 0);

    globals.substring = makeNative('substring', (text, start, end) => {
      if (typeof text !== 'string') {
        throw runtimeError(null, 'substring() requires string as first argument');
      }
      const s = ensureInteger(start, null, 'substring() indices must be integers');
      const e = ensureInteger(end, null, 'substring() indices must be integers');
      if (s < 0 || e < s || e > text.length) {
        throw runtimeError(null, 'substring() indices out of range');
      }
      return text.slice(s, e);
    }, 3);

    globals.random_int = makeNative('random_int', (a, b, n) => {
      if (typeof a !== 'number' || typeof b !== 'number' || typeof n !== 'number') {
        throw runtimeError(null, 'random_int needs two int numbers and an integer size');
      }
      if (!Number.isInteger(a) || !Number.isInteger(b) || !Number.isInteger(n)) {
        throw runtimeError(null, 'random_int needs three integer numbers');
      }
      if (a >= b) {
        throw runtimeError(null, 'random_int(a,b,n):  a should be less than b');
      }
      if (n < 1) {
        throw runtimeError(null, 'random_int(a,b,n): n cannot be less than 1');
      }
      const results = [];
      for (let i = 0; i < n; ++i) {
        const value = Math.floor(Math.random() * (b - a + 1)) + a;
        results.push(value);
      }
      return makeArray(results);
    }, 3);

    return globals;
  }

  return {
    runtimeError,
    isTruthy,
    unaryMinus,
    unaryNot,
    binaryPlus,
    binaryNumberOp,
    binaryMinus,
    binaryDivide,
    binaryMultiply,
    binaryMod,
    compareOp,
    greaterThan,
    greaterEqual,
    lessThan,
    lessEqual,
    logicalAnd,
    logicalOr,
    equals,
    notEquals,
    makeFunction,
    makeAnonFunction,
    makeNative,
    makeArray,
    makeMap,
    getIndex,
    setIndex,
    postfixAdjustVariable,
    postfixAdjustIndex,
    getProperty,
    callFunction,
    print,
    formatValue,
    handleError,
    readLine,
    readAllRemaining,
    globals: createGlobals(),
  };
})();
)JS",
    };
    std::string prelude;
    prelude.reserve(20936);
    for (const char* part : parts) {
        prelude.append(part);
    }
    return prelude;

}

}  // namespace transpose
