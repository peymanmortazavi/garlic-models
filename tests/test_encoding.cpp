#include <gtest/gtest.h>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/encoding.h>
#include <garlic/clove.h>

#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;

TEST(Encoding, DefaultLayerDecode) {
  auto doc = get_rapidjson_document("data/test.json");
  CloveDocument target;
  copy_layer(doc.get_view(), target.get_reference());
  ASSERT_TRUE(cmp_layers(doc.get_view(), target.get_view()));
}
