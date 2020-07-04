/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include <map>
#include <vector>


template<typename V>
void assert_no_list(V& val) {
  //ASSERT_THROW(val.get_list().begin(), garlic::TypeError);
  //ASSERT_THROW(val.get_list().end(), garlic::TypeError);
  //ASSERT_THROW(val.append(V::none), garlic::TypeError);
  //ASSERT_THROW(val.remove(0), garlic::TypeError);
  //ASSERT_THROW(val[0], garlic::TypeError);
}


template<typename V>
void assert_no_object(V& val) {
  //ASSERT_THROW(val.set("Peyman", V::none), garlic::TypeError);
  //ASSERT_THROW(val.get("Peyman"), garlic::TypeError);
  //ASSERT_THROW(val.get_object().begin(), garlic::TypeError);
  //ASSERT_THROW(val.get_object().end(), garlic::TypeError);
}


/**
 * Tests out a generic garlic list container.
 * @param list_container it assumes this container has exactly three elements string "a", null and true.
 */
template<typename T>
void test_list_iterations(T& list_container) {
  #define TRY_LIST_ITEM(element, index) switch (index) {\
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
    TRY_LIST_ITEM(element, index);
    index++;
  }

  // Const Range Iteration
  index = 0;
  const T& list_container_ref = list_container;
  for(const auto& element : list_container_ref.get_list()) {
    TRY_LIST_ITEM(element, index);
    index++;
  }
}


/**
 * Tests out a generic garlic object container.
 * @param object_container it assumes this container has {"name": "peyman", "age": 27, "photo": null}
 */
template<typename T>
void test_object_iterations(T& object_container) {
  #define TRY_OBJECT_ITEM(element, index) switch (index) {\
    case 0:\
      ASSERT_EQ(element.first, "age");\
      ASSERT_EQ(element.second.get_int(), 27);\
      break;\
    case 1:\
      ASSERT_EQ(element.first, "name");\
      ASSERT_EQ(element.second.get_string(), "peyman");\
      break;\
    case 2:\
      ASSERT_EQ(element.first, "photo");\
      ASSERT_EQ(element.second.is_null(), true);\
      break;\
  }
  // get the reference.
  auto index = 0;
  for(auto element : object_container.get_object()) {
    TRY_OBJECT_ITEM(element, index);
    index++;
  }

  // const compatibility.
  index = 0;
  const T& const_object_container = object_container;
  for(const auto& element : const_object_container.get_object()) {
    TRY_OBJECT_ITEM(element, index);
    index++;
  }
}


TEST(DefaultValue, StringValue) {
  //auto value = garlic::string{"valid string"};
  //ASSERT_EQ(value.get_string(), "valid string");
  //ASSERT_THROW(value.get_int(), garlic::TypeError);
  //ASSERT_THROW(value.get_bool(), garlic::TypeError);
  //ASSERT_THROW(value.get_double(), garlic::TypeError);

  //// convenience check.
  //garlic::string value2;
  //value2 = "test";  // assignment to const char* is easy.
  //const char* value3 = value2;  // getting const char* out of it is easy.
  //ASSERT_EQ(strcmp(value3, "test"), 0);

  //assert_no_list(value);
  //assert_no_object(value);

  //auto s_value = garlic::s_string{"valid string"};
  //ASSERT_EQ(s_value.get_string(), "valid string");
  //ASSERT_THROW(s_value.get_int(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_bool(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_double(), garlic::TypeError);

  //// convenience check.
  //garlic::s_string s_value2;
  //s_value2 = "test";  // assignment to const char* is easy.
  //const char* s_value3 = s_value2;  // getting const char* out of it is easy.
  //ASSERT_EQ(strcmp(s_value3, "test"), 0);

  //assert_no_list(s_value);
  //assert_no_object(s_value);
}


TEST(DefaultValue, IntegerValue) {
  //auto value = garlic::integer{25};
  //ASSERT_EQ(value.get_int(), 25);
  //ASSERT_THROW(value.get_string(), garlic::TypeError);
  //ASSERT_THROW(value.get_double(), garlic::TypeError);
  //ASSERT_THROW(value.get_bool(), garlic::TypeError);

  //// convenience check.
  //garlic::integer value2;
  //value2 = 14;  // assignment to it is easy.
  //int value3 = value2;  // getting int out of it is easy.
  //ASSERT_EQ(value3, 14);

  //assert_no_list(value);
  //assert_no_object(value);

  //auto s_value = garlic::s_integer{25};
  //ASSERT_EQ(s_value.get_int(), 25);
  //ASSERT_THROW(s_value.get_string(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_double(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_bool(), garlic::TypeError);

  //// convenience check.
  //garlic::s_integer s_value2;
  //s_value2 = 14;  // assignment to it is easy.
  //int s_value3 = s_value2;  // getting int out of it is easy.
  //ASSERT_EQ(s_value3, 14);

  //assert_no_list(s_value);
  //assert_no_object(s_value);
}


