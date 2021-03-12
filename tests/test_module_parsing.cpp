#include <algorithm>
#include <iostream>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml/parser.h>
#include <gtest/gtest.h>
#include "garlic/parsing/module.h"
#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;
using namespace garlic::providers::yamlcpp;
using namespace garlic::providers::libyaml;
using namespace std;


//TEST(ModuleParsing, Basic) {
//  // load a very basic module without using more sophisticated features.

//  auto test_module = [](const FlatModule& module) {
//    auto date_field = module.get_field("DateTime");
//    ASSERT_NE(date_field, nullptr);
//    ASSERT_STREQ(date_field->get_name().data(), "DateTime");  // named field.
//    assert_field_constraints(*date_field, {"type_constraint", "date_constraint"});

//    auto user_model = module.get_model("User");
//    ASSERT_NE(user_model, nullptr);
//    assert_model_fields(*user_model, {"first_name", "last_name", "birthdate", "registration_date"});

//    assert_module_structure(module, {
//          {
//            "User",
//            {
//              {"first_name",        {"type_constraint"}},
//              {"last_name",         {"type_constraint", "range_constraint"}},
//              {"birthdate",         {"type_constraint", "date_constraint"}},
//              {"registration_date", {"type_constraint", "date_constraint"}},
//            }
//          }
//        });

//    auto root = make_constraint<model_tag>(user_model);

//    CloveDocument doc;
//    doc.set_object();
//    doc.add_member("first_name", "Garlic");
//    doc.add_member("last_name", "Models");
//    doc.add_member("birthdate", "1/4/1993");
//    doc.add_member("registration_date", "1/4/21");

//    auto results = root.test(doc.get_view());
//    ASSERT_TRUE(results.is_valid());

//    doc.add_member("birthdate", "no good date");
//    doc.add_member("registration_date", "empty");

//    results = root.test(doc);
//    ASSERT_FALSE(results.is_valid());
//    ASSERT_EQ(results.details.size(), 2);
//    assert_field_constraint_result(results.details[0], "birthdate");
//    assert_constraint_result(results.details[0].details[0], "date_constraint", "bad date time.");
//    assert_field_constraint_result(results.details[1], "registration_date");
//    assert_constraint_result(results.details[1].details[0], "date_constraint", "bad date time.");
//  };

//  // JSON module using rapidjson.
//  {
//    auto doc = get_rapidjson_document("data/basic/module.json");
//    auto result = parsing::load_module(doc);
//    ASSERT_TRUE(result);
//    test_module(*result);
//  }

//  // YAML module using libyaml
//  {
//    auto doc = get_libyaml_document("data/basic/module.yaml");
//    auto result = parsing::load_module(doc.get_view());
//    ASSERT_TRUE(result);
//    test_module(*result);
//  }
//}

TEST(ModuleParsing, ForwardDeclarations) {
  // load a module full of forward dependencies to test and make sure all definitions get loaded properly.
  auto doc = get_rapidjson_document("data/forward_fields/module.json");

  auto result = parsing::load_module(doc);
  ASSERT_TRUE(result);

  unordered_map<text, deque<string>> expectations = {
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
    auto field_ptr = result->get_field(item.first);
    ASSERT_NE(field_ptr, nullptr);
    assert_field_constraints(*field_ptr, item.second);
  }

  auto model_ptr = result->get_model("FieldContainerModel");
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
      assert_field_constraints(*item.second.field, it->second);
      expectations.erase(it);
    }
  }
  ASSERT_TRUE(expectations.empty());
};


TEST(ModuleParsing, ModelInheritance) {
  FlatModule module;
  load_libyaml_module(module, "data/model_inheritance/module.yaml");

  assert_model_fields(module, "BaseUser", {"id", "username", "password"});
  assert_model_fields(module, "AdminUser", {"id", "username", "password", "is_super"});
  assert_model_fields(module, "AdminUser", {"id", "username", "password", "is_super"});
  assert_model_fields(module, "MobileUser", {"id", "username", "password"});
  assert_model_fields(module, "BaseQuery", {"skip", "limit"});
  assert_model_fields(module, "UserQuery", {"skip", "limit", "id", "username"});

  assert_model_has_field_name(module, "MobileUser", "username", "StrictUserName");
}

//TEST(ModuleParsing, ModelInheritanceLazy) {
//  auto module = Module<JsonView>();

//  load_libyaml_module(module, "data/model_inheritance/lazy_load.yaml");

//  assert_model_fields(module, "Model1", {"model3", "model4"});
//  assert_model_fields(module, "Model2", {"model3", "model4"});
//  assert_model_fields(module, "Model2_with_exclude", {"model3"});
//  assert_model_fields(module, "Model2_without_forwarding", {"model3"});
//  assert_model_field_constraints(module, "Model2_without_forwarding", "model3", {"type_constraint"});
//  assert_model_field_constraints(module, "Model2_with_exclude", "model3", {"Model3"});

//  assert_model_field_name(module, "Model2_overriding", "model3", "IntegerField");
//  assert_model_field_name(module, "Model2_overriding", "model4", "Model4");
//}

//TEST(ModuleParsing, OptionalFields) {
//  auto module = Module<JsonView>();

//  load_libyaml_module(module, "data/optional_fields/module.yaml");

//  auto valid_names = vector<string>{"good1", "good2", "good3"};
//  auto invalid_names = vector<string>{"bad1","bad2", "bad3"};

//  for (const auto& name : valid_names) {
//    auto path = "data/optional_fields/" + name + ".json";
//    assert_jsonfile_valid(module, "User", path.data());
//    assert_jsonfile_valid(module, "Staff", path.data());
//  }

//  for (const auto& name : invalid_names) {
//    auto path = "data/optional_fields/" + name + ".json";
//    assert_jsonfile_invalid(module, "User", path.data());
//    assert_jsonfile_invalid(module, "Staff", path.data());
//  }
//}
