#include <emscripten/bind.h>

#include <string>

#include "transpose/transpiler.hpp"

bool noLoop = false;

namespace {

std::string compileToJavascript(const std::string& source) {
    return transpose::transpileToJavascript(source);
}

void setNoLoopFlag(bool value) {
    noLoop = value;
}

}  // namespace

EMSCRIPTEN_BINDINGS(transpose_module) {
    emscripten::function("compile", &compileToJavascript);
    emscripten::function("setNoLoop", &setNoLoopFlag);
}

