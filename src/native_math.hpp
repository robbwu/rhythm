#pragma once
#include <iostream>
#include <string>
#include <vector>

#include "interpreter.hpp"
#include "token.hpp"


class FloorCallable final : public LoxCallable {
    public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> args) override {
        if (args.size() != 1) {
            throw RuntimeError({}, "floor() needs 1 argument");
        }
        auto a = std::get<double>(args[0]);
        return floor(a);
    }

    std::string toString()  override { return "<native fn>"; }
};

class CeilCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 1; }

    // return milli-seconds since Unix epoch, as a double
    Value call(RuntimeContext*, std::vector<Value> args) override {
        if (args.size() != 1) {
            throw RuntimeError({}, "ceil() needs 1 argument");
        }
        auto a = std::get<double>(args[0]);
        return ceil(a);
    }

    std::string toString()  override { return "<native fn>"; }
};
