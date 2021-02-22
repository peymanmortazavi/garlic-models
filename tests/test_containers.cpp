#include <algorithm>
#include <cstring>
#include <gtest/gtest.h>
#include <garlic/containers.h>
#include <iterator>
#include <string>

using namespace std;
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

TEST(GarlicText, StaticNoText) {
  ASSERT_EQ(text::no_text().data(), text::no_text().data());
}


TEST(GarlicSequence, Basic) {
  sequence<double> numbers;  // default ctor
  numbers.push_back(12);  // rvalue
  double x = 13;
  numbers.push_back(x);  // lvalue
  const double& z = ++x;
  numbers.push_back(z);
  double target[3] = {12, 13, 14};
  ASSERT_EQ(numbers.size(), 3);
  ASSERT_TRUE(equal(begin(numbers), end(numbers), begin(target)));
}

TEST(GarlicSequence, Growth) {
  sequence<double> numbers(2);
  static const auto total = 1000000;
  double target[total];
  for (auto i = 0; i < 1000000; ++i) {
    target[i] = i;
    numbers.push_back(i);
  }
  ASSERT_EQ(numbers.size(), total);
  ASSERT_TRUE(equal(begin(numbers), end(numbers), begin(target)));
}

TEST(GarlicSequence, BackInserter) {
  struct Data {
    long double d;
    string s;
    Data* another = nullptr;
  };
  sequence<Data> items;
  vector<Data> target {};
  for (auto i = 0; i< 100; ++i) target.push_back(Data{static_cast<long double>(i), std::to_string(i)});
  copy(begin(items), end(items), back_inserter(items));
}

TEST(GarlicSequence, MoveOperators) {
  sequence<int> numbers(12);
  numbers.push_back(1);
  numbers.push_back(2);
  auto another_one = std::move(numbers);  // move ctor.
  ASSERT_EQ(numbers.capacity(), 0);
  ASSERT_EQ(another_one.capacity(), 12);
  ASSERT_EQ(another_one.size(), 2);

  sequence<int> new_one(2);
  new_one.push_back(1);
  another_one = std::move(new_one);
  ASSERT_EQ(new_one.capacity(), 0);
  ASSERT_EQ(another_one.capacity(), 2);
  ASSERT_EQ(another_one.size(), 1);
}


TEST(Random, R) {
  Model<CloveView> c("Peyman");
}
