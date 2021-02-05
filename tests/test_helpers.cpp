#include <deque>
#include <string>
#include <gtest/gtest.h>
#include <garlic/utility.h>
#include "garlic/clove.h"
#include "garlic/encoding.h"
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

inline static CloveRef get_test_clove() {
  static std::unique_ptr<CloveDocument> doc;
  if (!doc) {
    doc = std::make_unique<CloveDocument>();
    auto ref = doc->get_reference();
    ref.set_object();
    ref.add_member("key", "value");
    ref.add_member("count", 1);
  }
  return doc->get_reference();
}

TEST(Utility, Get) {
  auto result = get<std::string>(get_test_clove(), "key");
  ASSERT_STREQ(result.c_str(), "value");
}

TEST(Utility, GetWithDefault) {
  auto key1 = get<std::string>(get_test_clove(), "key", "default");
  ASSERT_STREQ(key1.c_str(), "value");

  auto key2 = get<std::string>(get_test_clove(), "non_existing_key", "default");
  ASSERT_STREQ(key2.c_str(), "default");
}

TEST(Utility, GetWithCallback) {
  std::string value = "default";
  get_cb<std::string>(get_test_clove(), "non_existing_key", [&value](auto&& result) {
      value = result;
      });
  ASSERT_STREQ(value.c_str(), "default");

  get_cb<std::string>(get_test_clove(), "key", [&value](auto&& result) {
      value = result;
      });
  ASSERT_STREQ(value.c_str(), "value");
}

TEST(Utility, SafeGetWithCallback) {
  std::string value = "default";
  safe_get_cb<std::string>(get_test_clove(), "non_existing_key", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_test_clove(), "count", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_test_clove(), "key", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "value");
}

TEST(Utility, SafeGetWithDefault) {
  std::string value = safe_get<std::string>(get_test_clove(), "count", "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_test_clove(), "no key", "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_test_clove(), "key", "default");
  ASSERT_STREQ(value.c_str(), "value");
}
