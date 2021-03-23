#ifndef GARLIC_LIBYAML_EMITTER_H
#define GARLIC_LIBYAML_EMITTER_H

#include "../../layer.h"

#include "yaml.h"

#include "error.h"
#include "internal.h"

namespace garlic::adapters::libyaml {

  /*! \brief Use an initialized and ready emitter to emit a layer.
   *! \note This method does not create stream and document start and end events.
   *! \param emitter an initialized and ready emitter.
   *! \param event an event to use for emitting. does not need to be initialized.
   *! \param layer the layer to emit.
   *! \param buffer only used for integer and double values.
   *!               can be NULL if layer does not have numbers.
   *! \param mapping_style yaml mapping style to use when creating mapping events.
   *! \param sequence_style yaml sequence style to use when creating sequence events.
   */
  template<GARLIC_VIEW Layer>
  inline bool 
  emit(yaml_emitter_t* emitter,
       yaml_event_t* event,
       Layer&& layer,
       char* buffer,
       yaml_mapping_style_t mapping_style = yaml_mapping_style_t::YAML_ANY_MAPPING_STYLE,
       yaml_sequence_style_t sequence_style = yaml_sequence_style_t::YAML_ANY_SEQUENCE_STYLE) {
    if (layer.is_object()) {
      if (!internal::emit_mapping_start(emitter, event, mapping_style))
        return false;
      for (const auto& pair : layer.get_object()) {
        if (!emit(emitter, event, pair.key, buffer, mapping_style, sequence_style))
          return false;
        if (!emit(emitter, event, pair.value, buffer, mapping_style, sequence_style))
          return false;
      }
      yaml_mapping_end_event_initialize(event);
    } else if (layer.is_list()) {
      if (!internal::emit_sequence_start(emitter, event, sequence_style))
        return false;
      for (const auto& item : layer.get_list()) {
        emit(emitter, event, item, buffer, mapping_style, sequence_style);
      }
      yaml_sequence_end_event_initialize(event);
    } else if (layer.is_bool()) {
      if (! (layer.get_bool() ? internal::initialize_true(event) : internal::initialize_false(event)) )
        return false;
    } else if (layer.is_int()) {
      if (!internal::initialize_int(event, layer.get_int(), buffer))
        return false;
    } else if (layer.is_double()) {
      if (!internal::initialize_double(event, layer.get_double(), buffer))
        return false;
    } else if (layer.is_string()) {
      auto view = layer.get_string_view();
      if (!internal::initialize_string(event, view.data(), view.size()))
        return false;
    } else {
      if (!internal::initialize_null(event))
        return false;
    }
    if (!yaml_emitter_emit(emitter, event))
      return false;
    
    return true;
  }

  template<GARLIC_VIEW Layer, typename Initializer>
  static inline tl::expected<void, EmitterProblem>
  emit(Layer&& layer, bool pretty, Initializer&& initializer) {
    yaml_emitter_t emitter;
    yaml_event_t event;

    if (!yaml_emitter_initialize(&emitter))
      return tl::make_unexpected(EmitterProblem{emitter.problem});

    if (!internal::emit_start_stream_and_document(&emitter, &event)) {
      auto problem = EmitterProblem { emitter.problem };
      yaml_emitter_delete(&emitter);
      return tl::make_unexpected(problem);
    }

    initializer(&emitter);

    auto mapping_style = yaml_mapping_style_t::YAML_ANY_MAPPING_STYLE;
    auto sequence_style = yaml_sequence_style_t::YAML_ANY_SEQUENCE_STYLE;
    if (pretty) {
      mapping_style = yaml_mapping_style_t::YAML_BLOCK_MAPPING_STYLE;
      sequence_style = yaml_sequence_style_t::YAML_BLOCK_SEQUENCE_STYLE;
    }

    char buffer[325];  // enough to serializer smallest and largest double values.
    if (emit(&emitter, &event, layer, buffer, mapping_style, sequence_style) &&
        internal::emit_end_stream_and_document(&emitter, &event)) {
      yaml_emitter_delete(&emitter);
      return tl::expected<void, EmitterProblem>();
    }

    auto problem = EmitterProblem { emitter.problem };
    yaml_emitter_delete(&emitter);
    return tl::make_unexpected(problem);
  }

  //! Use libyaml emitter to dump layer content in YAML format in a file.
  //! \param file an open and writable file.
  //! \param layer layer to dump.
  //! \param pretty if true, more human readable format will be used.
  template<GARLIC_VIEW Layer>
  static inline tl::expected<void, EmitterProblem>
  emit(FILE* file, Layer&& layer, bool pretty = false) {
    return emit(layer, pretty, [&](yaml_emitter_t* emitter) {
        yaml_emitter_set_output_file(emitter, file);
        });
  }

  //! Use libyaml emitter to dump layer content in YAML format in a string buffer.
  //! \param output output string buffer.
  //! \param size length of the string buffer.
  //! \param pretty if true, more human readable format will be used.
  //! \return written size if successful, EmitterProblem otherwise.
  template<GARLIC_VIEW Layer>
  static inline tl::expected<size_t, EmitterProblem>
  emit(char* output, size_t size, Layer&& layer, bool pretty = false) {
    size_t written = 0;

    auto result = emit(layer, pretty, [&](yaml_emitter_t* emitter) {
        yaml_emitter_set_output_string(emitter, reinterpret_cast<unsigned char*>(output), size, &written);
        });

    if (result)
      return written;
    else
      return result.error();
  }

  //! Use libyaml emitter to dump layer content in YAML format using a custom handler.
  //! \param handler a libyaml write handler method.
  //! \param data pointer to custom context data to use for the custom write handler.
  //! \param pretty if true, more human readable format will be used.
  template<GARLIC_VIEW Layer>
  static inline tl::expected<void, EmitterProblem>
  emit(yaml_write_handler_t* handler, void* data, Layer&& layer, bool pretty = false) {
    return emit(layer, pretty, [&](yaml_emitter_t* emitter) {
        yaml_emitter_set_output(emitter, handler, data);
        });
  }

}

#endif /* end of include guard: GARLIC_LIBYAML_EMITTER_H */


