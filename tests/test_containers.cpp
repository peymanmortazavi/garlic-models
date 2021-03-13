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

  text d("abcd", 5);
  ASSERT_STREQ(d.data(), "abcd");
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

  ASSERT_EQ(txt, std::string("1234"));
}

TEST(GarlicText, StdStringViewConstructor) {
  std::string_view a = "Some String";
  text b = a;

  ASSERT_EQ(b, std::string("Some String"));
}

TEST(GarlicText, Copy) {
  text a = "abcd";
  text b ("abcd", text_type::copy);

  text c = a;
  text d = b;

  text copy1 = a.clone();
  text copy2 = b.clone();

  ASSERT_EQ(c.data(), a.data());  // same pointers.
  ASSERT_EQ(d.data(), b.data());  // same pointers.
  ASSERT_NE(copy1.data(), a.data());  // different pointers.
  ASSERT_NE(copy2.data(), b.data());  // different pointers.
}

TEST(GarlicText, StaticNoText) {
  ASSERT_EQ(text::no_text().data(), text::no_text().data());
}

TEST(GarlicText, ConstExpr) {
  constexpr text str = "Some String";
  constexpr auto size = str.size();

  constexpr text x = str;
  constexpr std::string_view y = "Some String";
  constexpr bool result = x == y;
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

TEST(GarlicSequence, PushFront) {
  sequence<double> one(4);
  one.push_back(3);
  one.push_back(4);

  sequence<double> two(2);
  two.push_back(1);
  two.push_back(2);

  one.push_front(two.begin(), two.end());

  {
    double expectation[4] = {1, 2, 3, 4};
    ASSERT_TRUE(std::equal(one.begin(), one.end(), expectation, expectation + 4));
  }

  sequence<double> three(4);
  for (auto i = 0; i < 4; ++i) three.push_back(i);

  one.push_front(three.begin(), three.end());
  {
    double expectation[8] = {0, 1, 2, 3, 1, 2, 3, 4};
    ASSERT_TRUE(std::equal(one.begin(), one.end(), expectation, expectation + 8));
  }
}

TEST(GarlicSequence, ListInitializer) {
  sequence<int> ints{1, 2, 3, 5, 8};
  ASSERT_EQ(ints.size(), 5);
  ASSERT_EQ(ints.capacity(), 5);
  ASSERT_EQ(ints[0], 1);
  ASSERT_EQ(ints[1], 2);
  ASSERT_EQ(ints[2], 3);
  ASSERT_EQ(ints[3], 5);
  ASSERT_EQ(ints[4], 8);
}
