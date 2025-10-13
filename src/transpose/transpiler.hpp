#pragma once

#include <string>

namespace transpose {

// Transpile Rhythm source code (including the standard core library) into
// executable JavaScript. The global parser option `noLoop` controls whether
// loop constructs are permitted.
std::string transpileToJavascript(const std::string& source);

// Transpile Rhythm source code to JavaScript, but return only the user's code
// without the runtime and core library. This is useful for displaying transpiled
// code in a readable format.
std::string transpileToJavascriptUserCodeOnly(const std::string& source);

}  // namespace transpose