TEST(DefaultValue, DoubleValue) {
  //auto value = garlic::float64{10.23};
  //ASSERT_EQ(value.get_double(), 10.23);
  //ASSERT_THROW(value.get_string(), garlic::TypeError);
  //ASSERT_THROW(value.get_int(), garlic::TypeError);
  //ASSERT_THROW(value.get_bool(), garlic::TypeError);

  //// convenience check.
  //garlic::float64 value2;
  //value2 = 12.43;  // assignment to it is easy.
  //double value3 = value2;  // getting double out of it is easy.
  //ASSERT_EQ(value3, 12.43);

  //assert_no_list(value);
  //assert_no_object(value);

  //auto s_value = garlic::s_float64{10.23};
  //ASSERT_EQ(s_value.get_double(), 10.23);
  //ASSERT_THROW(s_value.get_string(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_int(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_bool(), garlic::TypeError);

  //// convenience check.
  //garlic::s_float64 s_value2;
  //s_value2 = 12.43;  // assignment to it is easy.
  //double s_value3 = s_value2;  // getting double out of it is easy.
  //ASSERT_EQ(s_value3, 12.43);

  //assert_no_list(s_value);
  //assert_no_object(s_value);
}


TEST(DefaultValue, BooleanValue) {
  //auto value = garlic::boolean{false};
  //ASSERT_EQ(value.get_bool(), false);
  //ASSERT_THROW(value.get_double(), garlic::TypeError);
  //ASSERT_THROW(value.get_int(), garlic::TypeError);
  //ASSERT_THROW(value.get_string(), garlic::TypeError);

  //garlic::boolean value2;
  //value2 = true;
  //bool value3 = value2;
  //ASSERT_EQ(value3, true);

  //assert_no_list(value);
  //assert_no_object(value);

  //auto s_value = garlic::s_boolean{false};
  //ASSERT_EQ(s_value.get_bool(), false);
  //ASSERT_THROW(s_value.get_double(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_int(), garlic::TypeError);
  //ASSERT_THROW(s_value.get_string(), garlic::TypeError);

  //garlic::s_boolean s_value2;
  //s_value2 = true;
  //bool s_value3 = s_value2;
  //ASSERT_EQ(s_value3, true);

  //assert_no_list(s_value);
  //assert_no_object(s_value);
}


TEST(DefaultValue, ObjectValue) {
  //garlic::object value;
  //value.set("name", garlic::string{"test"});
  //value.set("age", garlic::integer{25});
  //ASSERT_EQ(value.get("name")->get_string(), "test");
  //ASSERT_EQ(value.get("age")->get_int(), 25);

  //garlic::object value2;
  //value2.set("title", garlic::string{"c++ programmer"});
  //value.set("job", value2);
  //ASSERT_EQ(value.get("job")->get("title")->get_string(), "c++ programmer");

  //garlic::s_object s_value;
  //s_value.set("name", garlic::s_string{"test"});
  //s_value.set("age", garlic::s_integer{25});
  //ASSERT_EQ(s_value.get("name")->get_string(), "test");
  //ASSERT_EQ(s_value.get("age")->get_int(), 25);

  //garlic::s_object s_value2;
  //s_value2.set("title", garlic::s_string{"c++ programmer"});
  //s_value.set("job", s_value2);
  //ASSERT_EQ(s_value.get("job")->get("title")->get_string(), "c++ programmer");
}

TEST(DefaultValue, ObjectValueIteratos) {
  //garlic::object value;
  //value.set("name", garlic::string{"peyman"});
  //value.set("age", garlic::integer{27});
  //value.set("photo", garlic::object::none);
  //test_object_iterations(value);

  //garlic::s_object s_value;
  //s_value.set("name", garlic::s_string{"peyman"});
  //s_value.set("age", garlic::s_integer{27});
  //s_value.set("photo", garlic::s_object::none);
  //test_object_iterations(s_value);
}


TEST(DefaultValue, ListValue) {
  //garlic::list value;
  //value.append(garlic::string{"peyman"});
  //value.append(garlic::value::none);
  //value.append(garlic::boolean{true});
  //ASSERT_EQ(value[0].get_string(), "peyman");
  //ASSERT_EQ(value[1].is_null(), true);
  //ASSERT_EQ(value[1].is_string(), false);
  //ASSERT_EQ(value[2].is_bool(), true);
  //ASSERT_EQ(value[2].get_bool(), true);
  //ASSERT_EQ(value[2].is_string(), false);

  //garlic::s_list s_value;
  //s_value.append(garlic::s_string{"peyman"});
  //s_value.append(garlic::s_value::none);
  //s_value.append(garlic::s_boolean{true});
  //ASSERT_EQ(value[0].get_string(), "peyman");
  //ASSERT_EQ(value[1].is_null(), true);
  //ASSERT_EQ(value[1].is_string(), false);
  //ASSERT_EQ(value[2].is_bool(), true);
  //ASSERT_EQ(value[2].get_bool(), true);
  //ASSERT_EQ(value[2].is_string(), false);
}

TEST(DefaultValue, ListValueIterators) {
  //garlic::list value;
  //value.append(garlic::string{"a"});
  //value.append(garlic::value::none);
  //value.append(garlic::boolean{true});
  //test_list_iterations(value);

  //garlic::s_list s_value;
  //s_value.append(garlic::s_string{"a"});
  //s_value.append(garlic::s_value::none);
  //s_value.append(garlic::s_boolean{true});
  //test_list_iterations(s_value);
}
