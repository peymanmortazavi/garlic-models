#ifndef GARLIC_UTILITY_H
#define GARLIC_UTILITY_H

#include "layer.h"
#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <streambuf>
#include <string_view>
#include <iostream>


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


  class lazy_string_splitter {
  public:
    using const_iterator = std::string_view::const_iterator;
    lazy_string_splitter(std::string_view text) : text_(text), cursor_(text.begin()) {}

    template<typename Callable>
    void for_each(const Callable& cb) {
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


  template<ReadableLayer LayerType, typename Callable>
  void resolve(const LayerType& value, std::string_view path, Callable cb) {
    lazy_string_splitter parts{path};
    std::unique_ptr<LayerType> cursor = std::make_unique<LayerType>(value);
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
        std::string_view x;
        // leave the list out for now.
        return;
      } else return;
    }
  }


  template<typename Callable>
  void get_member(const ReadableLayer auto& value, const char* key, const Callable& cb) noexcept {
    if(auto it = value.find_member(key); it != value.end_member()) cb((*it).value);
  }


  template<typename Callable>
  void get_member(const ReadableLayer auto& value, std::string_view key, const Callable& cb) noexcept {
    std::cout << "getting member : " << key << std::endl;
    if(auto it = value.find_member(key); it != value.end_member()) cb((*it).value);
  }


  template<typename Container, typename ValueType, typename Callable>
  void get(const Container& container, const ValueType& value, const Callable& cb) {
    if (auto it = container.find(value); it != container.end()) cb(*it);
  }


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
    char read_buffer_[65536];
  };

}

#endif /* end of include guard: GARLIC_UTILITY_H */
