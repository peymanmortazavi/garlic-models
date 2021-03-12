#ifndef GARLIC_TEST_UTILITY_H
#define GARLIC_TEST_UTILITY_H

#include <cstdio>
#include <string>
#include <deque>
#include <garlic/garlic.h>
#include "garlic/flat_constraints.h"
#include "garlic/flat_module.h"
#include "garlic/parsing/module.h"
#include <garlic/constraints.h>
#include <garlic/models.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml/parser.h>
#include "gtest/gtest.h"


using NameQueue = std::deque<std::string>;
using ModelStructure = std::unordered_map<garlic::text, NameQueue>;  // map of fields to their constraints.
using ModuleStructure = std::unordered_map<garlic::text, ModelStructure>;  // map of models to their fields to their constraints.

garlic::providers::libyaml::YamlDocument
get_libyaml_document(const char * name);

garlic::providers::rapidjson::JsonDocument
get_rapidjson_document(const char* name);

garlic::providers::yamlcpp::YamlNode
get_yamlcpp_node(const char* name);

void print_constraint_result(const garlic::ConstraintResult& result, int level=0);
void assert_field_constraint_result(const garlic::ConstraintResult& results, const char* name);
void assert_constraint_result(const garlic::ConstraintResult& results, const char* name, const char* message);

template<GARLIC_VIEW LayerType>
void print_constraints(const garlic::FieldPropertiesOf<LayerType>& props) {
  bool first = true;
  for (const auto& c : props.constraints) {
    if (first) std::cout << c->get_name();
    else std::cout << ", " << c->get_name();
    first = false;
  }
}


/*
 * Asserts the given field has constraints by the names provided.
 */
void assert_field_constraints(const garlic::FlatField& field, NameQueue names);

/*
 * Asserts the given model has fields by the names provided.
 */
void assert_model_fields(const garlic::FlatModel& model, NameQueue names);

/*
 * Asserts the given module has a model with the specified name and that model has the given set of fields.
 */
void assert_model_fields(const garlic::FlatModule& module, const garlic::text& model_name, NameQueue names);

/*
 * Asserts the given model fields and their respective constraints are present in the module.
 */
void assert_module_structure(const garlic::FlatModule& module, ModuleStructure structure);

/*
 * Asserts the given model has a field by the name given and the field has the set of constraints provided.
 */
void assert_model_has_field_with_constraints(
    const garlic::FlatModel& model, const garlic::text& field_name, NameQueue constraints);

/*
 * Asserts there is a model in the provided module that has a field by the name provided and that field
 * has the set of constraints given in the arguments.
 */
void assert_model_has_field_with_constraints(
    const garlic::FlatModule& module, const garlic::text& model_name,
    const garlic::text& field_name, NameQueue constraints);

/*
 * Asserts there is a model in the given module by the specified name and it has a field by the name given.
 */
void assert_model_has_field_name(
    const garlic::FlatModule& module, const garlic::text& model_name,
    const garlic::text& key, const garlic::text& field_name);

/*
 * Loads the specified module with a yaml file describing a garlic module.
 */
void load_libyaml_module(garlic::FlatModule& module, const char* filename);

/*
 * Uses a model by the name specified in the given module to validate a JSON file.
 */
garlic::ConstraintResult
validate_jsonfile(const garlic::FlatModule& module, const garlic::text& model_name, const char* filename);

/*
 * Asserts the JSON file specified in the arguments is a valid instance of a model in the specified module.
 */
void assert_jsonfile_valid(
    const garlic::FlatModule& module, const garlic::text& model_name,
    const char* filename, bool print=false);

/*
 * Asserts the JSON file specified in the arguments is an invalid instance of a model in the specified module.
 */
void assert_jsonfile_invalid(
    const garlic::FlatModule& module, const garlic::text& model_name,
    const char* filename, bool print=false);

#endif /* end of include guard: GARLIC_TEST_UTILITY_H */
