#include <algorithm>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml.h>
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include "utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;
using namespace garlic::providers::yamlcpp;
using namespace garlic::providers::libyaml;
using namespace std;


TEST(FieldValidation, Basic) {
  auto field = make_field<CloveView>("HTTPHeader");
  field->add_constraint<TypeConstraint>(TypeFlag::String);
  field->add_constraint<RegexConstraint>("[a-zA-Z0-9-_]+:\\s?[a-zA-Z0-9-_\\/]+");

  CloveDocument v;
  v.get_reference().set_string("Content-Type: application/garlic");

  auto result = field->test(v.get_view());
  ASSERT_TRUE(result.is_valid());

  v.get_reference().set_string("10.0.0.1");
  result = field->test(v.get_view());
  ASSERT_FALSE(result.is_valid());
  ASSERT_EQ(result.failures.size(), 1);
}

TEST(FieldValidation, ConstraintSkipping) {
  auto field = make_field<CloveView>("HTTPHeader");
  field->add_constraint<TypeConstraint>(TypeFlag::String);
  field->add_constraint<RegexConstraint>("\\d{1,3}");
  field->add_constraint<RegexConstraint>("\\w");

  CloveDocument v;
  v.get_reference().set_bool(true);

  auto result = field->test(v.get_view());
  ASSERT_FALSE(result.is_valid());
  ASSERT_EQ(result.failures.size(), 1);  // since the first constraint is fatal, only one is expected.
  ASSERT_STREQ(result.failures[0].name.data(), "type_constraint");

  v.get_reference().set_string("");
  result = field->test(v.get_view());
  ASSERT_FALSE(result.is_valid());
  ASSERT_EQ(result.failures.size(), 2);  // we should get two failures because they are not fatal.
  assert_constraint_result(result.failures[0], "regex_constraint", "invalid value.");
  assert_constraint_result(result.failures[1], "regex_constraint", "invalid value.");
}


TEST(ModelParsing, Basic) {
  // load a very basic module without using more sophisticated features.

  auto test_module = [](const Module<CloveView>& module) {
    auto date_field = module.get_field("DateTime");
    ASSERT_NE(date_field, nullptr);
    ASSERT_STREQ(date_field->get_properties().name.data(), "DateTime");  // named field.
    ASSERT_EQ(date_field->get_properties().constraints.size(), 2);  // a type and regex constraint.

    auto user_model = module.get_model("User");
    ASSERT_NE(user_model, nullptr);
    ASSERT_EQ(user_model->get_properties().field_map.size(), 4);

    auto root = ModelConstraint<CloveView>(user_model);

    CloveDocument v;
    auto ref = v.get_reference();
    ref.set_object();
    ref.add_member("first_name", "Garlic");
    ref.add_member("last_name", "Models");
    ref.add_member("birthdate", "1/4/1993");
    ref.add_member("registration_date", "1/4/21");

    auto results = root.test(v.get_view());
    ASSERT_TRUE(results.valid);

    ref.add_member("birthdate", "no good date");
    ref.add_member("registration_date", "empty");

    results = root.test(v.get_view());
    ASSERT_FALSE(results.valid);
    ASSERT_EQ(results.details.size(), 2);
    assert_field_constraint_result(results.details[0], "birthdate");
    assert_constraint_result(results.details[0].details[0], "date_constraint", "bad date time.");
    assert_field_constraint_result(results.details[1], "registration_date");
    assert_constraint_result(results.details[1].details[0], "date_constraint", "bad date time.");
  };

  // JSON module using rapidjson.
  {
    auto module = Module<CloveView>();
    auto document = get_rapidjson_document("data/basic_module.json");
    auto view = document.get_view();
    auto parse_result = module.parse(view);
    ASSERT_TRUE(parse_result.valid);
    test_module(module);
  }

  // YAML module using yaml-cpp
  {
    auto module = Module<CloveView>();
    auto node = get_yamlcpp_node("data/basic_module.yaml");
    auto parse_result = module.parse(node);
    ASSERT_TRUE(parse_result.valid);
    test_module(module);
  }

  // YAML module using libyaml
  {
    auto module = Module<CloveView>();
    auto file_handle = fopen("data/basic_module.yaml", "r");
    auto doc = garlic::providers::libyaml::Yaml::load(file_handle);
    YamlView view = doc.get_view();
    auto parse_result = module.parse(view);
    ASSERT_TRUE(parse_result.valid);
    test_module(module);
  }
}

