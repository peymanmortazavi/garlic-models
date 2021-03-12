#include "test_utility.h"

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

void
print_constraint_result(
    const garlic::ConstraintResult& result,
    int level) {
  if (result.is_valid()) {
    std::cout << "Passed all the checks." << std::endl;
  } else {
    std::string space = "";
    for (auto i = 0; i < level; i++) { space += "  "; }
    if (result.is_field()) {
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

void
assert_field_constraint_result(
    const garlic::ConstraintResult& results,
    const char* name) {
  ASSERT_FALSE(results.is_valid());
  ASSERT_TRUE(results.is_field());
  ASSERT_STREQ(results.name.data(), name);
}

void
assert_constraint_result(
    const garlic::ConstraintResult& results,
    const char* name,
    const char* message) {
  ASSERT_FALSE(results.is_valid());
  ASSERT_STREQ(results.name.data(), name);
  ASSERT_STREQ(results.reason.data(), message);
}

void assert_field_constraints(const garlic::FlatField& field, NameQueue names) {
  std::for_each(
      field.begin_constraints(), field.end_constraints(),
      [&names](const garlic::FlatConstraint& constraint) {
        ASSERT_STREQ(constraint.context().name.data(), names.front().data());
        names.pop_front();
      });
}

void assert_model_fields(const garlic::FlatModel& model, NameQueue names) {
  for(const auto& name : names) {
    ASSERT_NE(model.find_field(name), model.end_field());
  }
}

void assert_module_structure(const garlic::FlatModule& module, ModuleStructure structure) {
  for (const auto& model_structure : structure) {
    auto model = module.get_model(model_structure.first);
    ASSERT_NE(model, nullptr);
    for (const auto& field_structure : model_structure.second) {
      auto field = model->get_field(field_structure.first);
      assert_field_constraints(*field, std::move(field_structure.second));
    }
  }
}
