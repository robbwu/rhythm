#pragma once

#include <string>

namespace transpose {

// Transpile Rhythm source code (including the standard core library) into
// executable JavaScript. The global parser option `noLoop` controls whether
// loop constructs are permitted.
std::string transpileToJavascript(const std::string& source);

}  // namespace transpose

