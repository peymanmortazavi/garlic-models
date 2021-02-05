#include <gtest/gtest.h>
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>
#include <garlic/encoding.h>
#include <garlic/clove.h>
#include <memory>

#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;

TEST(Encoding, DefaultLayerCopy) {
  auto doc = get_rapidjson_document("data/test.json");
  CloveDocument target;
  copy_layer(doc.get_view(), target.get_reference());
  ASSERT_TRUE(cmp_layers(doc.get_view(), target.get_view()));
}

inline static CloveRef get_test_clove() {
  static std::unique_ptr<CloveDocument> doc;
  if (!doc) {
    doc = std::make_unique<CloveDocument>();
    auto ref = doc->get_reference();
    ref.set_object();
    ref.add_member("prop1", "Property 1");
    ref.add_member("prop2", 24);
  }
  return doc->get_reference();
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

struct Data3 {
  std::string prop1;
  int prop2;
};

template<typename Layer>
struct coder<Data3, Layer> {
  static inline Data3 decode(Layer layer) {
    return {
      (*layer.find_member("prop1")).value.get_string(),
      (*layer.find_member("prop2")).value.get_int(),
    };
  }
};

TEST(Encoding, CustomStaticClassDecoder) {
  auto data = decode<Data1>(get_test_clove());
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);
}

TEST(Encoding, CustomConstructorClassDecoder) {
  auto data = decode<Data2>(get_test_clove());
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);
}

TEST(Encoding, CustomExplicitDecoder) {
  auto data = decode<Data3>(get_test_clove());
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);
}
