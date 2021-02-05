#ifndef GARLIC_ENCODING_H
#define GARLIC_ENCODING_H

#include "layer.h"
#include <type_traits>

namespace garlic {

  namespace internal {

    template<typename T>
    struct always_false {
      enum { value = false };
    };

  }

  template<ViewLayer Layer, typename Output>
  static inline Output
  copy_layer(Layer layer) {
    static_assert(
        internal::always_false<Output>::value,
        "OutputType does not support decoding.");
  };

  template<ViewLayer Layer, RefLayer Output>
  static inline void
  copy_layer(Layer layer, Output output) {
    if (layer.is_double()) {
      output.set_double(layer.get_double());
    } else if (layer.is_int()) {
      output.set_int(layer.get_int());
    } else if (layer.is_bool()) {
      output.set_bool(layer.get_bool());
    } else if (layer.is_string()) {
      output.set_string(layer.get_cstr());
    } else if (layer.is_list()) {
      output.set_list();
      for (const auto& item : layer.get_list()) {
          output.push_back_builder(
              [&item](auto ref) { copy_layer(item, ref); }
              );
      }
    } else if (layer.is_object()) {
      output.set_object();
      for (const auto& pair : layer.get_object()) {
        output.add_member_builder(
            pair.key.get_cstr(),
            [&pair](auto ref) { copy_layer(pair.value, ref); }
            );
      }
    } else {
      output.set_null();
    }
  }

}

#endif /* end of include guard: GARLIC_ENCODING_H */
