#ifndef GARLIC_LIBYAML_H
#define GARLIC_LIBYAML_H

#include "yaml.h"
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <deque>
#include "../parsing/numbers.h"
#include "../layer.h"


namespace garlic::providers::libyaml {

  class YamlView {

    struct ValueIteratorWrapper {
      using iterator_type = yaml_node_item_t*;
      using output_type = YamlView;

      iterator_type iterator;
      yaml_document_t* doc;

      inline YamlView wrap() const {
        return YamlView(doc, yaml_document_get_node(doc, *iterator));
      }
    };

    struct MemberIteratorWrapper {
      using iterator_type = yaml_node_pair_t*;
      using output_type = MemberPair<YamlView>;

      iterator_type iterator;
      yaml_document_t* doc;

      inline output_type wrap() const {
        return output_type {
          YamlView{doc, yaml_document_get_node(doc, iterator->key)},
          YamlView{doc, yaml_document_get_node(doc, iterator->value)},
        };
      }
    };

  public:
    using ConstValueIterator = RandomAccessIterator<ValueIteratorWrapper>;
    using ConstMemberIterator = RandomAccessIterator<MemberIteratorWrapper>;

    YamlView (yaml_document_t* doc, yaml_node_t* node) : doc_(doc), node_(node) {}
    YamlView (yaml_document_t* doc) : doc_(doc) { node_ = yaml_document_get_root_node(doc); }

