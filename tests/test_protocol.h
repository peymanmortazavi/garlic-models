/******************************************************************************
* File:             test_protocol.h
*
* Author:           Peyman Mortazavi  
* Created:          03/29/20 
* Description:      Test methods for assuring an object viewer supports all the required functions.
*****************************************************************************/

#include "garlic/json.h"
#include "garlic/layer.h"
#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <map>
#include <vector>


template<garlic::ReadableLayer LayerType>
void test_readonly_string_value(const LayerType& value, const std::string& text) {
  ASSERT_TRUE(value.is_string());

  std::string std_string = value.get_string();
  const char* c_str = value.get_cstr();
  std::string_view std_string_view = value.get_string_view();
  ASSERT_STREQ(std_string.c_str(), text.c_str());
  ASSERT_STREQ(c_str, text.c_str());
  ASSERT_TRUE(std_string_view == text);
}

template<garlic::ReadableLayer LayerType>
void test_readonly_int_value(const LayerType& value, int expectation) {
  ASSERT_TRUE(value.is_int());
  ASSERT_EQ(value.get_int(), expectation);
}

template<garlic::ReadableLayer LayerType>
void test_readonly_double_value(const LayerType& value, double expectation) {
  ASSERT_TRUE(value.is_double());
  ASSERT_EQ(value.get_double(), expectation);
}

template<garlic::ReadableLayer LayerType>
void test_readonly_bool_value(const LayerType& value, bool expectation) {
  ASSERT_TRUE(value.is_bool());
  ASSERT_EQ(value.get_bool(), expectation);
}

template<garlic::ReadableLayer LayerType>
void test_readonly_null_value(const LayerType& value) {
  ASSERT_TRUE(value.is_null());
}

template<garlic::ReadableLayer LayerType>
void test_readonly_list_range(const LayerType& value) {
  ASSERT_TRUE(value.is_list());
  auto it = value.begin_list();
  for (const auto& item : value.get_list()) {
    ASSERT_TRUE(garlic::cmp_layers(*it, item));
    std::advance(it, 1);
  }
  ASSERT_EQ(it, value.end_list());
}

template<garlic::ReadableLayer LayerType>
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

template<garlic::GarlicLayer LayerType>
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

template<garlic::GarlicLayer LayerType>
void test_full_int_value(LayerType& value) {
  value.set_int(170);
  test_readonly_int_value(value, 170);
}

template<garlic::GarlicLayer LayerType>
void test_full_double_value(LayerType& value) {
  value.set_double(170.189);
  test_readonly_double_value(value, 170.189);
}

template<garlic::GarlicLayer LayerType>
void test_full_bool_value(LayerType& value) {
  value.set_bool(true);
  test_readonly_bool_value(value, true);

  value.set_bool(false);
  test_readonly_bool_value(value, false);
}

template<garlic::GarlicLayer LayerType>
void test_full_null_value(LayerType& value) {
  value.set_null();
  test_readonly_null_value(value);
}

template<garlic::GarlicLayer LayerType>
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

template<garlic::GarlicLayer LayerType>
void test_full_object_value(LayerType& value) {
  value.set_object();
  value.add_member("null");
  value.add_member("string", "string");
  value.add_member("double", 1.1);
  value.add_member("int", 25);
  value.add_member("bool", false);

  test_readonly_object_range(value);
  auto it = value.begin_member();
  ASSERT_STREQ((*it).key.get_cstr(), "null");
  ASSERT_TRUE((*it).value.is_null());
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "string");
  ASSERT_STREQ((*it).value.get_cstr(), "string");
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "double");
  ASSERT_EQ((*it).value.get_double(), 1.1);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "int");
  ASSERT_EQ((*it).value.get_int(), 25);
  it++;
  ASSERT_STREQ((*it).key.get_cstr(), "bool");
  ASSERT_EQ((*it).value.get_bool(), false);
  it++;
  ASSERT_EQ(it, value.end_member());
}

template<garlic::GarlicLayer LayerType>
void test_full_layer(LayerType&& value) {
  test_full_string_value(value);
  test_full_double_value(value);
  test_full_int_value(value);
  test_full_bool_value(value);
  test_full_null_value(value);
  test_full_list_value(value);
  test_full_object_value(value);
}
