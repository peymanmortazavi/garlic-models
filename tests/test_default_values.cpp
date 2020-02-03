/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include <iostream>
#include <map>
#include <vector>


void assert_no_list(garlic::value& val) {
  ASSERT_THROW(val.get_list().begin(), garlic::TypeError);
  ASSERT_THROW(val.get_list().end(), garlic::TypeError);
  ASSERT_THROW(val.append(garlic::value::none), garlic::TypeError);
  ASSERT_THROW(val.remove(0), garlic::TypeError);
  ASSERT_THROW(val[0], garlic::TypeError);
}


void assert_no_object(garlic::value& val) {
  ASSERT_THROW(val.set("Peyman", garlic::value::none), garlic::TypeError);
  ASSERT_THROW(val.get("Peyman"), garlic::TypeError);
  ASSERT_THROW(val.get_object().begin(), garlic::TypeError);
  ASSERT_THROW(val.get_object().end(), garlic::TypeError);
}


/**
 * Tests out a generic garlic list container.
 * @param list_container it assumes this container has exactly three elements string "a", null and true.
 */
template<typename T>
void test_list_iterations(T& list_container) {
  #define TRY_ITEM(element, index) switch (index) {\
    case 0:\
      ASSERT_EQ(element.is_string(), true);\
      ASSERT_EQ(element.get_string(), "a");\
      break;\
    case 1:\
      ASSERT_EQ(element.is_null(), true);\
      break;\
    case 2:\
      ASSERT_EQ(element.is_bool(), true);\
      ASSERT_EQ(element.get_bool(), true);\
      break;\
  }

  // Reference Range Iteration
  auto index = 0;
  for(auto& element : list_container.get_list()) {
    TRY_ITEM(element, index);
    index++;
  }

  // Const Range Iteration
  index = 0;
  const T& list_container_ref = list_container;
  for(const auto& element : list_container_ref.get_list()) {
    TRY_ITEM(element, index);
    index++;
  }
}


TEST(DefaultValue, StringValue) {
  auto value = garlic::string{"valid string"};
  ASSERT_EQ(value.get_string(), "valid string");
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);
  ASSERT_THROW(value.get_double(), garlic::TypeError);

  // convenience check.
  garlic::string value2;
  value2 = "test";  // assignment to const char* is easy.
  const char* value3 = value2;  // getting const char* out of it is easy.
  ASSERT_EQ(strcmp(value3, "test"), 0);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, IntegerValue) {
  auto value = garlic::integer{25};
  ASSERT_EQ(value.get_int(), 25);
  ASSERT_THROW(value.get_string(), garlic::TypeError);
  ASSERT_THROW(value.get_double(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);

  // convenience check.
  garlic::integer value2;
  value2 = 14;  // assignment to it is easy.
  int value3 = value2;  // getting int out of it is easy.
  ASSERT_EQ(value3, 14);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, DoubleValue) {
  auto value = garlic::float64{10.23};
  ASSERT_EQ(value.get_double(), 10.23);
  ASSERT_THROW(value.get_string(), garlic::TypeError);
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);

  // convenience check.
  garlic::float64 value2;
  value2 = 12.43;  // assignment to it is easy.
  double value3 = value2;  // getting double out of it is easy.
  ASSERT_EQ(value3, 12.43);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, BooleanValue) {
  auto value = garlic::boolean{false};
  ASSERT_EQ(value.get_bool(), false);
  ASSERT_THROW(value.get_double(), garlic::TypeError);
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_string(), garlic::TypeError);

  garlic::boolean value2;
  value2 = true;
  bool value3 = value2;
  ASSERT_EQ(value3, true);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, ObjectValue) {
  garlic::object value;
  value.set("name", garlic::string{"test"});
  value.set("age", garlic::integer{25});
  ASSERT_EQ(value.get("name")->get_string(), "test");
  ASSERT_EQ(value.get("age")->get_int(), 25);

  garlic::object value2;
  value2.set("title", garlic::string{"c++ programmer"});
  value.set("job", value2);
  ASSERT_EQ(value.get("job")->get("title")->get_string(), "c++ programmer");
}


TEST(DefaultValue, ListValue) {
  garlic::list value;
  value.append(garlic::string{"peyman"});
  value.append(garlic::value::none);
  value.append(garlic::boolean{true});
  ASSERT_EQ(value[0].get_string(), "peyman");
  ASSERT_EQ(value[1].is_null(), true);
  ASSERT_EQ(value[1].is_string(), false);
  ASSERT_EQ(value[2].is_bool(), true);
  ASSERT_EQ(value[2].get_bool(), true);
  ASSERT_EQ(value[2].is_string(), false);
}

TEST(DefaultValue, ListValueIterators) {
  garlic::list value;
  value.append(garlic::string{"a"});
  value.append(garlic::value::none);
  value.append(garlic::boolean{true});
  test_list_iterations(value);
}
