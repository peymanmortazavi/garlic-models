#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/type.h"
#include <yaml-cpp/yaml.h>
#include <gtest/gtest.h>
#include <garlic/providers/yaml-cpp.h>
#include <iostream>



TEST(YamlCpp, BasicTest) {
  auto document = YAML::LoadFile("data/test.yaml");

  garlic::YamlView view {document};
  for(const auto& item : view.get_object()) {
    std::cout << item.key.get_string() << ": " << item.value.get_string_view() << std::endl;
  }
}
