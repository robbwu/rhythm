#include "transpose/transpiler.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "core/core_lib.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "resolver.hpp"
#include "scanner.hpp"
#include "statement.hpp"
#include "transpose/javascript_generator.hpp"

namespace transpose {

namespace {

std::vector<Token> scanSource(const std::string& source) {
    Scanner scanner(source);
    return scanner.scanTokens();
}

}  // namespace

std::string transpileToJavascript(const std::string& source) {
    if (source.empty()) {
        return {};
    }

    auto coreTokens = scanSource(std::string(CORE_LIB_SOURCE));
    Parser coreParser(coreTokens);
    auto coreStatements = coreParser.parse();

    auto userTokens = scanSource(source);
    Parser userParser(userTokens);
    auto userStatements = userParser.parse();

    std::vector<std::unique_ptr<Stmt>> statements;
    statements.reserve(coreStatements.size() + userStatements.size());

    for (auto& stmt : coreStatements) {
        statements.push_back(std::move(stmt));
    }
    for (auto& stmt : userStatements) {
        statements.push_back(std::move(stmt));
    }

    Interpreter interpreter;
    Resolver resolver(&interpreter);
    resolver.resolve(statements);

    JavascriptGenerator generator;
    return generator.generate(statements);
}

}  // namespace transpose

