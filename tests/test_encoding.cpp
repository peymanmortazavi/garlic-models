#include <memory>

#include <gtest/gtest.h>

#include <garlic/clove.h>
#include <garlic/utility.h>

#include "test_utility.h"

using namespace garlic;
using namespace garlic::providers::rapidjson;

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
      get<std::string>(layer, "prop1"),
      get<int>(layer, "prop2")
    };
  }

  template<typename Layer, typename Callable>
  static inline void safe_decode(Layer layer, Callable&& cb) {}
};

class Data2 {
public:
  Data2(const char* prop1, int prop2) : prop1(prop1), prop2(prop2) {}

  template<typename Layer>
  Data2(Layer layer) {
    prop1 = get<std::string>(layer, "prop1");
    prop2 = get<int>(layer, "prop2");
  }
  Data2(const Data2&) = delete;

  template<typename Layer>
  static inline void encode(Layer layer, const Data2& value) {
    layer.set_object();
    layer.add_member("prop1", value.prop1.c_str());
    layer.add_member("prop2", value.prop2);
  }

  std::string prop1;
  int prop2;
};

struct Data3 {
  std::string prop1;
  int prop2;
};

template<>
struct garlic::coder<Data3> {

  template<typename Layer>
  static inline Data3 decode(Layer&& layer) {
    return {
      get<std::string>(layer, "prop1"),
      get<int>(layer, "prop2")
    };
  }

  template<typename Layer>
  static inline void encode(Layer&& layer, const Data3& value) {
    layer.set_object();
    layer.add_member("prop1", value.prop1.c_str());
    layer.add_member("prop2", value.prop2);
  }

  template<GARLIC_VIEW Layer, typename Callable>
  static inline void safe_decode(Layer&& layer, Callable&& cb) {
    cb(Data3{
        get<std::string>(layer, "prop1", "default"),
        safe_get<int>(layer, "prop2", 0)
        });
  }
};

TEST(Encoding, CustomStaticClassDecoder) {
  auto data = decode<Data1>(get_test_clove());
  ASSERT_STREQ(data.prop1.c_str(), "Property 1");
  ASSERT_EQ(data.prop2, 24);

  safe_decode<Data1>(get_test_clove(), [](auto&&) {
      ASSERT_TRUE(false);
      });
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

  safe_decode<Data3>(get_test_clove(), [](auto result){
      ASSERT_STREQ(result.prop1.c_str(), "Property 1");
      ASSERT_EQ(result.prop2, 24);
      });
}

TEST(Encoding, CustomExplicitEncoder) {
  Data3 value {"Property", 30};
  CloveDocument doc;
  encode(doc.get_reference(), value);
  ASSERT_STREQ(safe_get<const char*>(doc.get_view(), "prop1", ""), "Property");
  ASSERT_EQ(safe_get<int>(doc.get_view(), "prop2", 0), 30);
}

TEST(Encoding, CustomStaticClassEncoder) {
  Data2 value("Property", 30);
  CloveDocument doc;
  encode(doc.get_reference(), value);
  ASSERT_STREQ(safe_get<const char*>(doc.get_view(), "prop1", ""), "Property");
  ASSERT_EQ(safe_get<int>(doc.get_view(), "prop2", 0), 30);
}
