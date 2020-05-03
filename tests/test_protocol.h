/******************************************************************************
* File:             test_protocol.h
*
* Author:           Peyman Mortazavi  
* Created:          03/29/20 
* Description:      Test methods for assuring an object viewer supports all the required functions.
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include <map>
#include <vector>


template<typename T>
void assert_no_list(T& value) {
  ASSERT_FALSE(value.is_list());
  ASSERT_THROW(value.get_list().begin(), garlic::TypeError);
  ASSERT_THROW(value.get_list().end(), garlic::TypeError);
  ASSERT_THROW(value.append(T::none), garlic::TypeError);
  ASSERT_THROW(value.remove(0), garlic::TypeError);
  ASSERT_THROW(value[0], garlic::TypeError);
}

template<typename T>
void assert_no_object(T& value) {
  ASSERT_FALSE(value.is_list());
  ASSERT_THROW(value.set("Peyman", T::none), garlic::TypeError);
  ASSERT_THROW(value.get("Peyman"), garlic::TypeError);
  ASSERT_THROW(value.get_object().begin(), garlic::TypeError);
  ASSERT_THROW(value.get_object().end(), garlic::TypeError);
}


/**
 * value must be an object with ability creating child elements.
 */
template<typename T>
void test_object_string_capabilities(T& value) {
  std::string texts[] = {"Peyman", "Some other name just to make sure it can deal with it."};

  auto check_value = [&](const auto& key, const auto& text) {
    auto str = value.get(key);
    ASSERT_TRUE(str.is_string());
    assert_no_object(str);
    assert_no_list(str);

    std::string std_string = str.get_string();
    const char* c_str = str.get_cstr();
    std::string_view std_string_view = str.get_string_view();
    ASSERT_STREQ(std_string, text);
    ASSERT_STREQ(c_str, text);
    ASSERT_STREQ(std_string_view, text);
  };

  std::for_each(begin(texts), end(texts), [&](auto text) {
      value.set("name", text.c_str());
      check_value("name", text);

      std::string key = "name_str_string";
      value.set(key, text);
      check_value(key, text);
    }
  );
}
