#include <cstring>
#include <gtest/gtest.h>
#include <garlic/containers.h>
#include <string>

using namespace garlic;

TEST(GarlicText, ConstCharConstructor) {
  char static_text[5] = "abcd";
  text a = static_text;
  text b {static_text};
  text c (static_text);

  text copy(static_text, text_type::copy);

  strncpy(static_text, "1234", 4); // update the text.
  ASSERT_STREQ(a.data(), static_text);
  ASSERT_STREQ(b.data(), static_text);
  ASSERT_STREQ(c.data(), static_text);
  ASSERT_STRNE(copy.data(), static_text);
  ASSERT_STREQ(copy.data(), "abcd");
}

TEST(GarlicText, StdStringConstructor) {
  std::string txt = "abcd";
  text a = txt;
  text b {txt};
  text c (txt);

  text copy(txt, text_type::copy);

  txt = "1234";
  ASSERT_STREQ(a.data(), txt.data());
  ASSERT_STREQ(b.data(), txt.data());
  ASSERT_STREQ(c.data(), txt.data());
  ASSERT_STRNE(copy.data(), txt.data());
  ASSERT_STREQ(copy.data(), "abcd");
}

TEST(GarlicText, Copy) {
  text a = "abcd";
  text b ("abcd", text_type::copy);

  text c = a.copy();
  text d = b.copy();

  text copy1 = a.deep_copy();
  text copy2 = b.deep_copy();

  ASSERT_EQ(c.data(), a.data());  // same pointers.
  ASSERT_EQ(d.data(), b.data());  // same pointers.
  ASSERT_NE(copy1.data(), a.data());  // different pointers.
  ASSERT_NE(copy2.data(), b.data());  // different pointers.
}
