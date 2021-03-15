/*!
 * \file utility.h
 * \brief This file contains various functions and classes to work with layers like getting or
 *        resolving using a path.
 */

#ifndef GARLIC_UTILITY_H
#define GARLIC_UTILITY_H

#include <charconv>
#include <string_view>

#include "meta.h"
#include "encoding.h"


namespace garlic {

  template<GARLIC_VIEW L1, GARLIC_VIEW L2>
  static inline std::enable_if_t<!is_comparable<L1, L2>::value, bool>
  cmp_layers(const L1& layer1, const L2& layer2) {
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

  template<GARLIC_VIEW Layer1, GARLIC_VIEW Layer2>
  static inline std::enable_if_t<is_comparable<Layer1, Layer2>::value, bool>
  cmp_layers(const Layer1& layer1, const Layer2& layer2) {
    return layer1 == layer2;
  }


  //! Lazy string splitter for getting tokens one by one.
  /*! A more details
   * \note This is a note.
  */
  class lazy_string_splitter {
  public:
    using const_iterator = std::string_view::const_iterator;
    lazy_string_splitter(std::string_view text) : text_(text), cursor_(text.begin()) {}

    template<typename Callable>
    void for_each(Callable&& cb) {
      std::string_view part = this->next();
      while (!part.empty()) {
        cb(part);
        part = this->next();
      }
    }

    std::string_view next() {
      bool found_word = false;
      auto it = cursor_;
      for(; it < text_.end(); it++) {
        if (*it == '.') {
          if (found_word) return get_substr(it);
          else cursor_ = it;
        } else {
          if (!found_word) cursor_ = it;
          found_word = true;
        }
      }
      if (found_word) return get_substr(it);
      return {};
    }
    
  private:
    std::string_view text_;
    const_iterator cursor_;

    std::string_view get_substr(const const_iterator& it) {
      auto old_cursor = cursor_;
      cursor_ = it;
      return std::string_view{old_cursor, it};
    }
  };


  template<GARLIC_VIEW LayerType, typename Callable>
  static inline void
  resolve_layer_cb(const LayerType& value, std::string_view path, Callable&& cb) {
    lazy_string_splitter parts{path};
    auto cursor = std::make_unique<LayerType>(value);
    while (true) {
      auto part = parts.next();
      if (part.empty()) {
        cb(*cursor);
        return;
      }
      if (cursor->is_object()) {
        bool found = false;
        get_member(*cursor, part, [&cursor, &found](const auto& result) {
            cursor = std::make_unique<LayerType>(result);
            found = true;
            });
        if (!found) return;
      } else if (cursor->is_list()) {
        size_t position;
        if (std::from_chars(part.begin(), part.end(), position).ec == std::errc::invalid_argument)
          return;
        bool found = false;
        get_item(*cursor, position, [&cursor, &found](const auto& result) {
            cursor = std::make_unique<LayerType>(result);
            found = true;
            });
        if (!found) return;
      } else return;
    }
  }


  template<typename OutputType, GARLIC_VIEW Layer>
  static inline OutputType
  safe_resolve(const Layer& value, std::string_view key, OutputType&& default_value) {
    resolve_layer_cb(value, key, [&default_value](const auto& result) {
        safe_decode<OutputType, Layer>(result, [&default_value](auto&& result){
            default_value = result;
            });
        });
    return default_value;
  }


  template<typename OutputType, GARLIC_VIEW Layer, typename Callable>
  static inline void
  safe_resolve_cb(const Layer& value, std::string_view key, Callable&& cb) {
    resolve_layer_cb(value, key, [&cb](const auto& result) {
        safe_decode<OutputType, Layer>(result, cb);
        });
  }


  template<typename OutputType, GARLIC_VIEW Layer>
  static inline OutputType
  resolve(const Layer& value, std::string_view key, OutputType&& default_value) {
    resolve_layer_cb(value, key, [&default_value](const auto& result) {
        default_value = decode<OutputType, Layer>(result);
        });
    return default_value;
  }


  template<typename OutputType, GARLIC_VIEW Layer, typename Callable>
  static inline void
  resolve_cb(const Layer& value, std::string_view key, Callable&& cb) {
    resolve_layer_cb(value, key, [&cb](const auto& result) {
        cb(decode<OutputType, Layer>(result, cb));
        });
  }


  template<GARLIC_VIEW Layer, typename Callable>
  static inline void
  get_member(const Layer& value, const char* key, Callable&& cb) noexcept {
    if(auto it = value.find_member(key); it != value.end_member()) cb((*it).value);
  }


  template<GARLIC_VIEW Layer, typename Callable>
  static inline void
  get_member(const Layer& value, std::string_view key, Callable&& cb) noexcept {
    if(auto it = value.find_member(key); it != value.end_member()) cb((*it).value);
  }


  template<GARLIC_VIEW Layer, std::integral IndexType, typename Callable>
  static inline std::enable_if_t<std::__is_random_access_iter<ConstValueIteratorOf<Layer>>::value>
  get_item(Layer layer, IndexType index, Callable&& cb) noexcept {
    if (auto it = layer.begin_list() += index; it < layer.end_list()) cb(*it);
  }


  template<GARLIC_VIEW Layer, std::integral IndexType, typename Callable>
  static inline std::enable_if_t<!std::__is_random_access_iter<ConstValueIteratorOf<Layer>>::value>
  get_item(Layer layer, IndexType index, Callable&& cb) noexcept {
    auto it = layer.begin_list();
    IndexType counter = 0;
    while (it != layer.end_list()) {
      if (counter == index) {
        cb(*it);
        return;
      }
      ++counter;
      ++it;
    }
  }


  template<typename Container, typename ValueType, typename Callable>
  static inline void
  find(const Container& container, const ValueType& value, Callable&& cb) {
    if (auto it = container.find(value); it != container.end()) cb(*it);
  }


  template<typename OutputType, GARLIC_VIEW Layer>
  static inline OutputType
  get(Layer layer, const char* key) {
    return decode<OutputType, Layer>((*layer.find_member(key)).value);
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType>
  static inline std::enable_if_t<std::__is_random_access_iter<ConstValueIteratorOf<Layer>>::value, OutputType>
  get(Layer layer, IndexType index) {
    return decode<OutputType, Layer>(layer.begin_list()[index]);
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType>
  static inline std::enable_if_t<!std::__is_random_access_iter<ConstValueIteratorOf<Layer>>::value, OutputType>
  get(Layer layer, IndexType index) {
    auto it = layer.begin_list();
    while (index > 0) {
      --index;
      ++it;
    }
    return decode<OutputType, Layer>(*it);
  }


  template<typename OutputType, GARLIC_VIEW Layer>
  static inline OutputType
  get(Layer layer, const char* key, OutputType default_value) {
    if (auto it = layer.find_member(key); it != layer.end_member()) {
      return decode<OutputType, Layer>((*it).value);
    }
    return default_value;
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType>
  static inline OutputType
  get(Layer layer, IndexType index, OutputType default_value) {
    get_item(layer, index, [&default_value](const auto& result) {
        default_value = decode<OutputType, Layer>(result);
        });
    return default_value;
  }


  template<typename OutputType, typename Callable, GARLIC_VIEW Layer>
  static inline void
  get_cb(Layer layer, const char* key, Callable&& cb) {
    if (auto it = layer.find_member(key); it != layer.end_member()) {
      cb(decode<OutputType, Layer>((*it).value));
    }
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType, typename Callable>
  static inline void
  get_cb(Layer layer, IndexType index, Callable&& cb) {
    get_item(layer, index, [&cb](const auto& result){
        cb(decode<OutputType, Layer>(result));
        });
  }


  template<typename OutputType, GARLIC_VIEW Layer>
  static inline OutputType
  safe_get(Layer layer, const char* key, OutputType&& default_value) {
    if (auto it = layer.find_member(key); it != layer.end_member()) {
      safe_decode<OutputType, Layer>(
          (*it).value,
          [&default_value](auto&& result) { default_value = result; });
    }
    return default_value;
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType>
  static inline OutputType
  safe_get(Layer layer, IndexType index, OutputType&& default_value) {
    get_item(layer, index, [&default_value](const auto& result) {
        safe_decode<OutputType, Layer>(result, [&default_value](auto&& value){
            default_value = value;
            });
        });
    return default_value;
  }


  template<typename OutputType, GARLIC_VIEW Layer, typename Callable>
  static inline void
  safe_get_cb(Layer layer, const char* key, Callable&& cb) {
    if (auto it = layer.find_member(key); it != layer.end_member()) {
      safe_decode<OutputType, Layer>((*it).value, cb);
    }
  }


  template<typename OutputType, GARLIC_VIEW Layer, std::integral IndexType, typename Callable>
  static inline void
  safe_get_cb(Layer layer, IndexType index, Callable&& cb) {
    get_item(layer, index, [&cb](const auto& result) {
        safe_decode<OutputType, Layer>(result, cb);
        });
  }


  template<GARLIC_VIEW Layer, GARLIC_REF Output>
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


  template<int BufferSize = 65536>
  class FileStreamBuffer : public std::streambuf {
  public:
    FileStreamBuffer(FILE* file) : file_(file) {}

    std::streambuf::int_type underflow() override {
      auto length = fread(read_buffer_, 1, sizeof(read_buffer_), file_);
      if (!length) return traits_type::eof();
      setg(read_buffer_, read_buffer_, read_buffer_ + length);
      return traits_type::to_int_type(*gptr());
    }

  private:
    FILE* file_;
    char read_buffer_[BufferSize];
  };

}

#endif /* end of include guard: GARLIC_UTILITY_H */
