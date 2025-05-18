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



void run(Interpreter &interpreter, std::string &source)
{
    auto scanner = new Scanner(source);
    std::vector<Token> tokens = scanner->scanTokens();

    for (auto &token: tokens) {
        std::cout << token.toString() << ", ";
    }
    std::cout << std::endl;

    auto parser = Parser(tokens);
    // auto expr = parser.parseExpr();
    AstPrinter printer;

    auto stmts = parser.parse();
    printer.print(stmts);
    std::cout << std::endl;
    // try {
    //     auto val = interpreter.eval(*expr);
    //     std::visit([&](const auto& x) { std::cout << x; }, val);
    //     std::cout << std::endl;
    // } catch (const std::exception &e) {
    //     std::cout << e.what() << std::endl;
    // }
    interpreter.interpret(stmts);

}

void runFile(Interpreter &interpreter, char *filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + std::string(filepath));
    }
    auto str = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    run(interpreter, str);
}

void runPrompt(Interpreter &interpreter)
{
    std::string line;
    std::cout << "> ";
    for (std::string line; std::getline(std::cin, line);) {
        // std::cout << "You entered: " << line << '\n';
        run(interpreter, line);
        std::cout << "> ";
    }
}


int main(int argc, char **argv) {
    Environment env(nullptr);
    Interpreter interpreter(&env);
    // std::cout << std::format("{} {}", "hello", "world");
    if (argc > 2) {
        std::cout << "Usage: cclox [script]" << std::endl;
        exit(1);
    } else if (argc == 2) {
        runFile(interpreter, argv[1]);
    } else if (argc == 1) {
        runPrompt(interpreter);
    }
}
