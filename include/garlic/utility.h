#include "layer.h"
#include <algorithm>
#include <cstring>

#ifndef UTILITY_H
#define UTILITY_H

namespace garlic {

  template<garlic::ReadableLayer L1, garlic::ReadableLayer L2>
  bool cmp_layers(const L1& layer1, const L2& layer2) {
    if (layer1.is_int() && layer2.is_int() && layer1.get_int() == layer2.get_int()) return true;
    else if (layer1.is_string() && layer2.is_string() && std::strcmp(layer1.get_cstr(), layer2.get_cstr()) == 0) {
      return true;
    }
    else if (layer1.is_double() && layer2.is_double() && layer1.get_double() == layer2.get_double()) return true;
    else if (layer1.is_bool() && layer2.is_bool() && layer1.get_bool() == layer2.get_bool()) return true;
    else if (layer1.is_null() && layer2.is_null()) return true;
    else if (layer1.is_list() && layer2.is_list()) {
      return std::equal(
          layer1.begin_list(), layer1.end_list(),
          layer2.begin_list(), layer2.end_list(),
          [](const auto& item1, const auto& item2) { return cmp_layers(item1, item2); }
      );
    } else if (layer1.is_object() && layer2.is_object()) {
      return std::equal(
          layer1.begin_member(), layer1.end_member(),
          layer2.begin_member(), layer2.end_member(),
          [](const auto& item1, const auto& item2) {
            return cmp_layers(item1.key, item2.key) && cmp_layers(item1.value, item2.value);
          }
      );
    }
    return false;
  }


}

#endif /* end of include guard: UTILITY_H */
