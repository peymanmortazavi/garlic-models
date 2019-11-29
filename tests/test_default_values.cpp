/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>


void assert_no_list(garlic::value& val) {
  ASSERT_THROW(val.get_list().begin(), garlic::TypeError);
  ASSERT_THROW(val.get_list().end(), garlic::TypeError);
  ASSERT_THROW(val.append(garlic::NoResult), garlic::TypeError);
  ASSERT_THROW(val.remove(0), garlic::TypeError);
  ASSERT_THROW(val[0], garlic::TypeError);
}


void assert_no_object(garlic::value& val) {
  ASSERT_THROW(val.set("Peyman", garlic::NoResult), garlic::TypeError);
  ASSERT_THROW(val.get("Peyman"), garlic::TypeError);
  ASSERT_THROW(val.get_object().begin(), garlic::TypeError);
  ASSERT_THROW(val.get_object().end(), garlic::TypeError);
}


TEST(DefaultValue, StringValue) {
  auto value = garlic::string{"valid string"};
  ASSERT_EQ(value.get_string(), "valid string");
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);
  ASSERT_THROW(value.get_double(), garlic::TypeError);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, IntegerValue) {
  auto value = garlic::integer{25};
  ASSERT_EQ(value.get_int(), 25);
  ASSERT_THROW(value.get_string(), garlic::TypeError);
  ASSERT_THROW(value.get_double(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, DoubleValue) {
  auto value = garlic::float64{10.23};
  ASSERT_EQ(value.get_double(), 10.23);
  ASSERT_THROW(value.get_string(), garlic::TypeError);
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_bool(), garlic::TypeError);

  assert_no_list(value);
  assert_no_object(value);
}


TEST(DefaultValue, BooleanValue) {
  auto value = garlic::boolean{false};
  ASSERT_EQ(value.get_bool(), false);
  ASSERT_THROW(value.get_double(), garlic::TypeError);
  ASSERT_THROW(value.get_int(), garlic::TypeError);
  ASSERT_THROW(value.get_string(), garlic::TypeError);

  assert_no_list(value);
  assert_no_object(value);
}
