#include <fstream>
#include <iostream>

#include "ast_printer.hpp"
#include "chunk.hpp"
#include "parser.hpp"
#include "vm.hpp"
#include "version.hpp"
#include "compiler.hpp"

bool printAst = false;
bool noLoop = false;
bool disassemble = false;

void run( VM &vm, Compiler &compiler, std::string &source);


void printVersion() {
    std::cout << "beat version " << CCLOX_VERSION << std::endl;
    std::cout << "Commit: " << GIT_COMMIT_HASH << " (" << GIT_COMMIT_DATE << ") "  << GIT_COMMIT_MESSAGE  <<  std::endl;
    std::cout << "Build commit " << GIT_DIRTY_FLAG << std::endl;
    std::cout << "Built: " << BUILD_DATE << std::endl;
}

void printUsage() {
    std::cout << "Usage: beat [options] [script]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << "  -a, --ast        Print AST before execution" << std::endl;
    std::cout << "  -n, --no-loop    Disable loop constructs (forces recursion)" << std::endl;
    std::cout << "  -d, --disasm     Print disassembled bytecode chunk"    << std::endl;
}

void runFile(VM &vm,  Compiler &compiler, char *script_file) {
    std::ifstream file(script_file);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + std::string(script_file));
    }
    auto str = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    run( vm, compiler, str );
};

void run( VM &vm, Compiler &compiler, std::string &source) {
    auto scanner = new Scanner(source);
    std::vector<Token> tokens = scanner->scanTokens();
    // for (auto &token: tokens) {
    //     std::cout << token.toString() << ", ";
    // }
    // std::cout << std::endl;

    auto parser = Parser(tokens);
    auto stmts = parser.parse();
    if (printAst) {
        AstPrinter printer;
        printer.print(stmts);
        std::cout << std::endl;
    }

    compiler.clear();
    // auto chunk = compiler.compile(std::move(stmts));
    auto script = compiler.compileBeatFunction(std::move(stmts), "", 0, BeatFunctionType::SCRIPT);
    // chunk.write(OP_RETURN, 0); // TODO: remove me
    if (disassemble)
        script->chunk.disassembleChunk("test chunk");

    vm.run(script);
}

void runPrompt(VM &vm, Compiler &compiler)
{
    std::cout << "> ";
    for (std::string line; std::getline(std::cin, line);) {
        run(vm, compiler,  line);
        std::cout << "> ";
    }
}

int main(int argc, char **argv) {


    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        }
        if (std::strcmp(argv[i], "-a") == 0 || std::strcmp(argv[i], "--ast") == 0) {
            printAst = true;
        }
        if (std::strcmp(argv[i], "-d") == 0 || std::strcmp(argv[i], "--disasm") == 0) {
            disassemble = true;
        }
    }

    Compiler compiler;
    VM vm{};

    // Count non-option arguments
    int script_args = 0;
    char* script_file = nullptr;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            script_args++;
            if (script_args == 1) {
                script_file = argv[i];
            }
        }
    }

    if (script_args > 1) {
        std::cout << "Usage: beat [options] [script]" << std::endl;
        return 1;
    } else if (script_args == 1) {
        runFile(vm, compiler, script_file);
    } else {
        runPrompt(vm, compiler);
    }

    return 0;
}
