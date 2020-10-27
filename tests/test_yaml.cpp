#include <algorithm>
#include <cstdio>
#include <gtest/gtest.h>
#include "yaml.h"
#include <garlic/garlic.h>
#include <garlic/providers/libyaml.h>

using namespace garlic;
using namespace garlic::providers::libyaml;

void print_document(YamlView value, int level) {
  std::string prefix = "";
  for(auto i = 0; i < level; i++) prefix += "....";
  if (value.is_bool()) {
    std::cout << prefix << "bool: " << (value.get_bool() ? "true" : "false") << std::endl;
  } else if (value.is_double()) {
    std::cout << prefix << "double: " << value.get_double() << std::endl;
  } else if (value.is_null()) {
    std::cout << prefix << "null" << std::endl;
  } else if (value.is_object()) {
    for(const auto& item : value.get_object()) {
      std::cout << prefix << " + Member" << std::endl;
      print_document(item.key, level + 1);
      print_document(item.value, level + 1);
    }
  } else if (value.is_list()) {
    for(const auto& item : value.get_list()) {
      print_document(item, level + 1);
    }
  } else if (value.is_string()) {
    std::cout << prefix << "string: " << value.get_string_view() << std::endl;
  }

}

TEST(YamlCpp, ProtocolTest) {
  yaml_parser_t parser;
  yaml_parser_initialize(&parser);

  auto file = fopen("data/test.yaml", "r");
  yaml_parser_set_input_file(&parser, file);

  yaml_document_t document;
  yaml_parser_load(&parser, &document);

  auto node = yaml_document_get_root_node(&document);

  YamlView doc {&document};
}