TEST(ModelParsing, ForwardDeclarations) {
  // load a module full of forward dependencies to test and make sure all definitions get loaded properly.
  auto module = Module<CloveView>();

  auto document = get_rapidjson_document("data/forward_fields.json");
  auto view = document.get_view();

  auto parse_result = module.parse(view);
  ASSERT_TRUE(parse_result.valid);

  map<string, deque<string>> expectations = {
    {"NoDependencyField",           {"c0"}},
    {"RegularDependencyField",      {"c0", "c1"}},
    {"RegularAlias",                {"c0", "c1"}},
    {"ForwardDependencyField",      {"FieldContainerModel", "c4", "c2"}},
    {"ForwardDependencyAliasField", {"FieldContainerModel", "c4", "c3"}},
    {"ForwardAlias",                {"FieldContainerModel", "c4"}},
    {"TestModel",                   {"FieldContainerModel"}},
    {"FutureField",                 {"FieldContainerModel", "c4"}},
  };

  for (const auto& item : expectations) {
    auto field_ptr = module.get_field(item.first);
    ASSERT_NE(field_ptr, nullptr);
    assert_field_constraints(*field_ptr, item.second);
  }

  auto model_ptr = module.get_model("FieldContainerModel");
  ASSERT_NE(model_ptr, nullptr);

  expectations = {
    {"no_dependency_field",            {"c0"}},
    {"regular_dependency_field",       {"c0", "c1"}},
    {"regular_alias",                  {"c0", "c1"}},
    {"forward_dependency_field",       {"FieldContainerModel", "c4", "c2"}},
    {"forward_dependency_alias_field", {"FieldContainerModel", "c4", "c3"}},
    {"forward_alias",                  {"FieldContainerModel", "c4"}},
    {"inner_model",                    {"FieldContainerModel"}},
    {"future_field",                   {"FieldContainerModel", "c4"}},
    {"anonymous_field",                {"FieldContainerModel", "c4", "c5"}},
  };
  for(const auto& item : model_ptr->get_properties().field_map) {
    if (auto it = expectations.find(item.first); it != expectations.end()) {
      assert_field_constraints(*item.second, it->second);
      expectations.erase(it);
    }
  }
  ASSERT_TRUE(expectations.empty());
};


TEST(ModelParsing, FieldConstraints) {
  auto module = Module<JsonView>();

  auto model_document = get_libyaml_document("data/field_constraint/module.yaml");
  auto parse_results = module.parse(model_document.get_view());
  ASSERT_TRUE(parse_results.valid);

  auto model = module.get_model("Account");
  auto root = ModelConstraint<JsonView>(model);

  auto good_document = get_rapidjson_document("data/field_constraint/good.json");
  auto result = root.test(good_document.get_view());
  ASSERT_TRUE(result.valid);

  auto bad_document = get_rapidjson_document("data/field_constraint/bad.json");
  result = root.test(bad_document.get_view());
  ASSERT_FALSE(result.valid);
  print_constraint_result(result);
}

TEST(ModelParsing, AnyConstraint) {
  auto module = Module<JsonView>();

  auto model_document = get_libyaml_document("data/special_constraints/module.yaml");
  auto parse_results = module.parse(model_document.get_view());
  ASSERT_TRUE(parse_results.valid);

  auto get_result = [&module](const char* name, const char* filename) {
    auto model = module.get_model(name);
    auto root = ModelConstraint<JsonView>(model);
    auto doc = get_rapidjson_document(filename);
    return root.test(doc.get_view());
  };

  auto assert_good = [&get_result](const char* name, const char* filename) {
    ASSERT_TRUE(get_result(name, filename).valid);
  };

  auto assert_bad = [&get_result](const char* name, const char* filename) {
    auto result = get_result(name, filename);
    print_constraint_result(result);
    ASSERT_FALSE(result.valid);
  };

  assert_good("AnyTest", "data/special_constraints/any_good1.json");
  assert_good("AnyTest", "data/special_constraints/any_good2.json");
  assert_bad("AnyTest", "data/special_constraints/any_bad1.json");
}

TEST(ModelParsing, ModelInheritance) {
  auto module = Module<JsonView>();

  auto model_document = get_libyaml_document("data/model_inheritance/module.yaml");
  auto parse_results = module.parse(model_document.get_view());
  ASSERT_TRUE(parse_results.valid);

  assert_model_fields(module, "BaseUser", {"id", "username", "password"});
  assert_model_fields(module, "AdminUser", {"id", "username", "password", "is_super"});
  assert_model_fields(module, "AdminUser", {"id", "username", "password", "is_super"});
  assert_model_fields(module, "MobileUser", {"id", "username", "password"});
  assert_model_fields(module, "BaseQuery", {"skip", "limit"});
  assert_model_fields(module, "UserQuery", {"skip", "limit", "id", "username"});
}

TEST(ModelParsing, ModelInheritanceLazy) {
  auto module = Module<JsonView>();

  auto model_document = get_libyaml_document("data/model_inheritance/lazy_load.yaml");
  auto parse_results = module.parse(model_document.get_view());
  ASSERT_TRUE(parse_results.valid);

  assert_model_fields(module, "Model1", {"model3", "model4"});
  assert_model_fields(module, "Model2", {"model3", "model4"});
  assert_model_fields(module, "Model2_with_exclude", {"model3"});
  assert_model_fields(module, "Model2_without_forwarding", {"model3"});
  assert_model_field_constraints(module, "Model2_without_forwarding", "model3", {"type_constraint"});
  assert_model_field_constraints(module, "Model2_with_exclude", "model3", {"Model3"});
}
