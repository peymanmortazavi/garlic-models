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
  auto it = value.begin_list();
  for (const auto& item : value.get_list()) {
    ASSERT_TRUE(garlic::cmp_layers(*it, item));
    it++;
  }
  ASSERT_EQ(it, value.end_list());
}
