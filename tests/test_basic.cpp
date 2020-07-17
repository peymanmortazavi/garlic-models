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
#include "garlic/allocators.h"
#include "garlic/basic.h"
#include "test_protocol.h"

using namespace garlic;
using namespace std;


TEST(BasicValue, ProtocolTest) {
  CloveDocument doc;
  test_full_layer(doc.get_reference());
} 
