#ifndef UTILITY_H
#define UTILITY_H

#include "layer.h"
#include <algorithm>
#include <cstring>
#include <streambuf>


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


  template<typename Callable>
  void get_member(const ReadableLayer auto& value, const char* key, const Callable& cb) noexcept {
    if(auto it = value.find_member(key); it != value.end_member()) cb((*it).value);
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

#endif /* end of include guard: UTILITY_H */
