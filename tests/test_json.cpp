/******************************************************************************
* File:             test_json.cpp
*
* Author:           Peyman Mortazavi
* Created:          02/03/20
* Description:      Testing the JSON support for garlic.
*****************************************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <garlic/garlic.h>

#include "garlic/json.h"
#include "test_protocol.h"

using namespace std;

using namespace garlic;
using namespace rapidjson;


#define TRY_LIST_ITEM(element, index) switch (index) {\
  case 0:\
    ASSERT_EQ(element.is_double(), true);\
    ASSERT_EQ(element.get_double(), 1.1);\
    break;\
  case 1:\
    ASSERT_EQ(element.is_string(), true);\
    ASSERT_EQ(element.get_string(), "Test");\
    break;\
  case 2:\
    ASSERT_EQ(element.is_bool(), true);\
    ASSERT_EQ(element.get_bool(), true);\
    break;\
  case 3:\
    ASSERT_EQ(element.is_null(), true);\
    break;\
}


Document get_test_document() {
  ifstream ifs("data/test.json");
  IStreamWrapper isw(ifs);
  Document d;
  d.ParseStream(isw);
  return d;
}


TEST(DocumentTests, TypeMatching) {
  //auto wrapper = rapidjson_wrapper(get_test_document());
  //ASSERT_TRUE(wrapper.is_object());
  //ASSERT_TRUE(wrapper.get("name")->is_string());

  //rapidjson_wrapper wrapper{d["values"]};
  //auto index = 0;
  //for (auto it = wrapper.begin_list(); it != wrapper.end_list(); ++it) {
  //  auto element = *it;
  //  TRY_LIST_ITEM(element, index);
  //  index++;
  //}

  //index = 0;
  //for(const auto& it : wrapper.get_list()) {
  //  TRY_LIST_ITEM(it, index);
  //  index++;
  //}
}


TEST(DocumentTests, ListIterator) {

}


TEST(DocumentTests, ProtocolTest) {
  Document doc = get_test_document();

  // test happy path values for keys that do actually exist.
  test_readonly_string_value(rapidjson_readonly_layer(doc["name"]), "Peyman");
  test_readonly_int_value(rapidjson_readonly_layer(doc["age"]), 28);
  test_readonly_double_value(rapidjson_readonly_layer(doc["h"]), 12.123);
  test_readonly_bool_value(rapidjson_readonly_layer(doc["ready"]), true);
  test_readonly_null_value(rapidjson_readonly_layer(doc["career"]));

  // test the list values.
  auto list_value = rapidjson_readonly_layer(doc["values"]);
  ASSERT_TRUE(list_value.is_list());
  auto list_it = list_value.begin_list();
  test_readonly_double_value(*list_it, 1.1); list_it++;
  test_readonly_int_value(*list_it, 25); list_it++;
  test_readonly_string_value(*list_it, "Test"); list_it++;
  test_readonly_bool_value(*list_it, true); list_it++;
  test_readonly_null_value(*list_it); list_it++;
  ASSERT_EQ(list_it, list_value.end_list());

  // test the object values.
  auto object_value = rapidjson_readonly_layer(doc["objects"]);
  auto object_it = object_value.begin_member();
  auto first_item = *object_it; object_it++;
  test_readonly_string_value(first_item.key, "ready");
  test_readonly_bool_value(first_item.value, false);

  auto second_item = *object_it; object_it++;
  test_readonly_string_value(second_item.key, "composite");
  ASSERT_TRUE(second_item.value.is_object());
  auto second_item_it = second_item.value.begin_member();
  test_readonly_string_value((*second_item_it).key, "greeting");
  test_readonly_string_value((*second_item_it).value, "heya");
  second_item_it++;
  ASSERT_EQ(second_item_it, second_item.value.end_member());

  auto third_item = *object_it; object_it++;
  test_readonly_string_value(third_item.key, "list");
  ASSERT_TRUE(third_item.value.is_list());
  auto third_item_it = third_item.value.begin_list();
  test_readonly_int_value(*third_item_it, 38); third_item_it++;
  test_readonly_string_value(*third_item_it, "string"); third_item_it++;
  ASSERT_EQ(third_item_it, third_item.value.end_list());

  ASSERT_EQ(object_it, object_value.end_member());

  // test the list range
  test_readonly_list_range(garlic::rapidjson_readonly_layer(doc["values"]));

  // test the object range
  test_readonly_object_range(garlic::rapidjson_readonly_layer(doc["objects"]));
}
