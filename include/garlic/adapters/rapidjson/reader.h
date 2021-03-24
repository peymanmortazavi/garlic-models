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

    enum class node_state {
      key,
      push,
      set,
      value,
    };

  private:
    struct node {
      std::string_view key;
      reference_type layer;
      node_state state;

      node(reference_type layer,
           node_state state = node_state::set) : key(""), layer(layer), state(state) {}
    };
    std::deque<node> nodes_;

    inline reference_type layer() const { return nodes_.front().layer(); }
    inline node front() const { return nodes_.front(); }
    inline void pop() { nodes_.pop_front(); }
    
    template<typename Initializer>
    void add_member(Initializer&& initializer) {
      initializer(front().key, this->layer());
      this->front().state = node_state::key;
    }
    
  public:
    LayerHandler(Layer&& layer) {
      nodes_.emplace_front( node(layer.get_reference()) );
    }

    bool Null() {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_null();
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back();
          return true;
        }
        case node_state::value: {
          this->add_member([](auto key, auto ref) { ref.add_member(key); });
          return true;
        }
        default: return false;
      }
    }

    bool Bool(bool value) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_bool(value);
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member([&value](auto key, auto ref) { ref.add_member(key, value); });
          return true;
        }
        default: return false;
      }
      return true;
    }

    bool Int(int value) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_int(value);
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member([&value](auto key, auto ref) { ref.add_member(key, value); });
          return true;
        }
        default: return false;
      }
    }

    bool Double(double value) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_double(value);
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back(value);
          return true;
        }
        case node_state::value: {
          this->add_member([&value](auto key, auto ref) { ref.add_member(key, value); });
          return true;
        }
        default: return false;
      }
    }

    bool Uint(unsigned value) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_double(static_cast<double>(value));
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back(static_cast<double>(value));
          return true;
        }
        case node_state::value: {
          this->add_member([&value](auto key, auto ref) {
              ref.add_member(key, static_cast<double>(value));
              });
          return true;
        }
        default: return false;
      }
    }

    bool Int64(int64_t value) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_double(static_cast<double>(value));
          this->pop();
          return true;
        }
        case node_state::push: {
          this->layer().push_back(static_cast<double>(value));
          return true;
        }
        case node_state::value: {
          this->add_member([&value](auto key, auto ref) {
              ref.add_member(key, static_cast<double>(value));
              });
          return true;
        }
        default: return false;
      }
    }

    bool String(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_string(std::string_view(str, length));
          this->pop();
          break;
        }
        case node_state::push: {
          this->layer().push_back(std::string_view(str, length));
          break;
        }
        case node_state::value: {
          this->add_member([&str, &length](auto key, auto ref) {
              ref.add_member(key, std::string_view(key, length));
              });
          return true;
        }
        default: return false;
      }
      return true;
    }

    bool RawNumber(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      return String(str, length, copy);
    }

    bool StartObject() {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_object();
          this->front().state = node_state::key;
          return true;
        }
        case node_state::push: {
          auto& layer = this->layer();
          layer.push_back();
          this->nodes_.emplace_front( node(*--layer.end_list(), node_state::key) );
          this->layer().set_object();
          return true;
        }
        case node_state::value: {
          auto& layer = this->layer();
          layer.push_back();
          front().state = node_state::key;
          this->nodes_.emplace_front( node((*--layer.end_member()).value, node_state::key) );
          this->layer().set_object();
          return true;
        }
        default: return false;
      }
    }

    bool Key(const Ch* str, ::rapidjson::SizeType length, bool copy) {
      switch (front().state) {
        case node_state::key: {
          front().key = std::string_view(str, length);
          front().state = node_state::value;
          return true;
        }
        default: return false;
      }
    }

    bool EndObject(::rapidjson::SizeType length) {
      if (front().state == node_state::key) {
        this->pop();
        return true;
      }
      return false;
    }

    bool StartArray() {
      switch (front().state) {
        case node_state::set: {
          this->layer().set_list();
          this->front().state = node_state::push;
          return true;
        }
        case node_state::push: {
          auto& layer = this->layer();
          layer.push_back();
          this->nodes_.emplace_front( node(*--layer.end_list(), node_state::push) );
          this->layer().set_list();
          return true;
        }
        case node_state::value: {
          auto& layer = this->layer();
          layer.push_back();
          front().state = node_state::key;
          this->nodes_.emplace_front( node((*--layer.end_member()).value, node_state::push) );
          this->layer().set_list();
          return true;
        }
        default: return false;
      }
    }

    bool EndArray(::rapidjson::SizeType length) {
      if (front().state == node_state::push) {
        this->pop();
        return true;
      }
      return false;
    }
  };

}

#endif
