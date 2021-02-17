#ifndef GARLIC_LIBYAML_PARSER_H
#define GARLIC_LIBYAML_PARSER_H

#include "layer.h"
#include "yaml.h"

namespace garlic::providers::libyaml {

  enum libyaml_error : uint8_t {
    no_error = 0,
    parser_error = 1,
    parser_init_failure = 2,
    null_file = 3,
    alias_used = 4,
    emitter_error = 1,
    emitter_init_failure = 2,
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

      recursive_parser() {
        if (!yaml_parser_initialize(&parser_))
          error_ = libyaml_error::parser_init_failure;
      }

      inline void set_input(FILE* file) {
        if (!file)
          error_ = libyaml_error::null_file;
        yaml_parser_set_input_file(&parser_, file);
      }

      inline void set_input(const char* input, size_t length) {
        yaml_parser_set_input_string(
            &parser_,
            reinterpret_cast<const unsigned char*>(input),
            length);
      }

      inline void set_input(yaml_read_handler_t* handler, void* data) {
        yaml_parser_set_input(&parser_, handler, data);
      }

      template<garlic::RefLayer LayerType>
      inline libyaml_error parse(LayerType&& layer) {
        if (error_)
          return error_;

        // parse the very first event.
        parse_event();

        // expect beginning events.
        if (!consume(yaml_event_type_t::YAML_STREAM_START_EVENT))
          return libyaml_error::parser_error;
        if (!consume(yaml_event_type_t::YAML_DOCUMENT_START_EVENT))
          return libyaml_error::parser_error;

        read_value_recursive(layer);

        // return errors if any.
        if (error_)
          return error_;

        // expect final events.
        if (!consume(yaml_event_type_t::YAML_DOCUMENT_END_EVENT))
          return libyaml_error::parser_error;
        if (!consume(yaml_event_type_t::YAML_STREAM_END_EVENT))
          return libyaml_error::parser_error;

        return libyaml_error::no_error;
      }

    private:
      inline void parse_event() {
        if (!yaml_parser_parse(&parser_, &event_)) {
          error_ = libyaml_error::parser_error;
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
            error_ = libyaml_error::parser_error;
          }
          strcpy(key_, data());  // store a copy of the key.
          take();
          layer.add_member_builder(key_, [this](auto ref) {
              read_value_recursive(ref);
              });
        } while (!consume(yaml_event_type_t::YAML_MAPPING_END_EVENT) && !error_);
      }

      template<garlic::RefLayer LayerType>
      void read_sequence_recursive(LayerType&& layer) {
        layer.set_list();
        do {
          layer.push_back_builder([this](auto ref) {
              read_value_recursive(ref);
              });
        } while (!consume(yaml_event_type_t::YAML_SEQUENCE_END_EVENT) && !error_);
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
            error_ = libyaml_error::alias_used;
            return;
          default:
            return;
        }
      }

      yaml_parser_t parser_;
      yaml_event_t event_;
      libyaml_error error_ = libyaml_error::no_error;
      char key_[128];
    };


    class iterative_parser {
    public:

      iterative_parser() {
        if (!yaml_parser_initialize(&parser_))
          error_ = libyaml_error::parser_init_failure;
      }

      ~iterative_parser() {
        yaml_event_delete(&event_);
        yaml_parser_delete(&parser_);
      }

      inline void set_input(FILE* file) {
        if (!file)
          error_ = libyaml_error::null_file;
        yaml_parser_set_input_file(&parser_, file);
      }

      inline void set_input(const char* input, size_t length) {
        yaml_parser_set_input_string(
            &parser_,
            reinterpret_cast<const unsigned char*>(input),
            length);
      }

      inline void set_input(yaml_read_handler_t* handler, void* data) {
        yaml_parser_set_input(&parser_, handler, data);
      }
      
      template<garlic::RefLayer LayerType>
      libyaml_error parse(LayerType&& layer) {
        if (error_)
          return error_;

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
            return libyaml_error::parser_error;
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
                  return libyaml_error::parser_error;  // unexpected event.
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
              return libyaml_error::parser_error;
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

        return libyaml_error::no_error;
      }

    private:
      const char* data() const {
        return reinterpret_cast<const char*>(event_.data.scalar.value);
      }

      yaml_parser_t parser_;
      yaml_event_t event_;
      libyaml_error error_;
    };

    class emitter {
    public:
      emitter() {
        if (!yaml_emitter_initialize(&emitter_))
          error_ = libyaml_error::emitter_init_failure;
      }

      inline void set_output(FILE* file) {
        if (!file)
          error_ = libyaml_error::null_file;
        yaml_emitter_set_output_file(&emitter_, file);
      }
      
      inline void set_output(char* output, size_t size, size_t& count) {
        yaml_emitter_set_output_string(
            &emitter_,
            reinterpret_cast<unsigned char*>(output),
            size,
            &count);
      }

      inline void set_output(yaml_write_handler_t* handler, void* data) {
        yaml_emitter_set_output(&emitter_, handler, data);
      }

      ~emitter() {
        yaml_emitter_delete(&emitter_);
      }

      template<garlic::ViewLayer LayerType>
      libyaml_error emit(LayerType&& layer) {
        if (error_)
          return error_;

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

        if (error_)
          return error_;

        return libyaml_error::no_error;
      }


    private:
      yaml_emitter_t emitter_;
      yaml_event_t event_;
      libyaml_error error_ = libyaml_error::no_error;

      inline void emit() {
        if (!yaml_emitter_emit(&emitter_, &event_))
          error_ = libyaml_error::emitter_error;
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
    static inline libyaml_error
    parse(FILE* file, LayerType&& layer) {
      auto parser = internal::recursive_parser();
      parser.set_input(file);
      return parser.parse(layer);
    }

    template<typename LayerType>
    static inline void
    parse(const char* data, LayerType&& layer) {
      auto parser = internal::recursive_parser();
      parser.set_input(data, strlen(data));
      return parser.parse(layer);
    }

    template<typename LayerType>
    static inline void
    parse(const char* data, size_t length, LayerType&& layer) {
      auto parser = internal::recursive_parser();
      parser.set_input(data, length);
      return parser.parse(layer);
    }

    template<typename LayerType>
    static inline void
    parse(yaml_read_handler_t* handler, void* data, LayerType&& layer) {
      auto parser = internal::recursive_parser();
      parser.set_input(handler, data);
      return parser.parse(layer);
    }

    template<typename LayerType>
    static inline libyaml_error
    emit(FILE* file, LayerType&& layer) {
      auto emitter = internal::emitter();
      emitter.set_output(file);
      return emitter.emit(layer);
    }

    template<typename LayerType>
    static inline libyaml_error
    emit(char* output, size_t size, size_t& count, LayerType&& layer) {
      auto emitter = internal::emitter();
      emitter.set_output(output, size, count);
      return emitter.emit(layer);
    }

    template<typename LayerType>
    static inline libyaml_error
    emit(yaml_write_handler_t* handler, void* data, LayerType&& layer) {
      auto emitter = internal::emitter();
      emitter.set_output(handler, data);
      return emitter.emit(layer);
    }
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_PARSER_H */
