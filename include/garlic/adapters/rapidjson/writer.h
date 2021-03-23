#ifndef GARLIC_RAPIDJSON_WRITER_H
#define GARLIC_RAPIDJSON_WRITER_H

#include "../../layer.h"

#include "rapidjson/writer.h"

namespace garlic::adapters::rapidjson {
    
  //! Use a rapidjson writer to dump a readable layer.
  //! \tparam Writer a rapidjson Writer
  //! \tparam Layer any readable layer conforming to garlic::ViewLayer
  //! \param writer the writer to use.
  //! \param layer the layer to dump.
  template<typename Writer, GARLIC_VIEW Layer>
  static void
  dump(Writer&& writer, Layer&& layer) {
    if (layer.is_object()) {
      writer.StartObject();
      for (const auto& pair : layer.get_object()) {
        writer.Key(pair.key.get_cstr());
        dump(writer, pair.value);
      }
      writer.EndObject();
    } else if (layer.is_list()) {
      writer.StartArray();
      for (const auto& item : layer.get_list()) {
        dump(writer, item);
      }
      writer.EndArray();
    } else if (layer.is_bool()) {
      writer.Bool(layer.get_bool());
    } else if (layer.is_int()) {
      writer.Int(layer.get_int());
    } else if (layer.is_double()) {
      writer.Double(layer.get_double());
    } else if (layer.is_string()) {
      writer.String(layer.get_cstr());
    } else {
      writer.Null();
    }
  }

}

#endif
