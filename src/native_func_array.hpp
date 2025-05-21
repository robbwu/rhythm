#pragma once
#include <memory>
#include <variant>

#include "token.hpp"

// len(array)
class LenCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter* interp, std::vector<Value> arguments) override {
        if (arguments.size() != 1) {
            throw RuntimeError({}, "len() needs a single argument");
        }
        Value &x = arguments[0];
        return (double) std::get<std::shared_ptr<Array>>(x)->data.size();
    }

    std::string toString()  override { return "<native fn>"; }
};

// at(array, i)
class AtCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 2; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter* interp, std::vector<Value> arguments) override {
        if (arguments.size() != 2) {
            throw RuntimeError({}, "at(array, i) needs two argument");
        }
        auto x = std::get<std::shared_ptr<Array>>(arguments[0]);
        int i = std::get<double>(arguments[1]);
        return x->data.at(i);
    }

    std::string toString()  override { return "<native fn>"; }
};

// set(array, i, val ) means `array[i] = val`
class SetCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 3; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter* interp, std::vector<Value> arguments) override {
        if (arguments.size() != 3) {
            throw RuntimeError({}, "set(array, i, v) needs 3 argument");
        }
        auto x = std::get<std::shared_ptr<Array>>(arguments[0]);
        int i = std::get<double>(arguments[1]);
        auto &v = arguments[2];
        return x->data.at(i) = v;
    }

    std::string toString()  override { return "<native fn>"; }
};

// push(array, v): push v to the back of the array; size grows by 1
class PushCallable final : public LoxCallable {
    public:
    // zero arguments
    int arity() override { return 2; }

    // return milli-seconds since Unix epoch, as a double
    Value call(Interpreter* interp, std::vector<Value> arguments) override {
        if (arguments.size() != 2) {
            throw RuntimeError({}, "push(array, v) needs two argument");
        }
        auto x = std::get<std::shared_ptr<Array>>(arguments[0]);

        auto &v = arguments[1];
        x->data.push_back(v);
        return v;
    }

    std::string toString()  override { return "<native fn>"; }
};