    bool is_null() const {
      return node_->type == yaml_node_type_t::YAML_NO_NODE || (
          is_string() && strcmp("null", get_cstr()) == 0
          );
    }
    bool is_int() const noexcept {
      int holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseInt(scalar_data(), holder)
      );
    }
    bool is_string() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_double() const noexcept {
      double holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseDouble(scalar_data(), holder)
      );
    }
    bool is_object() const noexcept { return node_->type == yaml_node_type_t::YAML_MAPPING_NODE; }
    bool is_list() const noexcept { return node_->type == yaml_node_type_t::YAML_SEQUENCE_NODE; }
    bool is_bool() const noexcept {
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        check_bool(scalar_data(), [](auto){})
      );
    }

    char* scalar_data() const noexcept { return (char*)node_->data.scalar.value; }
    int get_int() const noexcept {
      int result = 0;
      parsing::ParseInt(scalar_data(), result);
      return result;
    }
    std::string get_string() const noexcept { return std::string{scalar_data()}; }
    std::string_view get_string_view() const noexcept { return std::string_view{scalar_data()}; }
    const char* get_cstr() const noexcept { return scalar_data(); }
    double get_double() const noexcept {
      double result;
      parsing::ParseDouble(scalar_data(), result);
      return result;
    }
    bool get_bool() const noexcept {
      bool result = false;
      check_bool(scalar_data(), [&result](auto r) { result = r; });
      return result;
    }

    ConstValueIterator begin_list() const { return ConstValueIterator({node_->data.sequence.items.start, doc_}); }
    ConstValueIterator end_list() const { return ConstValueIterator({node_->data.sequence.items.top, doc_}); }
    auto get_list() const { return ConstListRange<YamlView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator({node_->data.mapping.pairs.start, doc_}); }
    ConstMemberIterator end_member() const { return ConstMemberIterator({node_->data.mapping.pairs.top, doc_}); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return strcmp(key, item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(std::string_view key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return key.compare(item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(const YamlView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlView>{*this}; }

    const yaml_node_t& get_inner_value() const { return *node_; }
    const yaml_document_t& get_inner_document() const { return *doc_; }
    YamlView get_view() const { return YamlView(*this); }

  private:
    yaml_document_t* doc_;
    yaml_node_t* node_;

    template<typename Callable>
    bool check_bool(const char* input, const Callable& cb) const {
      struct pair {
        const char* name;
        bool value;
      };
      static const pair pairs[8] = {
        {"y", true}, {"n", false},
        {"true", true}, {"false", false},
        {"on", true}, {"off", false},
        {"yes", true}, {"no", false},
      };
      for(auto it = std::begin(pairs); it != std::end(pairs); it++) {
        if (strcmp(it->name, input) == 0) {
          cb(it->value);
          return true;
        }
      }
      return false;
    }
  };

  class YamlDocument {
  public:
    ~YamlDocument() { yaml_document_delete(&doc_); }

    YamlView get_view() {
      return YamlView{&doc_};
    }
    yaml_document_t* get_inner_document() { return &doc_; }

  protected:
    yaml_document_t doc_;
  };

  struct Yaml {
    static YamlDocument load(const char * data, size_t lenght) {
      YamlDocument doc;
      yaml_parser_t parser;
      yaml_parser_initialize(&parser);
      yaml_parser_set_input_string(&parser, reinterpret_cast<const unsigned char*>(data), lenght);
      yaml_parser_load(&parser, doc.get_inner_document());
      yaml_parser_delete(&parser);
      return doc;
    }

    static YamlDocument load(const char * data) {
      return Yaml::load(data, strlen((char*)data));
    }

    static YamlDocument load(FILE * file) {
      YamlDocument doc;
      yaml_parser_t parser;
      yaml_parser_initialize(&parser);
      yaml_parser_set_input_file(&parser, file);
      yaml_parser_load(&parser, doc.get_inner_document());
      yaml_parser_delete(&parser);
      return doc;
    }

    enum yaml_parse_error : uint8_t {
      no_error = 0,
      parser_error = 1,
      parser_init_failure = 2,
      null_file = 3,
    };

    template<typename LayerType>
    static inline yaml_parse_error
    parse(FILE* file, LayerType&& layer) {
      parse_context context;

      if (!file)
        return yaml_parse_error::null_file;  // null file.

      if (!yaml_parser_initialize(&context.parser))
        return yaml_parse_error::parser_init_failure;  // error state.

      yaml_parser_set_input_file(&context.parser, file);

      context.parse();  // take the first event.

      // expect beginning events.
      if (!context.consume(yaml_event_type_t::YAML_STREAM_START_EVENT))
        return yaml_parse_error::parser_error;
      if (!context.consume(yaml_event_type_t::YAML_DOCUMENT_START_EVENT))
        return yaml_parse_error::parser_error;

      read_value(layer, context);

      // return errors if any.
      if (context.has_error)
        return yaml_parse_error::parser_error;

      // expect final events.
      if (!context.consume(yaml_event_type_t::YAML_DOCUMENT_END_EVENT))
        return yaml_parse_error::parser_error;
      if (!context.consume(yaml_event_type_t::YAML_STREAM_END_EVENT))
        return yaml_parse_error::parser_error;

      return yaml_parse_error::no_error;
    }

    template<typename LayerType>
    static inline void
    parse(const char* data, LayerType&& layer) {
    }

    static inline void
    dump(FILE* file, const YamlDocument& doc) {
    }

    struct parse_context {
      yaml_parser_t parser;
      yaml_event_t event;
      bool has_error = false;
      char key[128];

      ~parse_context() {
        yaml_event_delete(&event);
        yaml_parser_delete(&parser);
      }

      inline void parse() {
        if (!yaml_parser_parse(&parser, &event)) {
          has_error = true;
        }
      }

      inline void take() {
        yaml_event_delete(&event);
        parse();
      }

      inline bool consume(yaml_event_type_t type) {
        if (event.type == type) {
          take();
          return true;
        }
        return false;
      }

      inline const char*
      data() const {
        return reinterpret_cast<const char*>(event.data.scalar.value);
      }
    };

    static bool parse_bool(const char* data, bool& output) {
      struct pair {
        const char* name;
        bool value;
      };
      static const pair pairs[8] = {
        {"y", true}, {"n", false},
        {"true", true}, {"false", false},
        {"on", true}, {"off", false},
        {"yes", true}, {"no", false},
      };
      for (const auto& it : pairs) {
        if (strcmp(it.name, data) == 0) {
          output = it.value;
          return true;
        }
      }
      return false;
    }

    template<garlic::RefLayer LayerType>
    static void
    read_mapping(LayerType&& layer, parse_context& context) {
      layer.set_object();
      do {
        if (context.event.type != yaml_event_type_t::YAML_SCALAR_EVENT) {
          return;  // error state.
        }
        strcpy(context.key, context.data());  // store a copy of the key.
        context.take();
        layer.add_member_builder(context.key, [&context](auto ref) {
            read_value(ref, context);
            });
      } while (!context.consume(yaml_event_type_t::YAML_MAPPING_END_EVENT) && !context.has_error);
    }

    template<garlic::RefLayer LayerType>
    static void
    read_sequence(LayerType&& layer, parse_context& context) {
      layer.set_list();
      do {
        layer.push_back_builder([&context](auto ref) {
            read_value(ref, context);
            });
      } while (!context.consume(yaml_event_type_t::YAML_SEQUENCE_END_EVENT) && !context.has_error);
    }

    template<garlic::RefLayer LayerType>
    static void read_value(LayerType&& layer, parse_context& context) {
      switch (context.event.type) {
        case yaml_event_type_t::YAML_MAPPING_START_EVENT:
          context.take();
          read_mapping(layer, context);
          break;
        case yaml_event_type_t::YAML_SEQUENCE_START_EVENT:
          context.take();
          read_sequence(layer, context);
          break;
        case yaml_event_type_t::YAML_SCALAR_EVENT:
          {
            if (context.event.data.scalar.style != yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE) {
              layer.set_string(context.data());
              context.take();
              return;
            }
            int i;
            const char* data = context.data();
            if (parsing::ParseInt(data, i)) {
              layer.set_int(i);
              context.take();
              return;
            }
            double d;
            if (parsing::ParseDouble(data, d)) {
              layer.set_double(d);
              context.take();
              return;
            }
            bool b;
            if (parse_bool(data, b)) {
              layer.set_bool(b);
              context.take();
              return;
            }
            if (strcmp(data, "null") == 0) {
              layer.set_null();
              context.take();
              return;
            }
            layer.set_string(data);
            context.take();
          }
          break;
        case yaml_event_type_t::YAML_ALIAS_EVENT:
          context.has_error = true;
          return;  // error state for unsupported aliasing.
        default:
          return;
      }
    }

    template<garlic::RefLayer LayerType>
    static inline void
    emit(FILE* file, LayerType&& layer) {
    }
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_H */
