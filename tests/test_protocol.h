/******************************************************************************
* File:             test_protocol.h
*
* Author:           Peyman Mortazavi  
* Created:          03/29/20 
* Description:      Test methods for assuring an object viewer supports all the required functions.
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <map>
#include <vector>


template<garlic::ViewLayer LayerType>
void test_readonly_string_value(const LayerType& value, const std::string& text) {
  ASSERT_TRUE(value.is_string());

  std::string std_string = value.get_string();
  const char* c_str = value.get_cstr();
  std::string_view std_string_view = value.get_string_view();
  ASSERT_STREQ(std_string.c_str(), text.c_str());
  ASSERT_STREQ(c_str, text.c_str());
  ASSERT_TRUE(std_string_view == text);
}

template<garlic::ViewLayer LayerType>
void test_readonly_int_value(const LayerType& value, int expectation) {
  ASSERT_TRUE(value.is_int());
  ASSERT_EQ(value.get_int(), expectation);
}

template<garlic::ViewLayer LayerType>
void test_readonly_double_value(const LayerType& value, double expectation) {
  ASSERT_TRUE(value.is_double());
  ASSERT_EQ(value.get_double(), expectation);
}

template<garlic::ViewLayer LayerType>
void test_readonly_bool_value(const LayerType& value, bool expectation) {
  ASSERT_TRUE(value.is_bool());
  ASSERT_EQ(value.get_bool(), expectation);
}

template<garlic::ViewLayer LayerType>
void test_readonly_null_value(const LayerType& value) {
  ASSERT_TRUE(value.is_null());
}

template<garlic::ViewLayer LayerType>
void test_readonly_list_range(const LayerType& value) {
  ASSERT_TRUE(value.is_list());
  auto it = value.begin_list();
  for (const auto& item : value.get_list()) {
    ASSERT_TRUE(garlic::cmp_layers(*it, item));
    std::advance(it, 1);
  }
  ASSERT_EQ(it, value.end_list());
}

template<garlic::ViewLayer LayerType>
void test_readonly_object_range(const LayerType& value) {
  ASSERT_TRUE(value.is_object());
  auto it = value.begin_member();
  for(const auto& item : value.get_object()) {
    ASSERT_TRUE(garlic::cmp_layers(item.key, (*it).key));
    ASSERT_TRUE(garlic::cmp_layers(item.value, (*it).value));
    std::advance(it, 1);
  }
}

// Tests for writing.

template<garlic::RefLayer LayerType>
void test_full_string_value(LayerType& value) {
  std::string origin = "This is a very smoky test just to show if we have some string support.";
  std::string_view view = std::string_view{origin};
  value.set_string(origin);
  test_readonly_string_value(value, origin);

  value.set_string(view);
  test_readonly_string_value(value, origin);

  value.set_string(origin.c_str());
  test_readonly_string_value(value, origin);
}

template<garlic::RefLayer LayerType>
void test_full_int_value(LayerType& value) {
  value.set_int(170);
  test_readonly_int_value(value, 170);
}

template<garlic::RefLayer LayerType>
void test_full_double_value(LayerType& value) {
  value.set_double(170.189);
  test_readonly_double_value(value, 170.189);
}

template<garlic::RefLayer LayerType>
void test_full_bool_value(LayerType& value) {
  value.set_bool(true);
  test_readonly_bool_value(value, true);

  value.set_bool(false);
  test_readonly_bool_value(value, false);
}

template<garlic::RefLayer LayerType>
void test_full_null_value(LayerType& value) {
  value.set_null();
  test_readonly_null_value(value);
}

template<garlic::RefLayer LayerType>
void test_full_list_value(LayerType& value) {
  value.set_list();
  value.push_back("string");
  value.push_back(25);
  value.push_back(1.4);
  value.push_back(false);
  value.push_back();
  
  test_readonly_list_range(value);
  auto it = value.begin_list();
  ASSERT_STREQ((*it).get_cstr(), "string");
  it++;
  ASSERT_EQ((*it).get_int(), 25);
  it++;
  ASSERT_EQ((*it).get_double(), 1.4);
  it++;
  ASSERT_EQ((*it).get_bool(), false);
  it++;
  ASSERT_TRUE((*it).is_null());
  it++;
  ASSERT_EQ(it, value.end_list());

  it = value.begin_list();
  value.erase(std::next(it, 1), std::next(it, 3));
  value.erase(std::next(value.begin_list(), 1));
  value.pop_back();
  it = value.begin_list();
  ASSERT_EQ((*it).get_string(), "string");
  it++;
  ASSERT_EQ(it, value.end_list());

  value.clear();
  ASSERT_EQ(value.begin_list(), value.end_list());
}

template<garlic::RefLayer LayerType>
void test_full_object_value(LayerType& value) {
  value.set_object();
  value.add_member("null");
  value.add_member("string", "string");
  value.add_member("double", 1.1);
  value.add_member("int", 25);
  value.add_member("bool", false);

  auto assert_member = [](auto& value, const auto& key, auto it) {
    ASSERT_EQ(value.find_member(key), it);
    ASSERT_EQ(value.find_member(std::string_view(key)), it);
  };

  test_readonly_object_range(value);
  auto it = value.begin_member();
  ASSERT_STREQ((*it).key.get_cstr(), "null");
  ASSERT_TRUE((*it).value.is_null());
  assert_member(value, "null", it);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "string");
  ASSERT_STREQ((*it).value.get_cstr(), "string");
  assert_member(value, "string", it);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "double");
  ASSERT_EQ((*it).value.get_double(), 1.1);
  assert_member(value, "double", it);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "int");
  ASSERT_EQ((*it).value.get_int(), 25);
  assert_member(value, "int", it);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "bool");
  ASSERT_EQ((*it).value.get_bool(), false);
  assert_member(value, "bool", it);
  // test search by string_view
  ASSERT_EQ(value.find_member(std::string_view{"bool_extraextra", 4}), it);
  it++;
  ASSERT_EQ(it, value.end_member());
  assert_member(value, "missing key", value.end_member());

  for(auto item : value.get_object()) {
    item.key.set_string("v2." + item.key.get_string());
  }
  it = std::find_if(
      value.begin_member(),
      value.end_member(),
      [](auto item) { return item.key.get_string() == "v2.null"; }
  );
  ASSERT_TRUE((*it).value.is_null());

  const auto& readonly = value;
  auto value_view = value.get_view();
  it = value.find_member("v2.null");
  ASSERT_NE(it, value.end_member());
  (*it).value.set_double(12);
  auto const_it = value_view.find_member("v2.null");
  ASSERT_NE(const_it, value_view.end_member());
  ASSERT_EQ((*const_it).value.get_double(), 12);
  const_it = value_view.find_member(std::string_view{"v2.null_extra", 7});
  ASSERT_NE(const_it, value_view.end_member());
  ASSERT_EQ((*const_it).value.get_double(), 12);
}

template<garlic::RefLayer LayerType>
void test_full_layer(LayerType&& value) {
  test_full_string_value(value);
  test_full_double_value(value);
  test_full_int_value(value);
  test_full_bool_value(value);
  test_full_null_value(value);
  test_full_list_value(value);
  test_full_object_value(value);
}
