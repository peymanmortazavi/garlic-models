#ifndef GARLIC_TEST_UTILITY_H
#define GARLIC_TEST_UTILITY_H

#include <cstdio>
#include <string>
#include <deque>
#include <garlic/garlic.h>
#include <garlic/constraints.h>
#include <garlic/models.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml.h>
#include "gtest/gtest.h"


using NameQueue = std::deque<std::string>;

garlic::providers::libyaml::YamlDocument
get_libyaml_document(const char * name);

garlic::providers::rapidjson::JsonDocument
get_rapidjson_document(const char* name);

garlic::providers::yamlcpp::YamlNode
get_yamlcpp_node(const char* name);

void print_constraint_result(const garlic::ConstraintResult& result, int level=0);
void assert_field_constraint_result(const garlic::ConstraintResult& results, const char* name);
void assert_constraint_result(const garlic::ConstraintResult& results, const char* name, const char* message);

template<garlic::ReadableLayer LayerType>
void print_constraints(const garlic::FieldPropertiesOf<LayerType>& props) {
  bool first = true;
  for (const auto& c : props.constraints) {
    if (first) std::cout << c->get_name();
    else std::cout << ", " << c->get_name();
    first = false;
  }
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
  assert_field_constraints(*it->second.field, std::move(constraints));
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

template<typename LayerType>
void assert_model_field_name(
    const garlic::Module<LayerType>& module,
    const char* model_name,
    const char* field_name,
    const char* field_type
    ) {
  auto model = module.get_model(model_name);
  ASSERT_NE(model, nullptr);
  auto field = model->get_field(field_name);
  ASSERT_NE(field, nullptr);
  ASSERT_STREQ(field->get_name().c_str(), field_type);
}

template<typename LayerType>
void load_libyaml_module(garlic::Module<LayerType>& module, const char* filename) {
  auto module_document = get_libyaml_document(filename);
  auto parse_results = module.parse(module_document.get_view());
  ASSERT_TRUE(parse_results.valid);
}

template<typename LayerType>
garlic::ConstraintResult
validate_jsonfile(
    const garlic::Module<LayerType>& module,
    const char* model_name,
    const char* filename) {
  auto model = module.get_model(model_name);
  auto doc = get_rapidjson_document(filename);
  auto root = garlic::ModelConstraint<garlic::providers::rapidjson::JsonView>(model);
  return root.test(doc.get_view());
}

template<typename LayerType>
void assert_jsonfile_valid(
    const garlic::Module<LayerType>& module,
    const char* model_name,
    const char* filename,
    bool print=false) {
  auto result = validate_jsonfile(module, model_name, filename);
  if (print) print_constraint_result(result);
  ASSERT_TRUE(result.valid);
}

template<typename LayerType>
void assert_jsonfile_invalid(
    const garlic::Module<LayerType>& module,
    const char* model_name,
    const char* filename,
    bool print=false) {
  auto result = validate_jsonfile(module, model_name, filename);
  if (print) print_constraint_result(result);
  ASSERT_FALSE(result.valid);
}

#endif /* end of include guard: GARLIC_TEST_UTILITY_H */
