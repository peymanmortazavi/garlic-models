#include <algorithm>
#include <iostream>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <gtest/gtest.h>
#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;


TEST(Constraints, FieldConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/field_constraint/module.yaml");

  assert_jsonfile_valid(module, "Account", "data/field_constraint/good.json");
  assert_jsonfile_invalid(module, "Account", "data/field_constraint/bad1.json");
  assert_jsonfile_invalid(module, "Account", "data/field_constraint/bad2.json");
  assert_jsonfile_invalid(module, "AccountCustomMessage", "data/field_constraint/bad2.json");

  // Test messages
  {
    auto results = validate_jsonfile(module, "Account", "data/field_constraint/bad1.json");
    ASSERT_STREQ(results.details[0].reason.data(), "Invalid user object.");
  }

  {
    auto results = validate_jsonfile(module, "Account", "data/field_constraint/bad2.json");
    ASSERT_STREQ(results.details[0].reason.data(), "Invalid email!");
  }

  {
    auto results = validate_jsonfile(module, "AccountCustomMessage", "data/field_constraint/bad2.json");
    ASSERT_STREQ(results.details[0].details[0].reason.data(), "Invalid email!");
  }

  assert_model_has_field_with_constraints(module, "Account", "user", {"User"});
}

TEST(Constraints, AnyConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "AnyTest", "data/special_constraints/any_good1.json");
  assert_jsonfile_valid(module, "AnyTest", "data/special_constraints/any_good2.json");
  assert_jsonfile_invalid(module, "AnyTest", "data/special_constraints/any_bad1.json");
}

TEST(Constraints, ListConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "ListTest", "data/special_constraints/list_good1.json");
  assert_jsonfile_invalid(module, "ListTest", "data/special_constraints/list_bad1.json");
  assert_jsonfile_invalid(module, "ListTest", "data/special_constraints/list_bad2.json");
}

TEST(Constraints, TupleConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "TupleTest", "data/special_constraints/tuple_good1.json");
  assert_jsonfile_valid(module, "TupleTest", "data/special_constraints/tuple_good2.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad1.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad2.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad3.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad4.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad5.json");
  assert_jsonfile_invalid(module, "TupleTest", "data/special_constraints/tuple_bad6.json");
}

TEST(Constraints, MapConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "MapTest", "data/special_constraints/map_good1.json");
  assert_jsonfile_invalid(module, "MapTest", "data/special_constraints/map_bad1.json");
  assert_jsonfile_invalid(module, "MapTest", "data/special_constraints/map_bad2.json");
  assert_jsonfile_invalid(module, "MapTest", "data/special_constraints/map_bad3.json");
}

TEST(Constraints, AllConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "AllTest", "data/special_constraints/all_good1.json");
  assert_jsonfile_invalid(module, "AllTest", "data/special_constraints/all_bad1.json");
  assert_jsonfile_invalid(module, "AllTest", "data/special_constraints/all_bad2.json");
}

TEST(Constraints, StopFeature) {
  FlatModule module;
  load_libyaml_module(module, "data/constraint/module.yaml");

  auto result = validate_jsonfile(module, "User", "data/constraint/bad1.json");
  ASSERT_STREQ(result.details[0].details[0].reason.data(), "Invalid Email!");
  ASSERT_EQ(result.details[0].details[0].details.size(), 0);

  ASSERT_STREQ(result.details[1].details[0].reason.data(), "Invalid Email!");
  ASSERT_EQ(result.details[1].details[0].details.size(), 1);

  ASSERT_STREQ(result.details[2].reason.data(), "Invalid Email!");
  ASSERT_EQ(result.details[2].details.size(), 0);
}

TEST(Constraints, LiteralConstraint) {
  FlatModule module;
  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "LiteralTest", "data/special_constraints/literal_good1.json");

  auto result = validate_jsonfile(module, "LiteralTest", "data/special_constraints/literal_bad1.json");
  ASSERT_EQ(result.details.size(), 6);
}
