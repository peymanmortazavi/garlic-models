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

void
assert_field_constraint_result(
    const garlic::ConstraintResult& results,
    const char* name) {
  ASSERT_FALSE(results.valid);
  ASSERT_TRUE(results.field);
  ASSERT_STREQ(results.name.data(), name);
}

void
assert_constraint_result(
    const garlic::ConstraintResult& results,
    const char* name,
    const char* message) {
  ASSERT_FALSE(results.valid);
  ASSERT_STREQ(results.name.data(), name);
  ASSERT_STREQ(results.reason.data(), message);
}
