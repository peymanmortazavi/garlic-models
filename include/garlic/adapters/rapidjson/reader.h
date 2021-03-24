#ifndef GARLIC_RAPIDJSON_WRITER_H
#define GARLIC_RAPIDJSON_WRITER_H

#include <deque>

#include "../../layer.h"

#include "rapidjson/reader.h"

namespace garlic::adapters::rapidjson {

  template<GARLIC_REF Layer>
  class LayerHandler {
    using Ch = char;
    using reference_type = decltype(std::declval<Layer>().get_reference());

  private:
    std::deque<reference_type> layers_;

    inline reference_type layer() const { return layers_.front(); }
    
  public:
    LayerHandler(Layer&& layer) {
      layers_.emplace_front(layer->get_reference());
    }

    bool Null() {
      this->layer().set_null();
      return true;
    }

    bool Bool(bool value) {
      this->layer().set_bool(value);
      return true;
    }

    bool Int(int value) {
      this->layer().set_int(value);
      return true;
    }

    bool Double(double value) {
      this->layer().set_double(value);
      return true;
    }

    bool Uint(unsigned value) {
      return Double(static_cast<double>(value));
    }

    bool Int64(int64_t value) {
      return Double(static_cast<double>(value));
    }

    bool String(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      this->layer().set_string(std::string_view(str, length));
      return true;
    }

    bool RawNumber(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      return String(str, length, copy);
    }

    bool StartObject() {
      this->layer().set_object();
      return true;
    }

    bool Key(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      this->layer().add_member(std::string_view(str, length));
      this->layers_.emplace_front((*--this->layer().end_member()).value);
    }

    bool EndObject(::rapidjson::SizeType length) {
      this->layers_.pop_front();
      return true;
    }

    bool StartArray() {
      this->layer().set_list();
      this->layers_.emplace_front((*--this->layer().end_list()));
      return true;
    }

    bool EndArray(::rapidjson::SizeType length) {
      this->layers_.pop_front();
      return true;
    }
  };
    
  template<typename Reader, GARLIC_REF Layer>
  static void load(Reader&& reader, Layer&& layer) {
    
  }

}

#endif
