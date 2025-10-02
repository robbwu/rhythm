#pragma once
#include <string_view>

inline constexpr std::string_view CORE_LIB_SOURCE = R"RH(
var core = {
  "max": fun(xs) {
    assert(len(xs) > 0, "core.max() expects a non-empty array");
    var best = xs[0];
    for (var i = 1; i < len(xs); i = i + 1) {
      if (xs[i] > best) best = xs[i];
    }
    return best;
  },

  "min": fun(xs) {
    assert(len(xs) > 0, "core.min() expects a non-empty array");
    var best = xs[0];
    for (var i = 1; i < len(xs); i = i + 1) {
      if (xs[i] < best) best = xs[i];
    }
    return best;
  },

  "copy": fun(o) {
    return from_json(to_json(o));
  },

  "make_array": fun(n, v) {
    var arr = [];
    for (var i = 0; i < n; i = i + 1) {
      push(arr, from_json(to_json(v)));
    }
    return arr;
  }
};
)RH";
