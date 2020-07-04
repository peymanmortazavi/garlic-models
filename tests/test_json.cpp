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
  //auto d = get_test_document();
  //auto root = rapidjson_wrapper(d);
}


TEST(DocumentTests, ProtocolTest) {
  Document doc = get_test_document();

  // test happy path values for keys that do actually exist.
  test_readonly_string_value(garlic::rapidjson_readonly_layer(doc["name"]), "Peyman");
  test_readonly_int_value(garlic::rapidjson_readonly_layer(doc["age"]), 28);
  test_readonly_double_value(garlic::rapidjson_readonly_layer(doc["h"]), 12.123);
  test_readonly_bool_value(garlic::rapidjson_readonly_layer(doc["ready"]), true);
  test_readonly_null_value(garlic::rapidjson_readonly_layer(doc["career"]));

  //auto element = rapidjson_wrapper(get_test_document());
  //test_object_string_capabilities(element);
}
