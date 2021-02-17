/******************************************************************************
* File:             test_json.cpp
*
* Author:           Peyman Mortazavi
* Created:          02/03/20
* Description:      Testing the JSON support for garlic.
*****************************************************************************/

#include <algorithm>
#include <iostream>
#include <fstream>
#include <deque>
#include <string>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>

#include "../../test_protocol.h"
#include "garlic/meta.h"
#include "garlic/utility.h"

using namespace std;

using namespace garlic;
using namespace garlic::providers::rapidjson;
using namespace rapidjson;


Document get_test_document() {
  ifstream ifs("data/test.json");
  IStreamWrapper isw(ifs);
  Document d;
  d.ParseStream(isw);
  return d;
}


TEST(RapidJson, DocumentTest) {
  JsonDocument doc;

  doc.set_int(12);
  ASSERT_EQ(doc.get_int(), 12);

  // Make sure getting referenece and view is working.
  doc.get_reference().set_int(15);
  ASSERT_EQ(doc.get_view().get_int(), 15);
  ASSERT_EQ(doc.get_reference().get_int(), 15);
  ASSERT_EQ(doc.get_int(), 15);

  doc.set_object();

  std::vector<std::string> names;
  names.push_back("a");
  names.push_back("b");
  names.push_back("c");

  JsonValue names_value {doc};  // initialize JsonValue via JsonDocument.
  names_value.set_list();
  copy(begin(names), end(names), garlic::back_inserter(names_value));

  doc.add_member("names", names_value);
  auto saved_names = (*doc.find_member("names")).value;
  for (auto item : saved_names.get_list()) {
    ASSERT_STREQ(item.get_cstr(), names[0].c_str());
    names.erase(begin(names));
  }
  
  test_full_layer(doc);
  test_full_layer(doc.get_reference());
}


TEST(RapidJson, ProtocolTest) {
  Document doc = get_test_document();

  // test happy path values for keys that do actually exist.
  test_readonly_string_value(JsonView(doc["name"]), "Peyman");
  test_readonly_int_value(JsonView(doc["age"]), 28);
  test_readonly_double_value(JsonView(doc["h"]), 12.123);
  test_readonly_bool_value(JsonView(doc["ready"]), true);
  test_readonly_null_value(JsonView(doc["career"]));

  // test the list values.
  auto list_value = JsonView(doc["values"]);
  ASSERT_TRUE(list_value.is_list());
  auto list_it = list_value.begin_list();
  test_readonly_double_value(*list_it, 1.1); list_it++;
  test_readonly_int_value(*list_it, 25); list_it++;
  test_readonly_string_value(*list_it, "Test"); list_it++;
  test_readonly_bool_value(*list_it, true); list_it++;
  test_readonly_null_value(*list_it); list_it++;
  ASSERT_EQ(list_it, list_value.end_list());

  // test the object values.
  auto object_value = JsonView(doc["objects"]);
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
  test_readonly_list_range(JsonView(doc["values"]));

  // test the object range
  test_readonly_object_range(JsonView(doc["objects"]));

  // the writable object.
  test_full_layer(JsonRef{doc});
}


TEST(RapidJson, EqualityTest) {
  Document doc = get_test_document();
  ASSERT_EQ(JsonView(doc), JsonView(doc));
  ASSERT_TRUE(cmp_layers(JsonView(doc), JsonView(doc)));

  ASSERT_EQ(JsonRef(doc), JsonRef(doc));
  ASSERT_TRUE(cmp_layers(JsonRef(doc), JsonRef(doc)));
 
  ASSERT_EQ(JsonRef(doc), JsonRef(doc));
  ASSERT_TRUE(cmp_layers(JsonRef(doc), JsonRef(doc)));

  ASSERT_EQ(JsonView(doc), JsonRef(doc));
  ASSERT_EQ(JsonRef(doc), JsonView(doc));
  ASSERT_TRUE(cmp_layers(JsonRef(doc), JsonView(doc)));
  ASSERT_TRUE(cmp_layers(JsonView(doc), JsonRef(doc)));
}
