#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>                 // for std::fixed
#include <variant>
#include <ranges>
#include <string_view>
#include <chrono>


#include "exception.hpp"
#include "token.hpp"

class AssertCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter*, std::vector<Value> args) override {
        if (args.size() != 1) {
            throw RuntimeError({}, "assert() takes 1 argument");
        }
        if (is_truthy(args[0])) {
            return nullptr;
        }
        throw std::runtime_error("assert failed; exit program");

    }

    std::string toString()  override { return "<native fn>"; }
};

class ReadlineCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 0; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter*, std::vector<Value>) override {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    std::string toString()  override { return "<native fn>"; }
};

class SplitCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 2; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter* interp, std::vector<Value> args) override {
        std::vector<Value> results;
        auto &str = std::get<std::string>(args[0]);
        auto &delim = std::get<std::string>(args[1]);
        for (const auto word : std::views::split(str, delim) ){
            results.emplace_back(std::string(std::ranges::begin(word), std::ranges::end(word)));
        }
        return std::make_shared<Array>(results);
    }

    std::string toString()  override { return "<native fn>"; }
};


class ClockCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 0; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter*, std::vector<Value>) override {
        using namespace std::chrono;
        auto now_ms = duration_cast<milliseconds>(
                          system_clock::now().time_since_epoch()).count();
        return static_cast<double>(now_ms)/1000.0;
    }

    std::string toString()  override { return "<native fn>"; }
};



struct PrintfCallable final : public LoxCallable
{
    /*  variable-arity native – we’ll just return -1
        (everywhere else in the interpreter you already special-case that)   */
    int arity() override { return -1; }
    std::string toString() override { return "<native printf>"; }

    Value call(Interpreter*, std::vector<Value> args) override
    {
        if (args.empty())
            throw RuntimeError({}, "printf needs a format string");

        if (!std::holds_alternative<std::string>(args[0]))
            throw RuntimeError({}, "first printf argument must be a string");
        const std::string raw = std::get<std::string>(args[0]);

        auto unescape = [](const std::string& in) -> std::string {
            std::string out;
            out.reserve(in.size());
            for (std::size_t i = 0; i < in.size(); ++i) {
                if (in[i] == '\\' && i + 1 < in.size()) {
                    switch (in[++i]) {
                        case 'n':  out.push_back('\n'); break;
                        case 't':  out.push_back('\t'); break;
                        case 'r':  out.push_back('\r'); break;
                        case '\\': out.push_back('\\'); break;
                        default:   out.append("\\").push_back(in[i]); break;
                    }
                } else {
                    out.push_back(in[i]);
                }
            }
            return out;
        };

        const std::string fmt = unescape(raw);
        std::ostringstream out;
        std::size_t ap = 1;                        // index into args

        auto next = [&](char spec) -> const Value&
        {
            if (ap >= args.size())
                throw RuntimeError({}, "too few arguments for printf");
            return args[ap++];
        };

        for (std::size_t i = 0; i < fmt.size(); ++i)
        {
            if (fmt[i] != '%') { out << fmt[i]; continue; }

            if (++i == fmt.size())
                throw RuntimeError({}, "lone % at end of format string");

            char spec = fmt[i];

            if (spec == '%') { out << '%'; continue; }

            const Value& v = next(spec);

            switch (spec)
            {
            case 'd': case 'i': {                   // integer
                if (!std::holds_alternative<double>(v) &&
                    !std::holds_alternative<bool>(v)) {
                    std::ostringstream oss;
                    oss << "wrong argument" << v;
                    throw RuntimeError({}, std::format("%d expects number/bool; got {}", oss.str()));
                }
                long long n = std::holds_alternative<double>(v)
                               ? static_cast<long long>(std::get<double>(v))
                               : (std::get<bool>(v) ? 1 : 0);
                out << n;
                break;
            }
            case 'f': {                             // floating point
                if (!std::holds_alternative<double>(v))
                    throw RuntimeError({}, "%f expects number");
                out << std::fixed << std::get<double>(v);
                break;
            }
            case 'e': {
                if (!std::holds_alternative<double>(v))
                    throw RuntimeError({}, "%e expects number");
                out << std::scientific << std::get<double>(v);
                break;
            }
            case 's': {                             // string
                if (std::holds_alternative<std::string>(v))
                    out << std::get<std::string>(v);
                else if (std::holds_alternative<std::nullptr_t>(v))
                    out << "nil";
                else
                    out << v;        // the helper you already use for “print”
                break;
            }
            case 'c': {                             // single character
                char ch;
                if (std::holds_alternative<double>(v))
                    ch = static_cast<char>(std::get<double>(v));
                else if (std::holds_alternative<std::string>(v) &&
                         std::get<std::string>(v).size() == 1)
                    ch = std::get<std::string>(v)[0];
                else
                    throw RuntimeError({}, "%c expects char");
                out << ch;
                break;
            }
            default:
                throw RuntimeError({}, std::string("unsupported %") + spec);
            }
        }

        if (ap != args.size())
            throw RuntimeError({}, "too many arguments for printf");

        std::cout << out.str();
        return nullptr;                            // Lox nil
    }
};

class ToNumberCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter*, std::vector<Value> values) override {
        if (values.size() != 1 )
            throw RuntimeError({}, "tonumber() requires 1 argument");
        auto &v = values[0];
        if (std::holds_alternative<double>(v)) return v;
        if (std::holds_alternative<bool>(v)) return (double)(std::get<bool>(v) ? 1 : 0);
        if (std::holds_alternative<std::string>(v)) return std::stod(std::get<std::string>(v));
    }

    std::string toString()  override { return "<native fn>"; }
};
