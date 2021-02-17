#ifndef GARLIC_LIBYAML_PARSER_H
#define GARLIC_LIBYAML_PARSER_H

#include "layer.h"
#include "yaml.h"

namespace garlic::providers::libyaml {

  enum yaml_parse_error : uint8_t {
    no_error = 0,
    parser_error = 1,
    parser_init_failure = 2,
    null_file = 3,
  };

  namespace internal {

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
    static inline void
    set_plain_scalar_value(LayerType&& layer, const char* data) {
      int i;
      if (parsing::ParseInt(data, i)) {
        layer.set_int(i);
        return;
      }
      double d;
      if (parsing::ParseDouble(data, d)) {
        layer.set_double(d);
        return;
      }
      bool b;
      if (parse_bool(data, b)) {
        layer.set_bool(b);
        return;
      }
      if (strcmp(data, "null") == 0) {
        layer.set_null();
        return;
      }
      layer.set_string(data);
    }


    class recursive_parser {
    public:
      ~recursive_parser() {
        yaml_event_delete(&event_);
        yaml_parser_delete(&parser_);
      }

      template<garlic::RefLayer LayerType>
      inline yaml_parse_error
      parse(FILE* file, LayerType&& layer) {
        if (!file)
          return yaml_parse_error::null_file;  // null file.

        if (!yaml_parser_initialize(&parser_))
          return yaml_parse_error::parser_init_failure;  // error state.

        yaml_parser_set_input_file(&parser_, file);
        return start(layer);
      }

      template<garlic::RefLayer LayerType>
      inline yaml_parse_error
      parse(const char* input, size_t length, LayerType&& layer) {
        if (!yaml_parser_initialize(&parser_))
          return yaml_parse_error::parser_init_failure;  // error state.

        yaml_parser_set_input_string(
            &parser_,
            reinterpret_cast<const unsigned char*>(input),
            length);

        return parse(layer);
      }

    private:
      template<garlic::RefLayer LayerType>
      inline yaml_parse_error parse(LayerType&& layer) {
        // parse the very first event.
        parse_event();

        // expect beginning events.
        if (!consume(yaml_event_type_t::YAML_STREAM_START_EVENT))
          return yaml_parse_error::parser_error;
        if (!consume(yaml_event_type_t::YAML_DOCUMENT_START_EVENT))
          return yaml_parse_error::parser_error;

        read_value_recursive(layer);

        // return errors if any.
        if (has_error_)
          return yaml_parse_error::parser_error;

        // expect final events.
        if (!consume(yaml_event_type_t::YAML_DOCUMENT_END_EVENT))
          return yaml_parse_error::parser_error;
        if (!consume(yaml_event_type_t::YAML_STREAM_END_EVENT))
          return yaml_parse_error::parser_error;

        return yaml_parse_error::no_error;
      }

      inline void parse_event() {
        if (!yaml_parser_parse(&parser_, &event_)) {
          has_error_ = true;
        }
      }

      inline void take() {
        yaml_event_delete(&event_);
        parse_event();
      }

      inline bool consume(yaml_event_type_t type) {
        if (event_.type == type) {
          take();
          return true;
        }
        return false;
      }

      inline const char*
      data() const {
        return reinterpret_cast<const char*>(event_.data.scalar.value);
      }

      template<garlic::RefLayer LayerType>
      void read_mapping_recursive(LayerType&& layer) {
        layer.set_object();
        do {
          if (event_.type != yaml_event_type_t::YAML_SCALAR_EVENT) {
            has_error_ = true;
          }
          strcpy(key_, data());  // store a copy of the key.
          take();
          layer.add_member_builder(key_, [this](auto ref) {
              read_value_recursive(ref);
              });
        } while (!consume(yaml_event_type_t::YAML_MAPPING_END_EVENT) && !has_error_);
      }

      template<garlic::RefLayer LayerType>
      void read_sequence_recursive(LayerType&& layer) {
        layer.set_list();
        do {
          layer.push_back_builder([this](auto ref) {
              read_value_recursive(ref);
              });
        } while (!consume(yaml_event_type_t::YAML_SEQUENCE_END_EVENT) && !has_error_);
      }

