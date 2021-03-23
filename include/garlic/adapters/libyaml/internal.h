#ifndef GARLIC_LIBYAML_INTERNAL_H
#define GARLIC_LIBYAML_INTERNAL_H

#include "../../parsing/numbers.h"
#include "../../layer.h"

#include "yaml.h"

namespace garlic::adapters::libyaml::internal {

  // Get the leading zero position in a string of format *.*0*
  static inline int leading_zero_position(char* buffer, int size) {
    while (buffer[--size] == '0');
    ++size;
    if (buffer[size] == '.') ++size;
    return size;
  }

  static bool parse_bool(const char* data, bool& output) {
    struct pair {
      const char* name;
      bool value;
    };
    static const pair pairs[8] = {
      {"y", true}, {"n", false},
      {"true", true}, {"false", false},
      {"on", true}, {"off", false},
      {"yes", true}, {"no", false},
    };
    for (const auto& it : pairs) {
      if (strcmp(it.name, data) == 0) {
        output = it.value;
        return true;
      }
    }
    return false;
  }

  // Write to the layer with the scalar value of the data.
  template<GARLIC_REF Layer>
  static inline void
  set_plain_scalar_value(Layer&& layer, const char* data) {
    int i;
    if (parsing::ParseInt(data, i)) {
      layer.set_int(i);
      return;
    }
    double d;
    if (parsing::ParseDouble(data, d)) {
      layer.set_double(d);
      return;
    }
    bool b;
    if (parse_bool(data, b)) {
      layer.set_bool(b);
      return;
    }
    if (strcmp(data, "null") == 0) {
      layer.set_null();
      return;
    }
    layer.set_string(data);
  }


}

#endif /* end of include guard: GARLIC_LIBYAML_INTERNAL_H */
