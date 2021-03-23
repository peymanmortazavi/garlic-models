#ifndef GARLIC_LIBYAML_PARSER_H
#define GARLIC_LIBYAML_PARSER_H

#include "../../layer.h"

#include "yaml.h"

#include "error.h"
#include "internal.h"


namespace garlic::adapters::libyaml {

  class RecursiveParser {
  private:
    yaml_parser_t* parser_;
    yaml_event_t event_;
    bool error_ = false;

  public:
    RecursiveParser(yaml_parser_t* parser) : parser_(parser) {}

    ~RecursiveParser() {
      yaml_event_delete(&event_);
    }

    // Parse a layer with the inner yaml_parser_t and return whether it went ok or not.
    template<GARLIC_REF Layer>
    inline bool parse(Layer&& layer) {
      // parse the very first event.
      parse_event();

      // expect beginning events.
      if (!consume(yaml_event_type_t::YAML_STREAM_START_EVENT))
        return false;
      if (!consume(yaml_event_type_t::YAML_DOCUMENT_START_EVENT))
        return false;

      read_value_recursive(layer);

      // return errors if any.
      if (error_)
        return false;

      // expect final events.
      if (!consume(yaml_event_type_t::YAML_DOCUMENT_END_EVENT))
        return false;
      if (!consume(yaml_event_type_t::YAML_STREAM_END_EVENT))
        return false;

      return true;  // everything went ok!
    }

  private:
    // Parse the next event and set the error state.
    inline void parse_event() {
      if (!yaml_parser_parse(parser_, &event_))
        error_ = true;
    }

    // Delete existing event, parse the next event and set the error state.
    inline void take() {
      yaml_event_delete(&event_);
      parse_event();
    }

    // If the current event matches the expected type, return true and parse the next event.
    inline bool consume(yaml_event_type_t type) {
      if (event_.type == type) {
        take();
        return true;
      }
      return false;
    }

    // Get the current scalar value in const char*
    inline const char*
    data() const {
      return reinterpret_cast<const char*>(event_.data.scalar.value);
    }

    // Recursively read keys and values until a mapping end event gets consumed.
    template<GARLIC_REF Layer>
    void read_mapping_recursive(Layer&& layer) {
      layer.set_object();
      do {
        if (event_.type != yaml_event_type_t::YAML_SCALAR_EVENT) {
          error_ = true;
          return;
        }
        std::string tmp = data();  // Keep a copy of the key. TODO improve this.
        take();  // parse the next event, this will destroy the event so that tmp above is needed.
        layer.add_member_builder(tmp.c_str(), [this](auto ref) {
            read_value_recursive(ref);  // parse the value!
            });
      } while (!consume(yaml_event_type_t::YAML_MAPPING_END_EVENT) && !error_);
    }

    // Recursively read values until a sequence end event gets consumed.
    template<GARLIC_REF Layer>
    void read_sequence_recursive(Layer&& layer) {
      layer.set_list();
      do {
        layer.push_back_builder([this](auto ref) {
            read_value_recursive(ref);  // parse the value!
            });
      } while (!consume(yaml_event_type_t::YAML_SEQUENCE_END_EVENT) && !error_);
    }

    template<GARLIC_REF Layer>
    void read_value_recursive(Layer&& layer) {
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
              layer.set_string(
                  std::string_view(this->data(), event_.data.scalar.length));
              take();
              return;
            }
            internal::set_plain_scalar_value(layer, this->data());
            take();
          }
          return;
        case yaml_event_type_t::YAML_ALIAS_EVENT:  // Unsupported feature.
          error_ = true;
          return;
        default:
          return;
      }
    }
  };

  template<GARLIC_REF Layer, typename Initializer>
  static inline tl::expected<void, ParserProblem>
  load(Layer&& layer, Initializer&& initializer) {
    yaml_parser_t parser;

    if (!yaml_parser_initialize(&parser))
      return tl::make_unexpected(ParserProblem(parser));

    initializer(&parser);

    if (!RecursiveParser(&parser).parse(layer)) {
      auto problem = ParserProblem(parser);
      yaml_parser_delete(&parser);
      return tl::make_unexpected(problem);
    }

    return tl::expected<void, ParserProblem>();
  }

  //! Use libyaml parser to populate a writable layer using YAML string.
  //! \param data YAML string.
  //! \param length length of the string to load.
  //! \param layer the writable layer to populate.
  template<GARLIC_REF Layer>
  static inline tl::expected<void, ParserProblem>
  load(const char* data, size_t length, Layer&& layer) {
    return load(layer, [&](yaml_parser_t* parser) {
        yaml_parser_set_input_string(parser, reinterpret_cast<const unsigned char*>(data), length);
        });
  }

  //! Use libyaml parser to populate a writable layer using YAML string.
  //! \param data YAML string.
  //! \param layer the writable layer to populate.
  template<GARLIC_REF Layer>
  static inline tl::expected<void, ParserProblem>
  load(const char* data, Layer&& layer) {
    return load(data, strlen(data), layer);
  }

  //! Use libyaml parser to populate a writable layer using a custom read handler.
  //! \param handler custom read handler.
  //! \param data pointer to custom context that gets fed to the read handler.
  //! \param layer the writable layer to populate.
  template<GARLIC_REF Layer>
  static inline tl::expected<void, ParserProblem>
  load(yaml_read_handler_t* handler, void* data, Layer&& layer) {
    return load(layer, [&](yaml_parser_t* parser) {
        yaml_parser_set_input(parser, handler, data);
        });
  }

  //! Use libyaml parser to populate a writable layer using a file.
  //! \param file an open and readable file. This function does not close the file afterward.
  //! \param layer the writable layer to populate.
  template<GARLIC_REF Layer>
  static inline tl::expected<void, ParserProblem>
  load(FILE* file, Layer&& layer) {
    return load(layer, [&](yaml_parser_t* parser) {
        yaml_parser_set_input_file(parser, file);
        });
  }

  //! Use an already initialized and ready yaml_parser_t to populate a writable layer.
  //! \param parser an initialized and ready yaml_parser_t
  //! \param layer the writable layer to populate.
  //! \return true if successful, false if loading runs into an error.
  template<GARLIC_REF Layer>
  static inline bool
  load(yaml_parser_t* parser, Layer&& layer) {
    return RecursiveParser(parser).parse(layer);
  }

}

#endif /* end of include guard: GARLIC_LIBYAML_PARSER_H */
