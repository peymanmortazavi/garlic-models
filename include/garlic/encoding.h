#ifndef GARLIC_ENCODING_H
#define GARLIC_ENCODING_H

#include "layer.h"
#include <type_traits>

namespace garlic {

  template<typename Type, typename Layer>
  struct coder {};

  /* TODO:  <04-02-21, Peyman> use concept for c++ and onward. */

  template<typename, typename, class = void>
  struct has_explicit_decoder : std::false_type {};

  template<typename Type, typename Layer>
  struct has_explicit_decoder<
    Type,
    Layer,
    std::void_t<decltype(coder<Type, Layer>::decode(std::declval<Layer>()))>> : std::true_type {};

  template<typename, typename, class = void>
  struct is_layer_constructible : std::false_type{};

  template<typename Type, typename Layer>
  struct is_layer_constructible<
    Type,
    Layer,
    std::void_t<decltype(Type(std::declval<Layer>()))>> : std::true_type {};

  template<typename, typename, class = void>
  struct has_decode_layer_method : std::false_type {};

  template<typename Type, typename Layer>
  struct has_decode_layer_method<
    Type,
    Layer,
    std::void_t<decltype(Type::decode(std::declval<Layer>()))>> : std::true_type {};

  namespace internal {
    template<typename Type, typename Layer>
    constexpr bool use_explicit_decoder = has_explicit_decoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_decode_layer_method =
      has_decode_layer_method<Type, Layer>::value
      && !has_explicit_decoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_decode_constructor = 
      is_layer_constructible<Type, Layer>::value
      && !has_decode_layer_method<Type, Layer>::value
      && !has_explicit_decoder<Type, Layer>::value;
  }

  template<typename Type, ViewLayer Layer>
  static inline std::enable_if_t<internal::use_decode_layer_method<Type, Layer>, Type>
  decode(Layer layer) { return Type::decode(layer); }

  template<typename Type, ViewLayer Layer>
  static inline std::enable_if_t<internal::use_decode_constructor<Type, Layer>, Type>
  decode(Layer layer) { return Type(layer); }

  template<typename Type, ViewLayer Layer>
  static inline std::enable_if_t<internal::use_explicit_decoder<Type, Layer>, Type>
  decode(Layer layer) { return coder<Type, Layer>::decode(layer); }

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
