#include <algorithm>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/providers/yaml-cpp.h>
#include <garlic/providers/libyaml/parser.h>
#include <gtest/gtest.h>
#include <iostream>
#include "test_utility.h"

using namespace garlic;
using namespace std;


TEST(FieldValidation, Basic) {
  auto field = make_field<CloveView>("HTTPHeader");
  field->add_constraint<TypeConstraint>(TypeFlag::String);
  field->add_constraint<RegexConstraint>("[a-zA-Z0-9-_]+:\\s?[a-zA-Z0-9-_\\/]+");

  CloveDocument v;
  v.get_reference().set_string("Content-Type: application/garlic");

  auto result = field->validate(v.get_view());
  ASSERT_TRUE(result.is_valid());

  v.get_reference().set_string("10.0.0.1");
  result = field->validate(v.get_view());
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

  auto result = field->validate(v.get_view());
  ASSERT_FALSE(result.is_valid());
  ASSERT_EQ(result.failures.size(), 1);  // since the first constraint is fatal, only one is expected.
  ASSERT_STREQ(result.failures[0].name.data(), "type_constraint");

  v.get_reference().set_string("");
  result = field->validate(v.get_view());
  ASSERT_FALSE(result.is_valid());
  ASSERT_EQ(result.failures.size(), 2);  // we should get two failures because they are not fatal.
  assert_constraint_result(result.failures[0], "regex_constraint", "invalid value.");
  assert_constraint_result(result.failures[1], "regex_constraint", "invalid value.");
}
