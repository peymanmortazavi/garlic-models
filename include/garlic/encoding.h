#ifndef GARLIC_ENCODING_H
#define GARLIC_ENCODING_H

/*! \file encoding.h
 */

#include <type_traits>

#include "layer.h"
#include "containers.h"

namespace garlic {

  template<typename Type, typename Layer>
  struct coder {};

  template<typename Layer>
  struct coder<int, Layer> {
    static inline int
    decode(Layer layer) { return layer.get_int(); }

    static inline void
    encode(Layer layer, int value) { layer.set_int(value); }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_int())
        cb(layer.get_int());
    }
  };

  template<typename Layer>
  struct coder<bool, Layer> {
    static inline bool
    decode(Layer layer) { return layer.get_bool(); }

    static inline void
    encode(Layer layer, bool value) { layer.set_bool(value); }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_bool())
        cb(layer.get_bool());
    }
  };

  template<typename Layer>
  struct coder<std::string, Layer> {
    static inline std::string
    decode(Layer layer) { return layer.get_string(); }

    static inline void
    encode(Layer layer, const std::string& value) {
      layer.set_string(value.c_str());
    }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string())
        cb(layer.get_string());
    }
  };

  template<typename Layer>
  struct coder<std::string_view, Layer> {
    static inline std::string_view
    decode(Layer layer) { return layer.get_string_view(); }

    static inline void
    encode(Layer layer, std::string_view value) {
      layer.set_string(value.data());
    }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string())
        cb(layer.get_string_view());
    }
  };

  template<typename Layer>
  struct coder<text, Layer> {
    static inline text
    decode(Layer layer) {
      auto view = layer.get_string_view();
      return text(view.data(), view.size());
    }

    static inline void
    encode(Layer layer, text value) {
      layer.set_string(value.data());
    }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string())
        cb(coder::decode(layer));
    }
  };

  template<typename Layer>
  struct coder<const char*, Layer> {

    static inline const char*
    decode(Layer layer) { return layer.get_cstr(); }

    static inline void
    encode(Layer layer, const char* value) { layer.set_string(value); }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_string())
        cb(layer.get_cstr());
    }
  };

  template<typename Layer>
  struct coder<double, Layer> {
    static inline double
    decode(Layer layer) { return layer.get_double(); }

    static inline void
    encode(Layer layer, double value) {
      layer.set_double(value);
    }

    template<typename Callable>
    static inline void
    safe_decode(Layer layer, Callable&& cb) {
      if (layer.is_double())
        cb(layer.get_double());
    }
  };

  namespace internal {

    template<typename, typename, class = void>
    struct has_explicit_decoder : std::false_type {};

    template<typename Type, typename Layer>
    struct has_explicit_decoder<
      Type, Layer,
      std::void_t<decltype(coder<Type, Layer>::decode(std::declval<Layer>()))>> : std::true_type {};

    template<typename T, typename L>
    using explicit_safe_decode_t = decltype(
        coder<T, L>::safe_decode(std::declval<L>(), std::declval<void(*)(T&&)>()));

    template<typename, typename, class = void>
    struct has_explicit_safe_decoder : std::false_type {};

    template<typename, typename, class = void>
    struct has_explicit_encoder : std::false_type {};

    template<typename Type, typename Layer>
    struct has_explicit_encoder<
      Type, Layer,
      std::void_t<
        decltype(coder<Type, Layer>::encode(std::declval<Layer>(), std::declval<const Type&>()))
        >> : std::true_type {};

    template<typename Type, typename Layer>
    struct has_explicit_safe_decoder<
      Type, Layer,
      std::void_t<internal::explicit_safe_decode_t<Type, Layer>>> : std::true_type {};

    template<typename, typename, class = void>
    struct is_layer_constructible : std::false_type{};

    template<typename Type, typename Layer>
    struct is_layer_constructible<
      Type, Layer,
      std::void_t<decltype(Type(std::declval<Layer>()))>> : std::true_type {};

    template<typename, typename, class = void>
    struct has_decode_layer_method : std::false_type {};

    template<typename Type, typename Layer>
    struct has_decode_layer_method<
      Type, Layer,
      std::void_t<decltype(Type::decode(std::declval<Layer>()))>> : std::true_type {};

    template<typename, typename, class = void>
    struct has_encode_layer_method : std::false_type {};

    template<typename Type, typename Layer>
    struct has_encode_layer_method<
      Type, Layer,
      std::void_t<
        decltype(Type::encode(std::declval<Layer>(), std::declval<const Type&>()))
        >> : std::true_type {};

    template<typename, typename, class = void>
    struct has_safe_decode_layer_method : std::false_type {};

    template<typename Type, typename Layer>
    struct has_safe_decode_layer_method<
      Type, Layer,
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

    template<typename Type, typename Layer>
    constexpr bool use_explicit_encoder = has_explicit_encoder<Type, Layer>::value;

    template<typename Type, typename Layer>
    constexpr bool use_encode_layer_method =
      has_encode_layer_method<Type, Layer>::value
      && !has_explicit_encoder<Type, Layer>::value;
  }

  template<typename Type, GARLIC_VIEW Layer>
  static inline std::enable_if_t<internal::use_decode_layer_method<Type, Layer>, Type>
  decode(Layer layer) { return Type::decode(layer); }

  template<typename Type, GARLIC_VIEW Layer>
  static inline std::enable_if_t<internal::use_decode_constructor<Type, Layer>, Type>
  decode(Layer layer) { return Type(layer); }

  //! Decodes a layer.
  /*!
   *  \tparam Layer any readable layer that conforms to the garlic::ViewLayer concept.
   *  \tparam Type the output type.
   *  \param layer the layer to read from.
   */
  template<typename Type, GARLIC_VIEW Layer>
  static inline std::enable_if_t<internal::use_explicit_decoder<Type, Layer>, Type>
  decode(Layer layer) { return coder<Type, Layer>::decode(layer); }

  template<typename Type, GARLIC_VIEW Layer, typename Callable>
  static inline std::enable_if_t<internal::use_explicit_safe_decoder<Type, Layer>>
  safe_decode(Layer layer, Callable&& cb) {
    coder<Type, Layer>::safe_decode(layer, cb);
  }

  //! Safely attempt to decode a value and if successful, call the callback method.
  /*!
   *  \tparam Layer any readable layer that conforms to the garlic::ViewLayer concept.
   *  \tparam Callable any lambda or callable type.
   *  \param layer the layer to read from.
   *  \param cb the callback function to call with a successfully decoded value.
   */
  template<typename Type, GARLIC_VIEW Layer, typename Callable>
  static inline std::enable_if_t<internal::use_safe_decode_layer_method<Type, Layer>>
  safe_decode(Layer layer, Callable&& cb) {
    Type::safe_decode(layer, cb);
  }

  template<GARLIC_REF Layer, typename Type>
  static inline std::enable_if_t<internal::use_explicit_encoder<Type, Layer>>
  encode(Layer layer, const Type& value) {
    coder<Type, Layer>::encode(layer, value);
  }

  //! Encode a value in a garlic::RefLayer
  /*! If an encoder is defined for the **Type**, it will be used to encode
   *  that value into the layer.
   *
   *  \tparam Layer any writable layer that conforms to the garlic::RefLayer concept.
   *  \param layer the layer to write to.
   *  \param value the value to encode to the layer.
   */
  template<GARLIC_REF Layer, typename Type>
  static inline std::enable_if_t<internal::use_encode_layer_method<Type, Layer>>
  encode(Layer layer, const Type& value) {
    Type::encode(layer, value);
  }

}

#endif /* end of include guard: GARLIC_ENCODING_H */
