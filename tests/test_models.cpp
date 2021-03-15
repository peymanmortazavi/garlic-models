#include <algorithm>
#include <iostream>

#include <gtest/gtest.h>

#include "test_utility.h"

using namespace garlic;
using namespace std;


TEST(Model, Basic) {
  auto model = make_model("User");
  model->add_field("name", make_field({make_constraint<regex_tag>("\\w{3,12}")}));
  model->add_field("score", make_field({make_constraint<type_tag>(TypeFlag::Integer)}), false);

  ASSERT_EQ(model->get_field("non_existing"), nullptr);
  auto name_field = model->get_field("name");
  auto score_field = model->get_field("score");
  ASSERT_NE(name_field, nullptr);
  ASSERT_NE(score_field, nullptr);
  ASSERT_NE(name_field, score_field);
}

TEST(Field, Basic) {
  auto field1 = make_field("Field 1", {});  // empty initializer.
  field1->add_constraint<regex_tag>("\\d{1,3}", "c1");
  ASSERT_EQ(field1->begin_constraints()->context().name, text("c1"));
}
