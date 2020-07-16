/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>
#include <iterator>
#include "test_protocol.h"

using namespace garlic;


TEST(BasicValue, ProtocolTest) {
  Data d;
  CloveView v{d};
  test_readonly_int_value(v, 2);
} 
