/******************************************************************************
* File:             test_default_values.cpp
*
* Author:           Peyman Mortazavi
* Created:          11/25/19
* Description:      Tests out default value implementations (garlic::object, garlic::string, etc.)
*****************************************************************************/

#include <gtest/gtest.h>
#include <garlic/garlic.h>

#include "test_protocol.h"


TEST(CloveValue, ProtocolTest) {
  garlic::CloveDocument doc;
  test_full_layer(doc);
  test_full_layer(doc.get_reference());
}