      template<garlic::RefLayer LayerType>
      void read_value_recursive(LayerType&& layer) {
        switch (event_.type) {
          case yaml_event_type_t::YAML_MAPPING_START_EVENT:
            take();
            read_mapping_recursive(layer);
            return;
          case yaml_event_type_t::YAML_SEQUENCE_START_EVENT:
            take();
            read_sequence_recursive(layer);
            return;
          case yaml_event_type_t::YAML_SCALAR_EVENT:
            {
              // if value is not plain, it is definitely a string.
              if (event_.data.scalar.style != yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE) {
                layer.set_string(this->data());
                take();
                return;
              }
              set_plain_scalar_value(layer, this->data());
              take();
            }
            return;
          case yaml_event_type_t::YAML_ALIAS_EVENT:
            has_error_ = true;
            return;
          default:
            return;
        }
      }

      yaml_parser_t parser_;
      yaml_event_t event_;
      bool has_error_ = false;
      char key_[128];
    };


    class iterative_parser {
    public:

      ~iterative_parser() {
        yaml_event_delete(&event_);
        yaml_parser_delete(&parser_);
      }
      
      template<garlic::RefLayer LayerType>
      yaml_parse_error
      parse(FILE* file, LayerType&& layer) {
        if (!file)
          return yaml_parse_error::null_file;

        if (!yaml_parser_initialize(&parser_))
          return yaml_parse_error::parser_init_failure;

        yaml_parser_set_input_file(&parser_, file);

        enum class state_type : uint8_t {
          set, push, read_key, read_value
        };

        struct node {
          using reference_type = decltype(layer.get_reference());

          reference_type value;
          state_type state = state_type::set;
        };

        std::deque<node> stack {
          node {layer.get_reference()}
        };

        bool delete_event = false;

        do {
          if (delete_event)
            yaml_event_delete(&event_);
          delete_event = true;
          if (!yaml_parser_parse(&parser_, &event_))
            return yaml_parse_error::parser_error;
          auto& item = stack.front();
          switch (item.state) {
            case state_type::push:
              {
                if (event_.type == yaml_event_type_t::YAML_SEQUENCE_END_EVENT) {
                  stack.pop_front();
                  continue;
                }
                item.value.push_back();
                stack.emplace_front(node { *--item.value.end_list() });
                // next section will update the layer.
              }
              break;
            case state_type::read_key:
              {
                if (event_.type == yaml_event_type_t::YAML_MAPPING_END_EVENT) {
                  stack.pop_front();
                  continue;
                }
                if (event_.type != yaml_event_type_t::YAML_SCALAR_EVENT)
                  return yaml_parse_error::parser_error;  // unexpected event.
                item.value.add_member(this->data());
                stack.emplace_front(node { (*--item.value.end_member()).value });
                continue;  // do not update the layer in this iteration.
              }
            case state_type::read_value:
              item.state = state_type::read_key;
            default:
              break;
          }
          // now update the layer.
          switch (event_.type) {
            case yaml_event_type_t::YAML_ALIAS_EVENT:
              return yaml_parse_error::parser_error;
            case yaml_event_type_t::YAML_SEQUENCE_START_EVENT:
              {
                stack.front().value.set_list();
                stack.front().state = state_type::push;
              }
              continue;
            case yaml_event_type_t::YAML_MAPPING_START_EVENT:
              {
                stack.front().value.set_object();
                stack.front().state = state_type::read_key;
              }
              continue;
            case yaml_event_type_t::YAML_SCALAR_EVENT:
              {
                if (event_.data.scalar.style != yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE) {
                  stack.front().value.set_string(this->data());
                } else {
                  set_plain_scalar_value(stack.front().value, this->data());
                }
                stack.pop_front();
              }
            default:
              continue;
          }
        } while(event_.type != yaml_event_type_t::YAML_STREAM_END_EVENT);

        return yaml_parse_error::no_error;
      }

    private:
      const char* data() const {
        return reinterpret_cast<const char*>(event_.data.scalar.value);
      }

      yaml_parser_t parser_;
      yaml_event_t event_;
    };

    class emitter {
    public:
      template<garlic::ViewLayer LayerType>
      yaml_parse_error emit(FILE* file, LayerType&& layer) {
        if (!file)
          return yaml_parse_error::null_file;

        if (!yaml_emitter_initialize(&emitter_))
          return yaml_parse_error::parser_init_failure;

        yaml_emitter_set_output_file(&emitter_, file);
        
        return this->process(layer);
      }

