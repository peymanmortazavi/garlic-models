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
  auto doc = get_rapidjson_document("data/resolve/file.json");
  auto assert_resolve = [&doc](const char* path, const auto& cb) {
    auto fail = true;
    resolve(doc.get_view(), path, [&fail, &cb](const auto& item){
        fail = false;
        cb(item);
        });
    if (fail) {
      FAIL() << "Resolve never called the callback.";
    }
  };
  assert_resolve("user.first_name", [](const auto& item) {
      test_readonly_string_value(item, "Peyman");
      });
  assert_resolve("user.numbers.3", [](const auto& item) {
      test_readonly_int_value(item, 4);
      });
}
