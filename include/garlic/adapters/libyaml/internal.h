#ifndef GARLIC_LIBYAML_INTERNAL_H
#define GARLIC_LIBYAML_INTERNAL_H

#include "../../parsing/numbers.h"
#include "../../layer.h"

#include "yaml.h"

namespace garlic::adapters::libyaml::internal {

  // Get the leading zero position in a string of format *.*0*
  static inline int leading_zero_position(char* buffer, int size) {
    while (buffer[--size] == '0');
    ++size;
    if (buffer[size] == '.') ++size;
    return size;
  }

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

  // Write to the layer with the scalar value of the data.
  template<GARLIC_REF Layer>
  static inline void
  set_plain_scalar_value(Layer&& layer, const char* data) {
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

  static inline bool emit_mapping_start(yaml_emitter_t* emitter, yaml_event_t* event, yaml_mapping_style_t style) {
    return (
      yaml_mapping_start_event_initialize(event, NULL, (yaml_char_t*)YAML_MAP_TAG, 1, style) &&
      yaml_emitter_emit(emitter, event)
    );
  }

  static inline bool emit_sequence_start(yaml_emitter_t* emitter, yaml_event_t* event, yaml_sequence_style_t style) {
    return (
      yaml_sequence_start_event_initialize(event, NULL, (yaml_char_t*)YAML_SEQ_TAG, 1, style) &&
      yaml_emitter_emit(emitter, event)
    );
  }

  static inline bool initialize_true(yaml_event_t* event) {
    return yaml_scalar_event_initialize(
      event, NULL,
      (yaml_char_t*)YAML_BOOL_TAG, (yaml_char_t*)"true", 4,
      1, 0, yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
  }

  static inline bool initialize_false(yaml_event_t* event) {
    return yaml_scalar_event_initialize(
      event, NULL,
      (yaml_char_t*)YAML_BOOL_TAG, (yaml_char_t*)"false", 5,
      1, 0, yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
  }

  static inline bool initialize_int(yaml_event_t* event, int value, char* buffer) {
    auto size = sprintf(buffer, "%d", value);
    return yaml_scalar_event_initialize(
        event, NULL,
        (yaml_char_t*)YAML_INT_TAG,
        (yaml_char_t*)buffer, size,
        1, 0,
        yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
  }

  static inline bool initialize_double(yaml_event_t* event, double value, char* buffer) {
    auto size = sprintf(buffer, "%f", value);
    size = leading_zero_position(buffer, size);
    return yaml_scalar_event_initialize(
        event, NULL,
        (yaml_char_t*)YAML_FLOAT_TAG,
        (yaml_char_t*)buffer, size,
        1, 0,
        yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
  }

  static inline bool
  initialize_string(yaml_event_t *event, const char *value, size_t length) {
    return yaml_scalar_event_initialize(
        event, NULL,
        (yaml_char_t*)YAML_STR_TAG,
        (yaml_char_t*)value, length,
        1, 1,
        yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
  }

  static inline bool initialize_null(yaml_event_t* event) {
    return yaml_scalar_event_initialize(
        event, NULL,
        (yaml_char_t*)YAML_NULL_TAG,
        (yaml_char_t*)"null", 4,
        1, 0,
        yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);

  }

  static inline bool emit_start_stream_and_document(yaml_emitter_t* emitter, yaml_event_t* event) {
    return (
      yaml_stream_start_event_initialize(event, yaml_encoding_t::YAML_UTF8_ENCODING) &&
      yaml_emitter_emit(emitter, event) &&
      yaml_document_start_event_initialize(event, NULL, NULL, NULL, 1) &&
      yaml_emitter_emit(emitter, event)
    );
  }

  static inline bool emit_end_stream_and_document(yaml_emitter_t* emitter, yaml_event_t* event) {
    return (
      yaml_document_end_event_initialize(event, 1) &&
      yaml_emitter_emit(emitter, event) &&
      yaml_stream_end_event_initialize(event) &&
      yaml_emitter_emit(emitter, event)
    );
  }

}

#endif /* end of include guard: GARLIC_LIBYAML_INTERNAL_H */
