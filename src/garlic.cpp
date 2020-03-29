#include <memory>

#include "../include/garlic/garlic.h"

using namespace std;
using namespace garlic;


template<> const value value::none{type_flag::null};
template<> const s_value s_value::none{type_flag::null};


int test() { return 0; }
