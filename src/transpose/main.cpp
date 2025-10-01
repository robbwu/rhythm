#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "scanner.hpp"
#include "transpose/javascript_generator.hpp"
#include "version.hpp"

bool noLoop = false;

namespace {

void printUsage() {
    std::cout << "Usage: transpose [options] [script]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "      --emit-js    Print generated JavaScript and exit" << std::endl;
}

void printVersion() {
    std::cout << "transpose version " << CCLOX_VERSION << std::endl;
    std::cout << "Commit: " << GIT_COMMIT_HASH << " (" << GIT_COMMIT_DATE << ") " << GIT_COMMIT_MESSAGE << std::endl;
    std::cout << "Build commit " << GIT_DIRTY_FLAG << std::endl;
    std::cout << "Built: " << BUILD_DATE << std::endl;
}

std::vector<Token> scanSource(const std::string& source) {
    Scanner scanner(source);
    return scanner.scanTokens();
}

std::filesystem::path makeTemporaryScriptPath() {
    auto dir = std::filesystem::temp_directory_path();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<long long> dist;
    for (int attempt = 0; attempt < 8; ++attempt) {
        auto candidate = dir / ("rhythm-transpose-" + std::to_string(dist(gen)) + ".cjs");
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return dir / "rhythm-transpose.cjs";
}

int executeSource(const std::string& source, bool emitJs) {
    auto tokens = scanSource(source);
    Parser parser(tokens);
    auto statements = parser.parse();

    Interpreter interpreter;
    Resolver resolver(&interpreter);
    resolver.resolve(statements);

    transpose::JavascriptGenerator generator;
    auto js = generator.generate(statements);

    if (emitJs) {
        std::cout << js;
        if (!js.empty() && js.back() != '\n') {
            std::cout << '\n';
        }
        return 0;
    }

    auto tempPath = makeTemporaryScriptPath();

    {
        std::ofstream out(tempPath);
        if (!out.is_open()) {
            throw std::runtime_error("Unable to create temporary file for JavaScript output");
        }
        out << js;
    }

    int exitCode = 0;
    std::string command = "node \"" + tempPath.string() + "\"";
    int status = std::system(command.c_str());

    if (status == -1) {
        std::filesystem::remove(tempPath);
        throw std::runtime_error("Failed to invoke Node.js runtime");
    }

#ifndef _WIN32
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exitCode = 128 + WTERMSIG(status);
    } else {
        exitCode = 1;
    }
#else
    exitCode = status;
#endif

    std::error_code ec;
    std::filesystem::remove(tempPath, ec);

    return exitCode;
}

}  // namespace

int main(int argc, char** argv) {
    bool emitJs = false;
    std::string scriptFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage();
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        }
        if (arg == "--emit-js") {
            emitJs = true;
            continue;
        }
        if (!arg.empty() && arg.front() == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
        if (!scriptFile.empty()) {
            std::cerr << "Multiple scripts provided." << std::endl;
            printUsage();
            return 1;
        }
        scriptFile = std::move(arg);
    }

    std::string source;
    try {
        if (!scriptFile.empty()) {
            std::ifstream file(scriptFile);
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << scriptFile << std::endl;
                return 1;
            }
            source.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        } else {
            source.assign((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to read source: " << ex.what() << std::endl;
        return 1;
    }

    if (source.empty()) {
        return 0;
    }

    try {
        return executeSource(source, emitJs);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}
