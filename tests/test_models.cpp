#include "garlic/basic.h"
#include "garlic/models.h"
#include <algorithm>
#include <garlic/garlic.h>
#include <gtest/gtest.h>
#include <memory>

using namespace garlic;


void print_result(const ConstraintResult& result, int level=0) {
  if (result.valid) {
    std::cout << "Passed all the checks." << std::endl;
  } else {
    std::string space = "";
    for (auto i = 0; i < level; i++) { space += "  "; }
    if (result.field) {
      std::cout << space << "Name: " << result.name << std::endl;
    } else {
      std::cout << space << "Constraint: " << result.name << std::endl;
    }
    std::cout << space << "Reason: " << result.reason << std::endl << std::endl;
    std::for_each(result.details.begin(), result.details.end(), [&level](const auto& item){print_result(item, level + 1); });
  }
}


TEST(GarlicModel, FieldValidation) {
  auto field = std::make_shared<Field<CloveView>>("Name");
  field->add_constraint<TypeConstraint>(TypeFlag::String);
  field->add_constraint<RegexConstraint>("Name:\\s?\\d{1,3}");

  auto field2 = std::make_shared<Field<CloveView>>("IP Address");
  field2->add_constraint<RegexConstraint>("\\d{1,3}[.]\\d{1,3}[.]\\d{1,3}[.]\\d{1,3}");

  auto user_model = Model<CloveView>("User");
  user_model.add_field("name", field);
  user_model.add_field("location", field2);

  std::cout << user_model.get_properties().name << std::endl;

  auto root = ModelConstraint<CloveView>(std::move(user_model));


  CloveDocument v;
  auto ref = v.get_reference();
  ref.set_object();
  ref.add_member("name", "1");
  ref.add_member("location", "192.1.2");

  auto result = root.test(v.get_view());
  print_result(result);

  //auto result = field.validate(v.get_view());
  //if (result.is_valid()) {
  //  std::cout << "Valid Model" << std::endl;
  //} else {
  //  for(const auto& failure : result.failures) {
  //    std::cout << failure.get_constraint().get_name() << ": " << failure.get_result().invalid_reason << std::endl;
  //  }
  //}
}
