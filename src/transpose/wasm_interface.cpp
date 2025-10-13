#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string>
#include <stdexcept>

#include "transpose/transpiler.hpp"

bool noLoop = false;

namespace {

std::string compileToJavascript(const std::string& source) {
    try {
        return transpose::transpileToJavascript(source);
    } catch (const std::runtime_error& e) {
        // Re-throw as a JavaScript error with the message
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
        return "";  // Never reached
    } catch (const std::exception& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
        return "";  // Never reached
    }
}

std::string compileToJavascriptUserCodeOnly(const std::string& source) {
    try {
        return transpose::transpileToJavascriptUserCodeOnly(source);
    } catch (const std::runtime_error& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
        return "";  // Never reached
    } catch (const std::exception& e) {
        emscripten::val::global("Error").new_(std::string(e.what())).throw_();
        return "";  // Never reached
    }
}

void setNoLoopFlag(bool value) {
    noLoop = value;
}

}  // namespace

EMSCRIPTEN_BINDINGS(transpose_module) {
    emscripten::function("compile", &compileToJavascript);
    emscripten::function("compileUserCodeOnly", &compileToJavascriptUserCodeOnly);
    emscripten::function("setNoLoop", &setNoLoopFlag);
}

