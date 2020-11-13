#include <deque>
#include <string>
#include <gtest/gtest.h>
#include <garlic/utility.h>
#include "test_utility.h"
#include "test_protocol.h"

using namespace std;
using namespace garlic;


TEST(Utility, StringSplitter) {
  auto assert_text = [](const auto& text, deque<string> parts) {
    lazy_string_splitter splitter{text};
    for (const auto& part : parts) {
      EXPECT_EQ(splitter.next(), part) << text;
    }
    ASSERT_TRUE(splitter.next().empty());
    ASSERT_TRUE(splitter.next().empty());
  };
  assert_text("a.b.c", {"a", "b", "c"});
  assert_text("..a..b..c..", {"a", "b", "c"});
  assert_text("a", {"a"});
  assert_text("", {});
  assert_text("...", {});
}

TEST(Utility, Resolve) {
  auto assert_resolve = [](const auto& value, const char* path, const auto& cb) {
    auto fail = true;
    resolve(value, path, [&fail, &cb](const auto& item){
        fail = false;
        cb(item);
        });
    if (fail) {
      FAIL() << "Resolve never called the callback.";
    }
  };
  auto assert_view = [&assert_resolve](const auto& value) {
    assert_resolve(value, "user.first_name", [](const auto& item) {
        test_readonly_string_value(item, "Peyman");
        });
    assert_resolve(value, "user.numbers.3", [](const auto& item) {
        test_readonly_int_value(item, 4);
        });
  };
  {
    auto value = get_rapidjson_document("data/resolve/file.json");
    assert_view(value.get_view());
  }
  {
    auto value = get_libyaml_document("data/resolve/file.yaml");
    assert_view(value.get_view());
  }
  {
    auto value = get_yamlcpp_node("data/resolve/file.yaml");
    assert_view(value.get_view());
  }
}