      template<garlic::ViewLayer LayerType>
      yaml_parse_error emit(char* output, size_t length, LayerType&& layer) {
        if (!yaml_emitter_initialize(&emitter_))
          return yaml_parse_error::parser_init_failure;

        size_t written;
        yaml_emitter_set_output_string(
            &emitter_,
            reinterpret_cast<unsigned char*>(output),
            length,
            &written);
        
        return this->process(layer);
      }

      ~emitter() {
        yaml_emitter_delete(&emitter_);
      }

    private:
      yaml_emitter_t emitter_;
      yaml_event_t event_;
      bool has_error_ = false;

      inline void emit() {
        if (!yaml_emitter_emit(&emitter_, &event_))
          has_error_ = true;
      }

      template<garlic::ViewLayer LayerType>
      yaml_parse_error process(LayerType&& layer)
      {
        yaml_stream_start_event_initialize(&event_, yaml_encoding_t::YAML_UTF8_ENCODING);
        this->emit();

        yaml_document_start_event_initialize(&event_, NULL, NULL, NULL, 0);
        this->emit();

        char buffer[128];
        this->emit(layer, buffer);

        yaml_document_end_event_initialize(&event_, 0);
        this->emit();

        yaml_stream_end_event_initialize(&event_);
        this->emit();

        if (has_error_)
          return yaml_parse_error::parser_error;

        return yaml_parse_error::no_error;
      }

      template<garlic::ViewLayer LayerType>
      inline void
      emit(LayerType&& layer, char* buffer) {
        if (layer.is_object()) {
          yaml_mapping_start_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_MAP_TAG,
              1, yaml_mapping_style_t::YAML_ANY_MAPPING_STYLE);
          emit();
          for (const auto& pair : layer.get_object()) {
            emit(pair.key, buffer);
            emit(pair.value, buffer);
          }
          yaml_mapping_end_event_initialize(&event_);
        } else if (layer.is_list()) {
          yaml_sequence_start_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_SEQ_TAG,
              1, yaml_sequence_style_t::YAML_ANY_SEQUENCE_STYLE);
          this->emit();
          for (const auto& item : layer.get_list()) {
            emit(item, buffer);
          }
          yaml_sequence_end_event_initialize(&event_);
        } else if (layer.is_bool()) {
          if (layer.get_bool())
            yaml_scalar_event_initialize(
                &event_, NULL,
                (yaml_char_t*)YAML_BOOL_TAG,
                (yaml_char_t*)"true", 4,
                1, 0,
                yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
          else
            yaml_scalar_event_initialize(
                &event_, NULL,
                (yaml_char_t*)YAML_INT_TAG,
                (yaml_char_t*)"false", 5,
                1, 0,
                yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        } else if (layer.is_int()) {
          auto size = sprintf(buffer, "%d", layer.get_int());
          yaml_scalar_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_INT_TAG,
              (yaml_char_t*)buffer, size,
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        } else if (layer.is_double()) {
          auto size = sprintf(buffer, "%f", layer.get_double());
          yaml_scalar_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_FLOAT_TAG,
              (yaml_char_t*)buffer, size,
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        } else if (layer.is_string()) {
          auto view = layer.get_string_view();
          yaml_scalar_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_STR_TAG,
              (yaml_char_t*)view.data(), view.size(),
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        } else {
          yaml_scalar_event_initialize(
              &event_, NULL,
              (yaml_char_t*)YAML_NULL_TAG,
              (yaml_char_t*)"null", 4,
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        }
        this->emit();
      }
    };

  }

  struct Yaml {

    static YamlDocument load(const char * data, size_t lenght) {
      YamlDocument doc;
      yaml_parser_t parser;
      yaml_parser_initialize(&parser);

      yaml_parser_set_input_string(
          &parser,
          reinterpret_cast<const unsigned char*>(data),
          lenght);

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

    template<typename LayerType>
    static inline yaml_parse_error
    parse(FILE* file, LayerType&& layer) {
      return internal::recursive_parser().parse(file, layer);
    }

    template<typename LayerType>
    static inline void
    parse(const char* data, LayerType&& layer) {
      return internal::recursive_parser().parse(data, strlen(data), layer);
    }

    template<typename LayerType>
    static inline void
    parse(const char* data, size_t length, LayerType&& layer) {
      return internal::recursive_parser().parse(data, length, layer);
    }

    template<typename LayerType>
    static inline yaml_parse_error
    emit(FILE* file, LayerType&& layer) {
      return internal::emitter().emit(file, layer);
    }
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_PARSER_H */
