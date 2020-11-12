#include <deque>
#include <string>
#include <gtest/gtest.h>
#include <garlic/utility.h>

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
