#pragma once

#include <string>
#include <vector>
#include <cmath>        // For C math functions like std::sin, std::pow, std::isnan, std::isinf
#include <stdexcept>    // For std::runtime_error (though you use your custom RuntimeError)
#include <random>


#include "token.hpp"     // For LoxCallable, Value
#include "exception.hpp" // For RuntimeError

// Forward declaration of Interpreter can be useful if token.hpp doesn't bring it in
// class Interpreter; // LoxCallable might already reference it.

namespace native {

// Namespace for storing the Lox names of the math functions
namespace NativeMathFunctionNames {
    inline constexpr char sin_name[] = "sin";
    inline constexpr char cos_name[] = "cos";
    inline constexpr char tan_name[] = "tan";
    inline constexpr char asin_name[] = "asin";
    inline constexpr char acos_name[] = "acos";
    inline constexpr char atan_name[] = "atan";
    inline constexpr char log_name[] = "log"; // natural log
    inline constexpr char log10_name[] = "log10";
    inline constexpr char sqrt_name[] = "sqrt";
    inline constexpr char exp_name[] = "exp";
    inline constexpr char fabs_name[] = "fabs"; // Or std::abs for doubles
    inline constexpr char floor_name[] = "floor";
    inline constexpr char ceil_name[] = "ceil";
    // For 2-arg functions
    inline constexpr char pow_name[] = "pow";
    inline constexpr char atan2_name[] = "atan2";
    inline constexpr char fmod_name[] = "fmod";
}

template<double (*MathFunc)(double), const char* LoxFuncName>
class NativeMath1ArgCallable : public LoxCallable {
public:
    int arity() override { return 1; }

    Value call(RuntimeContext* /*interpreter*/, std::vector<Value> arguments) override {
        if (arguments.size() != 1) {
            throw RuntimeError({}, std::string(LoxFuncName) + "() expects 1 argument.");
        }
        if (!std::holds_alternative<double>(arguments[0])) {
            throw RuntimeError({}, std::string(LoxFuncName) + "() argument must be a number.");
        }

        double arg = std::get<double>(arguments[0]);
        double result = MathFunc(arg);

        // Standard C math functions might return NaN or Inf for domain/range errors.
        // You might want to handle these explicitly, e.g., by returning nil or throwing.
        // For now, we'll return the double value as is, which Lox's Value can store.
        // if (std::isnan(result) || std::isinf(result)) {
        //     return nullptr; // Or throw RuntimeError
        // }
        return result;
    }

    std::string toString() override {
        return "<native fn " + std::string(LoxFuncName) + ">";
    }
};

template<double (*MathFunc)(double, double), const char* LoxFuncName>
class NativeMath2ArgsCallable : public LoxCallable {
public:
    int arity() override { return 2; }

    Value call(RuntimeContext* /*interpreter*/, std::vector<Value> arguments) override {
        if (arguments.size() != 2) {
            throw RuntimeError({}, std::string(LoxFuncName) + "() expects 2 arguments.");
        }
        if (!std::holds_alternative<double>(arguments[0])) {
            throw RuntimeError({}, std::string(LoxFuncName) + "() first argument must be a number.");
        }
        if (!std::holds_alternative<double>(arguments[1])) {
            throw RuntimeError({}, std::string(LoxFuncName) + "() second argument must be a number.");
        }

        double arg1 = std::get<double>(arguments[0]);
        double arg2 = std::get<double>(arguments[1]);
        double result = MathFunc(arg1, arg2);

        // Similar NaN/Inf handling considerations as in 1-arg version
        return result;
    }

    std::string toString() override {
        return "<native fn " + std::string(LoxFuncName) + ">";
    }
};

inline std::mt19937 rng{std::random_device{}()};

class UniformRandomRealCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 3; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> args) override {
        if (args.size() != arity()) {
            throw RuntimeError({}, "uniform_random_real needs three args");
        }
        if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
            throw RuntimeError({}, "uniform_random_real needs two real numbers and an integer");
        }
        double a = std::get<double>(args[0]);
        double b = std::get<double>(args[1]);
        int n = (int)std::get<double>(args[2]);
        if (n < 1) {
            throw RuntimeError({}, "uniform_random_real(a,b,n): n cannot be less than 1");
        }
        std::uniform_real_distribution<> dis(a, b);
        std::vector<Value> results{}; results.reserve(n);
        for (int i = 0; i < n; i++) {
            results.push_back(dis(rng));
        }
        return std::make_shared<Array>(std::move(results));
    }

    std::string toString()  override { return "<native fn>"; }
};

class UniformRandomIntegerCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 3; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> args) override {
        if (args.size() != arity()) {
            throw RuntimeError({}, "random_int needs three args");
        }
        if (!std::holds_alternative<double>(args[0]) || !std::holds_alternative<double>(args[1])) {
            throw RuntimeError({}, "random_int needs two int numbers and an integer size");
        }
        double a = std::get<double>(args[0]);
        double b = std::get<double>(args[1]);
        double n = (int)std::get<double>(args[2]);
        if (!is_integer(a) || !is_integer(b) || !is_integer(n)) {
            throw RuntimeError({}, "uniform_random_real needs three integer numbers");
        }
        if (a  >= b) {
            throw RuntimeError({}, "uniform_random_real(a,b,n):  a should be less than b");
        }
        if (n < 1) {
            throw RuntimeError({}, "uniform_random_real(a,b,n): n cannot be less than 1");
        }
        std::uniform_int_distribution<> dis((int)a, (int)b);
        std::vector<Value> results{}; results.reserve((int)n);
        for (int i = 0; i < n; i++) {
            results.push_back((double)dis(rng));
        }
        return std::make_shared<Array>(std::move(results));
    }

    std::string toString()  override { return "<native fn>"; }
};

} // namespace cclox
