/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <algorithm>
#include <gtest/gtest.h>
#include <garlic/garlic.h>
#include <iterator>
#include <iostream>
#include "test_protocol.h"

using namespace garlic;
using namespace std;


TEST(BasicValue, ProtocolTest) {
  CloveDocument doc;
  test_full_layer(doc.get_reference());

  //garlic_parser parser;
  //garlic_module module;
  //garlic_parser_init(&parser);
  //FILE* f;
  //garlic_parser_load(&module, f);  // file handle.
  //garlic_parser_load(&module, stream); // look into the stream wrapper for rapidjson.
  //garlic_parser_load(&module, "raw string");  // raw string.

  //garlic_model* model;
  //model = garlic_module_get_model("UserModel");
  //model->test();  // how do we deal with the different types? like YamlView, JsonView or even the custom made ones?
  //// potential solution: we could pass a type as a way to figure out what type it is. this needs to allow custom ones.

  //garlic_parser_delete(&parser);
  //garlic_module_delete(&my_module);
}
