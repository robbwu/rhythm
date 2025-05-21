#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>                 // for std::fixed
#include <variant>

// ── Native “clock” ──────────────────────────────────────────────────────────────
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
                    !std::holds_alternative<bool>(v))
                    throw RuntimeError({}, "%d expects number/bool");
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