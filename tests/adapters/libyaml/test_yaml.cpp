#include <cstdio>

#include <algorithm>
#include <functional>
#include <deque>

#include <gtest/gtest.h>

#include <garlic/garlic.h>
#include <garlic/adapters/libyaml.h>

using namespace garlic::adapters::libyaml;

void print_document(YamlView value, int level) {
  std::string prefix = "";
  for(auto i = 0; i < level; i++) prefix += "....";
  if (value.is_bool()) {
    std::cout << prefix << "bool: " << (value.get_bool() ? "true" : "false") << std::endl;
  } else if (value.is_double()) {
    std::cout << prefix << "double: " << value.get_double() << std::endl;
  } else if (value.is_null()) {
    std::cout << prefix << "null" << std::endl;
  } else if (value.is_object()) {
    for(const auto& item : value.get_object()) {
      std::cout << prefix << " + Member" << std::endl;
      print_document(item.key, level + 1);
      print_document(item.value, level + 1);
    }
  } else if (value.is_list()) {
    for(const auto& item : value.get_list()) {
      print_document(item, level + 1);
    }
  } else if (value.is_string()) {
    std::cout << prefix << "string: " << value.get_string_view() << std::endl;
  }

}

TEST(YamlCpp, ProtocolTest) {
  auto file = fopen("data/test.yaml", "r");
  auto result = load(file);
  ASSERT_TRUE(result);
  print_document(result->get_view(), 0);

  YamlView view = result->get_view();

  auto assertions = std::deque<std::function<bool(const YamlView&)>> {
    [](const auto& value) { return value.is_double() && value.get_double() == 1.1; },
    [](const auto& value) { return value.is_int() && value.get_int() == 25; },
    [](const auto& value) { return value.is_string() && value.get_string() == "Test"; },
    [](const auto& value) { return value.is_bool() && value.get_bool(); },
    [](const auto& value) { return value.is_null(); },
  };

  auto values = *view.get_view().find_member("values");
  for (const auto& value : values.value.get_list()) {
    ASSERT_TRUE(assertions.front()(value));
    assertions.pop_front();
  }
}
