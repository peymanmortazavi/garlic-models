#ifndef GARLIC_ENCODING_H
#define GARLIC_ENCODING_H

#include "layer.h"
#include <type_traits>

namespace garlic {

  template<typename Type, typename Layer>
  struct coder {};

  template<typename Layer>
  struct coder<int, Layer> {
    static inline int decode(Layer layer) { return layer.get_int(); }

    template<typename Callable>
    static inline void safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_int()) cb(layer.get_int());
    }
  };

  template<typename Layer>
  struct coder<std::string, Layer> {
    static inline std::string decode(Layer layer) { return layer.get_string(); }

    template<typename Callable>
    static inline void safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string()) cb(layer.get_string());
    }
  };

  template<typename Layer>
  struct coder<std::string_view, Layer> {
    static inline std::string_view decode(Layer layer) { return layer.get_string_view(); }

    template<typename Callable>
    static inline void safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string()) cb(layer.get_string_view());
    }
  };

  template<typename Layer>
  struct coder<const char*, Layer> {
    static inline const char* decode(Layer layer) { return layer.get_cstr(); }

    template<typename Callable>
    static inline void safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string()) cb(layer.get_cstr());
    }
  };

  template<typename Layer>
  struct coder<double, Layer> {
    static inline double decode(Layer layer) { return layer.get_double(); }

    template<typename Callable>
    static inline void safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_double()) cb(layer.get_double());
    }
  };

  namespace internal {

    template<typename, typename, class = void>
    struct has_explicit_decoder : std::false_type {};

    template<typename Type, typename Layer>
    struct has_explicit_decoder<
      Type,
      Layer,
      std::void_t<decltype(coder<Type, Layer>::decode(std::declval<Layer>()))>> : std::true_type {};

    template<typename T, typename L>
    using explicit_safe_decode_t = decltype(
        coder<T, L>::safe_decode(std::declval<L>(), std::declval<void(*)(T&&)>()));

    template<typename, typename, class = void>
    struct has_explicit_safe_decoder : std::false_type {};

    template<typename Type, typename Layer>
    struct has_explicit_safe_decoder<
      Type,
      Layer,
      std::void_t<internal::explicit_safe_decode_t<Type, Layer>>> : std::true_type {};

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

    template<typename, typename, class = void>
    struct has_safe_decode_layer_method : std::false_type {};

    template<typename Type, typename Layer>
    struct has_safe_decode_layer_method<
      Type,
      Layer,
      std::void_t<
        decltype(Type::safe_decode(std::declval<Layer>(), std::declval<void(*)(Type&&)>()))>
      > : std::true_type {};

    template<typename Type, typename Layer>
    constexpr bool use_explicit_decoder = has_explicit_decoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_explicit_safe_decoder = has_explicit_safe_decoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_decode_layer_method =
      has_decode_layer_method<Type, Layer>::value
      && !has_explicit_decoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_safe_decode_layer_method =
      has_safe_decode_layer_method<Type, Layer>::value
      && !has_explicit_safe_decoder<Type, Layer>::value;

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

  template<typename Type, ViewLayer Layer, typename Callable>
  static inline std::enable_if_t<internal::use_explicit_safe_decoder<Type, Layer>>
  safe_decode(Layer layer, Callable&& cb) {
    coder<Type, Layer>::safe_decode(layer, cb);
  }

  template<typename Type, ViewLayer Layer, typename Callable>
  static inline std::enable_if_t<internal::use_safe_decode_layer_method<Type, Layer>>
  safe_decode(Layer layer, Callable&& cb) {
    Type::safe_decode(layer, cb);
  }

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
