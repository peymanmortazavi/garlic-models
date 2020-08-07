#include "garlic/basic.h"
#include "garlic/constraints.h"
#include "garlic/models.h"
#include <algorithm>
#include <garlic/garlic.h>
#include <gtest/gtest.h>
#include <memory>
#include <rapidjson/rapidjson.h>
#include <rapidjson/istreamwrapper.h>
#include <fstream>
#include <deque>
#include <iostream>

using namespace garlic;
using namespace rapidjson;
using namespace std;


Document get_json_document(const std::string& name) {
  ifstream ifs(name);
  IStreamWrapper isw(ifs);
  Document d;
  d.ParseStream(isw);
  return d;
}

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


template<ReadableLayer LayerType>
void print_constraints(const FieldPropertiesOf<LayerType>& props) {
  bool first = true;
  for (const auto& c : props.constraints) {
    if (first) cout << c->get_name();
    else cout << ", " << c->get_name();
    first = false;
  }
}


auto assert_field_constraint_result(const ConstraintResult& results, const char* name) {
  ASSERT_FALSE(results.valid);
  ASSERT_TRUE(results.field);
  ASSERT_STREQ(results.name.data(), name);
}


auto assert_constraint_result(const ConstraintResult& results, const char* name, const char* message) {
  ASSERT_FALSE(results.valid);
  ASSERT_STREQ(results.name.data(), name);
  ASSERT_STREQ(results.reason.data(), message);
}


template<ReadableLayer LayerType>
auto assert_field_constraints(const Field<LayerType>& field, deque<string> names) {
  for(const auto& constraint : field.get_properties().constraints) {
    ASSERT_STREQ(constraint->get_name().data(), names.front().data());
    names.pop_front();
  }
}


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
  auto module = ModelContainer<CloveView>();

  auto document = get_json_document("data/basic_module.json");
  auto view = JsonView{document};

  auto parse_result = module.parse(view);
  ASSERT_TRUE(parse_result.valid);

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
}

TEST(ModelParsing, ForwardDeclarations) {
  // load a module full of forward dependencies to test and make sure all definitions get loaded properly.
  auto module = ModelContainer<CloveView>();

  auto document = get_json_document("data/forward_fields.json");
  auto view = JsonView{document};

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
}

TEST(GarlicModel, JsonParser) {
  auto document = get_json_document("data/models.json");
  auto value = JsonView{document};
  auto definitions = ModelContainer<CloveView>();
  auto parse_result = definitions.parse(value);
  std::cout << parse_result.valid << std::endl;
  auto model = definitions.get_model("User");
  std::cout << "Model Name: " << model->get_properties().name << std::endl;
  std::cout << "Meta: " << std::endl;
  for (const auto& item : model->get_properties().meta) {
    std::cout << "  - " << item.first << " : " << item.second << std::endl;
  }

  std::cout << "Fields: " << std::endl;
  for (const auto& item : model->get_properties().field_map) {
    std::cout << item.first << " (" << item.second->get_properties().constraints.size() << ": ";
    print_constraints<CloveView>(item.second->get_properties());
    cout << ")" << std::endl;
  }

  std::cout << std::endl;

  auto root = ModelConstraint<CloveView>(model);

  CloveDocument v;
  auto ref = v.get_reference();
  ref.set_object();
  ref.add_member("first_name", "Peyman");
  ref.add_member("last_name", "Mortazavi");
  ref.add_member("zip_code", "123");
  ref.add_member("age", 18);
  ref.add_member("birthdate", "1/4/1993");

  CloveValue company{v};
  auto cref = company.get_reference();
  cref.set_object();
  cref.add_member("name", "Microsoft");
  cref.add_member("founded_on", "a");

  ref.add_member("company", company.move_data());

  auto result = root.test(v.get_view());
  print_result(result);
}
