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


template<typename T>
void assert_no_list(T& value) {
  //ASSERT_FALSE(value.is_list());
  //ASSERT_THROW(value.get_list().begin(), garlic::TypeError);
  //ASSERT_THROW(value.get_list().end(), garlic::TypeError);
  //ASSERT_THROW(value.append(T::none), garlic::TypeError);
  //ASSERT_THROW(value.remove(0), garlic::TypeError);
  //ASSERT_THROW(value[0], garlic::TypeError);
}


template<typename T>
void assert_no_object(T& value) {
  //ASSERT_FALSE(value.is_object());
  //ASSERT_THROW(value.set("Peyman", T::none), garlic::TypeError);
  //ASSERT_THROW(value.get("Peyman"), garlic::TypeError);
  //ASSERT_THROW(value.get_object().begin(), garlic::TypeError);
  //ASSERT_THROW(value.get_object().end(), garlic::TypeError);
}


template<garlic::ReadableLayer LayerType>
void test_readonly_string_value(const LayerType& value, const std::string& text) {
  ASSERT_TRUE(value.is_string());
  //assert_no_object(value);
  //assert_no_list(value);

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

/**
 * value must be an object with ability creating child elements.
 * testing that a string value would return cstr, std string and std string view.
 */
template<typename T>
void test_writable_object_string_capabilities(T& value) {
  std::string texts[] = {"Peyman", "Some other name just to make sure it can deal with it."};

  std::for_each(begin(texts), end(texts), [&](auto text) {
      value.set("name", text.c_str());
      check_value("name", text);

      std::string key = "name_str_string";
      value.set(key, text);
      auto str = value.get(key);
      test_readonly_string_value(str, text);
    }
  );
}
