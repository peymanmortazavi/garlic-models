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
  CloveData d;
  CloveView v{d};
  auto allocator = CAllocator();
  CloveRef r{d, allocator};
  r.set_list();
  r.push_back("a");
  r.push_back("b1");
  r.push_back("b2");
  r.push_back("c");
  r.erase(std::next(r.begin_list(), 1), std::next(r.begin_list(), 2));
  for(auto item : r.get_list()) {
    std::cout << item.get_string() << std::endl;
  }
  test_full_layer(CloveRef{d, allocator});
} 
