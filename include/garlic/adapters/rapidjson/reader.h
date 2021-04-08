#ifndef GARLIC_RAPIDJSON_READER_H
#define GARLIC_RAPIDJSON_READER_H

#include <deque>

#include "../../layer.h"

#include "rapidjson/reader.h"

namespace garlic::adapters::rapidjson {

  //! RapidJSON read handler type that would populate a layer.
  //! \tparam Layer Any type conforming to garlic::RefLayer concept that is to be populated.
  template<GARLIC_REF Layer>
  class LayerHandler {
    using Ch = char;
    using reference_type = decltype(std::declval<Layer>().get_reference());

    enum class node_state {
      key,
      push,
      set,
      value,
    };

    struct node {
      text key;
      reference_type layer;
      node_state state;
      bool copy_key = true;

      node(reference_type layer,
           node_state state = node_state::set) : key(""), layer(layer), state(state), copy_key(true) {}
    };

  private:
    std::deque<node> nodes_;

    void add_member() {
      auto& item = nodes_.front();
      if (item.copy_key)
        item.layer.add_member(item.key);
      else
        item.layer.add_member(item.key.string_ref());
      item.state = node_state::key;
    }

    template<typename T>
    void add_member(T value) {
      auto& item = nodes_.front();
      if (item.copy_key)
        item.layer.add_member(item.key, value);
      else
        item.layer.add_member(item.key.string_ref(), value);
      item.state = node_state::key;
    }
    
  public:
    LayerHandler(Layer&& layer) {
      nodes_.emplace_front( node(layer.get_reference()) );
    }

    bool Null() {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_null();
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back();
          return true;
        }
        case node_state::value: {
          this->add_member();
          return true;
        }
        default: return false;
      }
    }

    bool Bool(bool value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_bool(value);
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member(value);
          return true;
        }
        default: return false;
      }
    }

    bool Int(int value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_int(value);
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member(value);
          return true;
        }
        default: return false;
      }
    }

    bool Double(double value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_double(value);
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member(value);
          return true;
        }
        default: return false;
      }
    }

    bool Uint(unsigned value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          if (value < INT32_MAX) {
            item.layer.set_int(static_cast<int>(value));
          } else {
            item.layer.set_double(static_cast<double>(value));
          }
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          if (value < INT32_MAX) {
            item.layer.push_back(static_cast<int>(value));
          } else {
            item.layer.push_back(static_cast<double>(value));
          }
          return true;
        }
        case node_state::value: {
          if (value < INT32_MAX) {
            this->add_member(static_cast<int>(value));
          } else {
            this->add_member(static_cast<double>(value));
          }
          return true;
        }
        default: return false;
      }
    }

    bool Uint64(uint64_t value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_double(static_cast<double>(value));
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back(static_cast<double>(value));
          return true;
        }
        case node_state::value: {
          this->add_member(static_cast<double>(value));
          return true;
        }
        default: return false;
      }
    }

    bool Int64(int64_t value) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_double(static_cast<double>(value));
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          item.layer.push_back(static_cast<double>(value));
          return true;
        }
        case node_state::value: {
          this->add_member(static_cast<double>(value));
          return true;
        }
        default: return false;
      }
    }

    bool String(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          if (copy)
            item.layer.set_string(text(str, length));
          else
            item.layer.set_string(string_ref(str, length));
          nodes_.pop_front();
          return true;
        }
        case node_state::push: {
          if (copy) {
            item.layer.push_back(text(str, length));
          } else {
            item.layer.push_back(string_ref(str, length));
          }
          return true;
        }
        case node_state::value: {
            if (copy)
              this->add_member(text(str, length));
            else
              this->add_member(string_ref(str, length));
          return true;
        }
        default: return false;
      }
    }

    bool RawNumber(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      return String(str, length, copy);
    }

    bool StartObject() {
      auto& item = nodes_.front();
      switch (nodes_.front().state) {
        case node_state::set: {
          item.layer.set_object();
          item.state = node_state::key;
          return true;
        }
        case node_state::push: {
          item.layer.push_back();
          this->nodes_.emplace_front( node(*--item.layer.end_list(), node_state::key) );
          nodes_.front().layer.set_object();
          return true;
        }
        case node_state::value: {
          this->add_member();
          nodes_.front().state = node_state::key;
          this->nodes_.emplace_front( node((*--item.layer.end_member()).value, node_state::key) );
          nodes_.front().layer.set_object();
          return true;
        }
        default: return false;
      }
    }

    bool Key(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::key: {
          item.key = text(str, length, (copy ? text_type::copy : text_type::reference));
          item.state = node_state::value;
          item.copy_key = copy;
          return true;
        }
        default: return false;
      }
    }

    bool EndObject(::rapidjson::SizeType length) {
      if (nodes_.front().state == node_state::key) {
        nodes_.pop_front();
        return true;
      }
      return false;
    }

    bool StartArray() {
      auto& item = nodes_.front();
      switch (item.state) {
        case node_state::set: {
          item.layer.set_list();
          item.state = node_state::push;
          return true;
        }
        case node_state::push: {
          item.layer.push_back();
          nodes_.emplace_front( node(*--item.layer.end_list(), node_state::push) );
          nodes_.front().layer.set_list();
          return true;
        }
        case node_state::value: {
          this->add_member();
          item.state = node_state::key;
          this->nodes_.emplace_front( node((*--item.layer.end_member()).value, node_state::push) );
          nodes_.front().layer.set_list();
          return true;
        }
        default: return false;
      }
    }

    bool EndArray(::rapidjson::SizeType length) {
      if (nodes_.front().state == node_state::push) {
        nodes_.pop_front();
        return true;
      }
      return false;
    }
  };

  //! Convenient shortcut method to create a layer handler to be used in a rapidjson reader.
  //! \tparam Layer any type conforming to garlic::RefLayer concept that is to be populated.
  template<GARLIC_REF Layer>
  static inline LayerHandler<Layer> make_handler(Layer&& layer) {
    return LayerHandler<Layer>(std::forward<Layer>(layer));
  }

}

#endif
