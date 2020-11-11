#ifndef GARLIC_TEST_UTILITY_H
#define GARLIC_TEST_UTILITY_H

#include <cstdio>
#include <string>
#include <deque>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml.h>
#include "gtest/gtest.h"


using NameQueue = std::deque<std::string>;


garlic::providers::libyaml::YamlDocument
get_libyaml_document(const char * name) {
  auto file = fopen(name, "r");
  auto doc = garlic::providers::libyaml::Yaml::load(file);
  fclose(file);
  return doc;
}

garlic::providers::rapidjson::JsonDocument
get_rapidjson_document(const char* name) {
  auto file = fopen(name, "r");
  auto doc = garlic::providers::rapidjson::Json::load(file);
  fclose(file);
  return doc;
}

garlic::providers::yamlcpp::YamlNode
get_yamlcpp_node(const char* name) {
  auto file = fopen(name, "r");
  auto node = garlic::providers::yamlcpp::Yaml::load(file);
  fclose(file);
  return node;
}

void print_constraint_result(const garlic::ConstraintResult& result, int level=0) {
  if (result.valid) {
    std::cout << "Passed all the checks." << std::endl;
  } else {
    std::string space = "";
    for (auto i = 0; i < level; i++) { space += "  "; }
    if (result.field) {
      std::cout << space << "Field: " << result.name << std::endl;
    } else {
      std::cout << space << "Constraint: " << result.name << std::endl;
    }
    std::cout << space << "Reason: " << result.reason << std::endl << std::endl;
    std::for_each(result.details.begin(), result.details.end(),
        [&level](const auto& item) { print_constraint_result(item, level + 1); }
        );
  }
}

template<garlic::ReadableLayer LayerType>
void print_constraints(const garlic::FieldPropertiesOf<LayerType>& props) {
  bool first = true;
  for (const auto& c : props.constraints) {
    if (first) std::cout << c->get_name();
    else std::cout << ", " << c->get_name();
    first = false;
  }
}

auto assert_field_constraint_result(const garlic::ConstraintResult& results, const char* name) {
  ASSERT_FALSE(results.valid);
  ASSERT_TRUE(results.field);
  ASSERT_STREQ(results.name.data(), name);
}

auto assert_constraint_result(const garlic::ConstraintResult& results, const char* name, const char* message) {
  ASSERT_FALSE(results.valid);
  ASSERT_STREQ(results.name.data(), name);
  ASSERT_STREQ(results.reason.data(), message);
}

template<garlic::ReadableLayer LayerType>
void assert_field_constraints(const garlic::Field<LayerType>& field, NameQueue names) {
  for(const auto& constraint : field.get_properties().constraints) {
    ASSERT_STREQ(constraint->get_name().data(), names.front().data());
    names.pop_front();
  }
}

template<typename LayerType>
void assert_model_fields(const garlic::Model<LayerType>& model, NameQueue names) {
  auto field_map = model.get_properties().field_map;
  for(const auto& name : names) {
    ASSERT_NE(field_map.find(name), field_map.end());
  }
}


template<typename LayerType>
void assert_model_fields(const garlic::Module<LayerType>& module, const char* model_name, NameQueue names) {
  auto model = module.get_model(model_name);
  ASSERT_NE(model, nullptr);
  assert_model_fields(*model, std::move(names));
}

template<typename LayerType>
void assert_model_field_constraints(
    const garlic::Model<LayerType>& model,
    const char* field_name,
    NameQueue constraints) {
  const auto& field_map = model.get_properties().field_map;
  auto it = field_map.find(field_name);
  ASSERT_NE(it, field_map.end());
  assert_field_constraints(*it->second, std::move(constraints));
}

template<typename LayerType>
void assert_model_field_constraints(
    const garlic::Module<LayerType>& module,
    const char* model_name,
    const char* field_name,
    NameQueue constraints) {
  auto model = module.get_model(model_name);
  ASSERT_NE(model, nullptr);
  assert_model_field_constraints(*model, field_name, std::move(constraints));
}

#endif /* end of include guard: GARLIC_TEST_UTILITY_H */
