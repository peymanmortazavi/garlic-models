#include "garlic/models.h"
#include <garlic/garlic.h>
#include <gtest/gtest.h>
#include <memory>

using namespace garlic;


TEST(GarlicModel, FieldValidation) {
  auto field = Field<CloveView>("name");
  field.add_constraint<TypeConstraint>(TypeFlag::String);
  field.add_constraint<RegexConstraint>("Name:\\s?\\d{1,3}");

  CloveDocument v;
  v.get_reference().set_string("Name: 256");
  auto result = field.validate(v.get_view());
  if (result.is_valid()) {
    std::cout << "Valid Model" << std::endl;
  } else {
    for(const auto& failure : result.failures) {
      std::cout << failure.get_constraint().get_name() << ": " << failure.get_result().invalid_reason << std::endl;
    }
  }
}
