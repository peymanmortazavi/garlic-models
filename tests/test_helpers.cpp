#include <deque>
#include <string>

#include <gtest/gtest.h>

#include <garlic/utility.h>
#include <garlic/clove.h>

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
    resolve_layer_cb(value, path, [&fail, &cb](const auto& item){
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
    ASSERT_STREQ(resolve<const char*>(value, "user.first_name", ""), "Peyman");
    ASSERT_STREQ(resolve<const char*>(value, "random", ""), "");
    ASSERT_EQ(safe_resolve(value, "user.first_name", 0), 0);
    ASSERT_EQ(safe_resolve(value, "random", 0), 0);
    ASSERT_STREQ(safe_resolve<const char*>(value, "user.first_name", ""), "Peyman");
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
}

inline static CloveRef get_clove_object() {
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

inline static CloveRef get_clove_list() {
  static std::unique_ptr<CloveDocument> doc;
  if (!doc) {
    doc = std::make_unique<CloveDocument>();
    auto ref = doc->get_reference();
    ref.set_list();
    ref.push_back("text");
    ref.push_back(12);
  }
  return doc->get_reference();
}

TEST(Encoding, DefaultLayerCopy) {
  auto doc = get_rapidjson_document("data/test.json");
  CloveDocument target;
  copy_layer(doc.get_view(), target.get_reference());
  ASSERT_TRUE(cmp_layers(doc.get_view(), target.get_view()));
}

TEST(Utility, Get) {
  auto result = get<std::string>(get_clove_object(), "key");
  ASSERT_STREQ(result.c_str(), "value");
}

TEST(Utility, GetIndex) {
  ASSERT_STREQ(get<const char*>(get_clove_list(), 0), "text");
  ASSERT_EQ(get<int>(get_clove_list(), 1), 12);
}

TEST(Utility, GetWithDefault) {
  auto key1 = get<std::string>(get_clove_object(), "key", "default");
  ASSERT_STREQ(key1.c_str(), "value");

  auto key2 = get<std::string>(get_clove_object(), "non_existing_key", "default");
  ASSERT_STREQ(key2.c_str(), "default");
}

TEST(Utility, GetIndexWithDefault) {
  ASSERT_STREQ(get<const char*>(get_clove_list(), 0, ""), "text");
  ASSERT_STREQ(get<const char*>(get_clove_list(), 23, ""), "");  // non existing
  ASSERT_EQ(get<int>(get_clove_list(), 1, 0), 12);
  ASSERT_EQ(get<int>(get_clove_list(), 23, 0), 0);  // non existing
}

TEST(Utility, GetWithCallback) {
  std::string value = "default";
  get_cb<std::string>(get_clove_object(), "non_existing_key", [&value](auto&& result) {
      value = result;
      });
  ASSERT_STREQ(value.c_str(), "default");

  get_cb<std::string>(get_clove_object(), "key", [&value](auto&& result) {
      value = result;
      });
  ASSERT_STREQ(value.c_str(), "value");
}

TEST(Utility, GetIndexWithCallback) {
  std::string text = "default";
  int number = 0;
  get_cb<std::string>(get_clove_list(), 23, [&text](auto&& result) {
      text = result;
      });
  ASSERT_STREQ(text.c_str(), "default");

  get_cb<std::string>(get_clove_list(), 0, [&text](auto&& result) {
      text = result;
      });
  ASSERT_STREQ(text.c_str(), "text");

  get_cb<int>(get_clove_list(), 1, [&number](auto&& result) {
      number = result;
      });
  ASSERT_EQ(number, 12);
}

TEST(Utility, SafeGetWithCallback) {
  std::string value = "default";
  safe_get_cb<std::string>(get_clove_object(), "non_existing_key", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_clove_object(), "count", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_clove_object(), "key", [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "value");
}

TEST(Utility, SafeGetIndexWithCallback) {
  std::string value = "default";
  safe_get_cb<std::string>(get_clove_list(), 23, [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_clove_list(), 1, [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "default");  // should remain untouched.
  safe_get_cb<std::string>(get_clove_list(), 0, [&](auto&& v) { value = v; });
  ASSERT_STREQ(value.c_str(), "text");
}

TEST(Utility, SafeGetWithDefault) {
  std::string value = safe_get<std::string>(get_clove_object(), "count", "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_clove_object(), "no key", "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_clove_object(), "key", "default");
  ASSERT_STREQ(value.c_str(), "value");
}

TEST(Utility, SafeGetIndexWithDefault) {
  std::string value = safe_get<std::string>(get_clove_list(), 1, "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_clove_list(), 23, "default");
  ASSERT_STREQ(value.c_str(), "default");
  value = safe_get<std::string>(get_clove_list(), 0, "default");
  ASSERT_STREQ(value.c_str(), "text");
}
