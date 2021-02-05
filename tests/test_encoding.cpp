#include <gtest/gtest.h>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/encoding.h>
#include <garlic/clove.h>

#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;

TEST(Encoding, DefaultLayerCopy) {
  auto doc = get_rapidjson_document("data/test.json");
  CloveDocument target;
  copy_layer(doc.get_view(), target.get_reference());
  ASSERT_TRUE(cmp_layers(doc.get_view(), target.get_view()));
}

struct Data1 {
  std::string prop1;
  int prop2;

  template<typename Layer>
  static inline Data1 decode(Layer layer) {
    return {
      (*layer.find_member("prop1")).value.get_string(),
      (*layer.find_member("prop2")).value.get_int()
    };
  }
};

class Data2 {
public:
  template<typename Layer>
  Data2(Layer layer) {
    prop1 = (*layer.find_member("prop1")).value.get_string();
    prop2 = (*layer.find_member("prop2")).value.get_int();
  }

  std::string prop1;
  int prop2;
};

TEST(Encoding, CustomStaticClassDecoder) {
  CloveDocument doc;
  auto target = doc.get_reference();
  target.set_object();
  target.add_member("prop1", "Property 1");
  target.add_member("prop2", 24);
  auto data = decode<Data1>(target);
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);
}

TEST(Encoding, CustomConstructorClassDecoder) {
  CloveDocument doc;
  auto target = doc.get_reference();
  target.set_object();
  target.add_member("prop1", "Property 1");
  target.add_member("prop2", 24);
  auto data = decode<Data2, CloveRef>(target);
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);
}
