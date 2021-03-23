#ifndef GARLIC_LIBYAML_EMITTER_H
#define GARLIC_LIBYAML_EMITTER_H

#include "../../layer.h"

#include "yaml.h"


namespace garlic::adapters::libyaml {

  class emitter {
  public:
    yaml_mapping_style_t mapping_style = yaml_mapping_style_t::YAML_FLOW_MAPPING_STYLE;
    yaml_sequence_style_t sequence_style = yaml_sequence_style_t::YAML_FLOW_SEQUENCE_STYLE;

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

    template<GARLIC_VIEW Layer>
    libyaml_error emit(Layer&& layer) {
      if (error_)
        return error_;

      yaml_stream_start_event_initialize(&event_, yaml_encoding_t::YAML_UTF8_ENCODING);
      this->emit();

      yaml_document_start_event_initialize(&event_, NULL, NULL, NULL, 1);
      this->emit();

      char buffer[128];
      this->emit(layer, buffer);

      yaml_document_end_event_initialize(&event_, 1);
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

    template<GARLIC_VIEW Layer>
    inline void
    emit(Layer&& layer, char* buffer) {
      if (layer.is_object()) {
        yaml_mapping_start_event_initialize(
            &event_, NULL,
            (yaml_char_t*)YAML_MAP_TAG,
            1, mapping_style);
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
            1, sequence_style);
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
        size = leading_zero_position(buffer, size);
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
            1, 1,
            yaml_scalar_style_t::YAML_DOUBLE_QUOTED_SCALAR_STYLE);
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

    template<typename Layer>
    static inline libyaml_error
    emit(FILE* file, Layer&& layer, bool pretty=false) {
      auto emitter = internal::emitter();
      emitter.set_output(file);
      if (pretty) {
        emitter.mapping_style = yaml_mapping_style_t::YAML_BLOCK_MAPPING_STYLE;
        emitter.sequence_style = yaml_sequence_style_t::YAML_BLOCK_SEQUENCE_STYLE;
      }
      return emitter.emit(layer);
    }

    template<typename Layer>
    static inline libyaml_error
    emit(char* output, size_t size, size_t& count, Layer&& layer, bool pretty = false) {
      auto emitter = internal::emitter();
      emitter.set_output(output, size, count);
      if (pretty) {
        emitter.mapping_style = yaml_mapping_style_t::YAML_BLOCK_MAPPING_STYLE;
        emitter.sequence_style = yaml_sequence_style_t::YAML_BLOCK_SEQUENCE_STYLE;
      }
      return emitter.emit(layer);
    }

    template<typename Layer>
    static inline libyaml_error
    emit(yaml_write_handler_t* handler, void* data, Layer&& layer, bool pretty = false) {
      auto emitter = internal::emitter();
      emitter.set_output(handler, data);
      if (pretty) {
        emitter.mapping_style = yaml_mapping_style_t::YAML_BLOCK_MAPPING_STYLE;
        emitter.sequence_style = yaml_sequence_style_t::YAML_BLOCK_SEQUENCE_STYLE;
      }
      return emitter.emit(layer);
    }

    template<GARLIC_VIEW Layer>
    inline int 
    emit_layer(
        yaml_emitter_t* emitter,
        yaml_event_t* event,
        Layer&& layer,
        char* buffer,
        yaml_mapping_style_t mapping_style = yaml_mapping_style_t::YAML_ANY_MAPPING_STYLE,
        yaml_sequence_style_t sequence_style = yaml_sequence_style_t::YAML_ANY_SEQUENCE_STYLE) {
      if (layer.is_object()) {
        yaml_mapping_start_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_MAP_TAG,
            1, mapping_style);
        if (!yaml_emitter_emit(emitter, event))
          return 1;
        for (const auto& pair : layer.get_object()) {
          if (!emit_layer(emitter, event, pair.key, buffer, mapping_style, sequence_style))
            return 1;
          if (!emit_layer(emitter, event, pair.value, buffer, mapping_style, sequence_style))
            return 1;
        }
        yaml_mapping_end_event_initialize(event);
      } else if (layer.is_list()) {
        yaml_sequence_start_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_SEQ_TAG,
            1, sequence_style);
        if (!yaml_emitter_emit(emitter, event))
          return 1;
        for (const auto& item : layer.get_list()) {
          emit(item, buffer);
        }
        yaml_sequence_end_event_initialize(event);
      } else if (layer.is_bool()) {
        if (layer.get_bool())
          yaml_scalar_event_initialize(
              event, NULL,
              (yaml_char_t*)YAML_BOOL_TAG,
              (yaml_char_t*)"true", 4,
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
        else
          yaml_scalar_event_initialize(
              event, NULL,
              (yaml_char_t*)YAML_INT_TAG,
              (yaml_char_t*)"false", 5,
              1, 0,
              yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
      } else if (layer.is_int()) {
        auto size = sprintf(buffer, "%d", layer.get_int());
        yaml_scalar_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_INT_TAG,
            (yaml_char_t*)buffer, size,
            1, 0,
            yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
      } else if (layer.is_double()) {
        auto size = sprintf(buffer, "%f", layer.get_double());
        size = leading_zero_position(buffer, size);
        yaml_scalar_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_FLOAT_TAG,
            (yaml_char_t*)buffer, size,
            1, 0,
            yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
      } else if (layer.is_string()) {
        auto view = layer.get_string_view();
        yaml_scalar_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_STR_TAG,
            (yaml_char_t*)view.data(), view.size(),
            1, 1,
            yaml_scalar_style_t::YAML_DOUBLE_QUOTED_SCALAR_STYLE);
      } else {
        yaml_scalar_event_initialize(
            event, NULL,
            (yaml_char_t*)YAML_NULL_TAG,
            (yaml_char_t*)"null", 4,
            1, 0,
            yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE);
      }
      if (!yaml_emitter_emit(emitter, event))
        return 1;
    }

}

#endif /* end of include guard: GARLIC_LIBYAML_EMITTER_H */


