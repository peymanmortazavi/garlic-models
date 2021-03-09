#include "garlic/clove.h"
#include <iostream>
#include <garlic/layer.h>
#include <garlic/constraints.h>

using namespace garlic;

int main() {
  garlic::CloveDocument doc;
  doc.set_string("Peyman");

  auto constraint = FlatConstraint::make<string_literal_tag>("Peyman", "name_constraint");
  auto cresult = constraint.test(doc.get_view());
  std::cout << cresult.is_valid() << std::endl;
  std::cout << cresult.name << std::endl;
  std::cout << cresult.reason << std::endl;
  std::cout << sizeof(FlatConstraint) << std::endl;
  return 0;
}
