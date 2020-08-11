#ifndef GARLIC_LIBYAML_H
#define GARLIC_LIBYAML_H

#include "yaml.h"


namespace garlic {

  class YamlView {
  public:
    YamlView (const yaml_node_t& value) : value_(value) {}
    YamlView (const YamlView& another) : value_(another.value_) {}

    bool is_null() const { return value_.type & YAML_NO_NODE; }
    bool is_int() const noexcept { return true; }
    bool is_string() const noexcept { return true; }
    bool is_double() const noexcept { return true; }
    bool is_object() const noexcept { return true; }
    bool is_list() const noexcept { return true; }
    bool is_bool() const noexcept { return true; }


  private:
    const yaml_node_t& value_;
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_H */
