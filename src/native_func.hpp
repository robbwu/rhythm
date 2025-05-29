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
#include "json.hpp"

class AssertCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> args) override {
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
    Value call(RuntimeContext*, std::vector<Value>) override {
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
    Value call(RuntimeContext* interp, std::vector<Value> args) override {
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
    Value call(RuntimeContext*, std::vector<Value>) override {
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

    Value call(RuntimeContext*, std::vector<Value> args) override
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
    Value call(RuntimeContext*, std::vector<Value> values) override {
        if (values.size() != 1 )
            throw RuntimeError({}, "tonumber() requires 1 argument");
        auto &v = values[0];
        if (std::holds_alternative<double>(v)) return v;
        if (std::holds_alternative<bool>(v)) return (double)(std::get<bool>(v) ? 1 : 0);
        if (std::holds_alternative<std::string>(v)) return std::stod(std::get<std::string>(v));
    }

    std::string toString()  override { return "<native fn>"; }
};

// slurp the whole stdin into a single string
class SlurpCallable final : public LoxCallable {
    // zero arguments
    int arity() override { return 0; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> values) override {
        if (values.size() != arity() )
            throw RuntimeError({}, "slurp() requires 0 argument");
        std::string all(std::istreambuf_iterator<char>(std::cin), {});
        return std::move(all);
    }

    std::string toString()  override { return "<native fn>"; }
};


class FromJsonCallable final : public LoxCallable {
public:
    int arity() override { return 1; }
    // read a JSON string into Value
    Value call(RuntimeContext*, std::vector<Value> values) override {
        if (values.size() != 1 )
            throw RuntimeError({}, "from_json() requires 1 argument");
        auto &v = values[0];
        if (!std::holds_alternative<std::string>(v)) {
            throw RuntimeError({}, "from_json() requires string argument");
        }
        auto j = nlohmann::json::parse(std::get<std::string>(v));
        // Recursive function to convert JSON to Value
        std::function<Value(const nlohmann::json&)> convert = [&](const nlohmann::json& json) -> Value {
            if (json.is_null()) {
                return nullptr;
            } else if (json.is_boolean()) {
                return json.get<bool>();
            } else if (json.is_number()) {
                return json.get<double>();
            } else if (json.is_string()) {
                return json.get<std::string>();
            } else if (json.is_array()) {
                std::vector<Value> arr;
                for (const auto& elem : json) {
                    arr.push_back(convert(elem));
                }
                return std::make_shared<Array>(arr);
            } else if (json.is_object()) {
                std::unordered_map<Value, Value> map;
                for (const auto& [key, value] : json.items()) {
                    map[std::string(key)] = convert(value);
                }
                return std::make_shared<Map>(map);
            } else {
                throw RuntimeError({}, "unsupported JSON type");
            }
        };
        return convert(j);
    }

    std::string toString()  override { return "<native fn>"; }
};

class ToJsonCallable final : public LoxCallable {
public:
    int arity() override { return 1; }

    Value call(RuntimeContext*, std::vector<Value> values) override {
        if (values.size() != 1)
            throw RuntimeError({}, "to_json() requires 1 argument");

        auto &v = values[0];

        // Recursive function to convert Value to JSON
        std::function<nlohmann::json(const Value&)> convert = [&](const Value& value) -> nlohmann::json {
            return std::visit(overloaded{
                [&](std::nullptr_t) -> nlohmann::json {
                    return nlohmann::json();
                },
                [&](bool b) -> nlohmann::json {
                    return b;
                },
                [&](double d) -> nlohmann::json {
                    return d;
                },
                [&](const std::string& s) -> nlohmann::json {
                    return s;
                },
                [&](LoxCallable* fn) -> nlohmann::json {
                    throw RuntimeError({}, "cannot serialize function to JSON");
                },
                [&](std::shared_ptr<Array> arr) -> nlohmann::json {
                    nlohmann::json json_arr = nlohmann::json::array();
                    for (const auto& elem : arr->data) {
                        json_arr.push_back(convert(elem));
                    }
                    return json_arr;
                },
                [&](std::shared_ptr<Map> map) -> nlohmann::json {
                    nlohmann::json json_obj = nlohmann::json::object();
                    for (const auto& [key, value] : map->data) {
                        // Convert key to string (JSON object keys must be strings)
                        std::string key_str;
                        std::visit(overloaded{
                            [&](const std::string& s) { key_str = s; },
                            [&](double d) {
                                if (is_integer(d)) {
                                    key_str = std::to_string(static_cast<long long>(d));
                                } else {
                                    key_str = std::to_string(d);
                                }
                            },
                            [&](bool b) { key_str = b ? "true" : "false"; },
                            [&](std::nullptr_t) { key_str = "nil"; },
                            [&](auto&&) {
                                throw RuntimeError({}, "unsupported map key type for JSON serialization");
                            }
                        }, key);
                        json_obj[key_str] = convert(value);
                    }
                    return json_obj;
                }
            }, value);
        };

        nlohmann::json j = convert(v);
        return j.dump();
    }

    std::string toString() override { return "<native fn>"; }
};
