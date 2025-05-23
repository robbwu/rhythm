#include <format>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ranges>
#include <string>
#include <stdlib.h>

#include "ast_printer.hpp"
#include "scanner.hpp"
#include "interpreter.hpp"
#include "expr.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "version.hpp"
bool printAst = false;

void printVersion() {
    std::cout << "cclox version " << CCLOX_VERSION << std::endl;
    std::cout << "Commit: " << GIT_COMMIT_HASH << " (" << GIT_COMMIT_DATE << ") "  << GIT_COMMIT_MESSAGE  <<  std::endl;
    std::cout << "Build commit " << GIT_DIRTY_FLAG << std::endl;
    std::cout << "Built: " << BUILD_DATE << std::endl;
}

void printUsage() {
    std::cout << "Usage: cclox [options] [script]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << "  -a, --ast        Print AST before execution" << std::endl;
}

void run(Interpreter &interpreter, Resolver &resolver, std::string &source)
{
    auto scanner = new Scanner(source);
    std::vector<Token> tokens = scanner->scanTokens();

    // for (auto &token: tokens) {
    //     std::cout << token.toString() << ", ";
    // }
    // std::cout << std::endl;

    auto parser = Parser(tokens);
    // auto expr = parser.parseExpr();
    // AstPrinter printer;



    auto stmts = parser.parse();
    // printer.print(stmts);
    // std::cout << std::endl;
    // try {
    //     auto val = interpreter.eval(*expr);
    //     std::visit([&](const auto& x) { std::cout << x; }, val);
    //     std::cout << std::endl;
    // } catch (const std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }

    if (printAst) {
        AstPrinter printer;
        printer.print(stmts);
        std::cout << std::endl;
    }

    resolver.resolve(stmts);
    interpreter.interpret(stmts);

}

void runFile(Interpreter &interpreter,  Resolver &resolver, char *filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + std::string(filepath));
    }
    auto str = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    run(interpreter, resolver, str );
}

void runPrompt(Interpreter &interpreter, Resolver &resolver)
{
    std::string line;
    std::cout << "> ";
    for (std::string line; std::getline(std::cin, line);) {
        run(interpreter,resolver,  line);
        std::cout << "> ";
    }
}


int main(int argc, char **argv) {
    Interpreter interpreter;
    Resolver resolver(&interpreter);
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage();
            return 0;
        } else if (std::strcmp(argv[i], "-a") == 0 || std::strcmp(argv[i], "--ast") == 0) {
            printAst = true;
        }else if (argv[i][0] == '-') {
            std::cout << "Unknown option: " << argv[i] << std::endl;
            printUsage();
            return 1;
        }
    }

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
        std::cout << "Usage: cclox [options] [script]" << std::endl;
        return 1;
    } else if (script_args == 1) {
        runFile(interpreter, resolver, script_file);
    } else {
        runPrompt(interpreter, resolver);
    }

    return 0;
}
