#include <algorithm>
#include <iostream>

#include <gtest/gtest.h>

#include <garlic/module.h>
#include <garlic/constraints.h>

using namespace std;
using namespace garlic;


TEST(Module, Basic) {
  Module module;

  std::shared_ptr<Field> fields[] = {
    make_field("Field 1"),
    make_field("Field 2"),
  };

  std::shared_ptr<Model> models[] = {
    make_model("Model 1"),
    make_model("Model 2"),
  };

  // add all the fields.
  for (const auto& item : fields) {
    module.add_field(item);
  }

  // add all the models.
  for (const auto& item : models) {
    module.add_model(item);
  }

  // test find field methods.
  for (const auto& item : fields) {
    auto it = module.find_field(item->name());
    ASSERT_EQ(it->second, item);
    ASSERT_EQ(module.get_field(item->name()), item);
  }
  ASSERT_EQ(module.get_field("Random Field"), nullptr);

  // test find model methods.
  for (const auto& item : models) {
    auto it = module.find_model(item->name());
    ASSERT_EQ(it->second, item);
    ASSERT_EQ(module.get_model(item->name()), item);
  }
  ASSERT_EQ(module.get_model("Random Model"), nullptr);
}


TEST(Module, AvoidDuplicates) {
  Module module;
  ASSERT_TRUE(module.add_field(make_field("Field 1")));
  ASSERT_TRUE(module.add_field("Field 2", module.get_field("Field 1")));  // register an alias, it's ok!
  auto result = module.add_field(make_field("Field 1"));
  ASSERT_FALSE(result);
  ASSERT_EQ(result.error().value(), static_cast<int>(GarlicError::Redefinition));
}
