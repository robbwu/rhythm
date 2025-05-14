#include <format>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ranges>
#include <string>
#include <stdlib.h>

#include "scanner.hpp"
#include "interpreter.hpp"

void run(std::string &source)
{
    auto scanner = new Scanner(source);
    std::vector<Token> tokens = scanner->scanTokens();

    for (auto &token: tokens) {
        std::cout << token.toString() << " ";
    }
}

void runFile(char *filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + std::string(filepath));
    }
    auto str = std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
    run(str);
}

void runPrompt()
{
    std::string line;
    while (1) {
        std::cout << "> ";
        std::getline(std::cin, line);
        std::cout << "You entered: " << line << '\n';
        run(line);
        if (!std::cin.eof()) {
            std::cerr << "Error reading input\n";
            return;
        }
    }
}


int main(int argc, char **argv) {
    // std::cout << std::format("{} {}", "hello", "world");
    if (argc > 2) {
        std::cout << "Usage: cclox [script]" << std::endl;
        exit(1);
    } else if (argc == 2) {
        runFile(argv[1]);
    } else if (argc == 1) {
        runPrompt();
    }
}
