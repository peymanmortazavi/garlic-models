#include <algorithm>
#include <iostream>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <gtest/gtest.h>
#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;


TEST(Constraints, FieldConstraint) {
  auto module = Module<JsonView>();

  load_libyaml_module(module, "data/field_constraint/module.yaml");

  assert_jsonfile_valid(module, "Account", "data/field_constraint/good.json");
  assert_jsonfile_invalid(module, "Account", "data/field_constraint/bad.json");
}

TEST(Constraints, AnyConstraint) {
  auto module = Module<JsonView>();

  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "AnyTest", "data/special_constraints/any_good1.json");
  assert_jsonfile_valid(module, "AnyTest", "data/special_constraints/any_good2.json");
  assert_jsonfile_invalid(module, "AnyTest", "data/special_constraints/any_bad1.json");
}

TEST(Constraints, ListConstraint) {
  auto module = Module<JsonView>();

  load_libyaml_module(module, "data/special_constraints/module.yaml");

  assert_jsonfile_valid(module, "ListTest", "data/special_constraints/list_good1.json", true);
  assert_jsonfile_invalid(module, "ListTest", "data/special_constraints/list_bad1.json", true);
  assert_jsonfile_invalid(module, "ListTest", "data/special_constraints/list_bad2.json", true);
}
