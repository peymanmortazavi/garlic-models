#include <memory>

#include "../include/garlic/garlic.h"

using namespace std;
using namespace garlic;

template<> const value::store_type value::no_result = nullptr;
template<> const shareable_value::store_type shareable_value::no_result = nullptr;

template<> const value::store_type value::none { new value(type_flag::null) };
template<> const shareable_value::store_type shareable_value::none { new shareable_value(type_flag::null) };
