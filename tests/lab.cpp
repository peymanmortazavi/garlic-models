#include "garlic/layer.h"
#include "rapidjson/document.h"
#include <garlic/garlic.h>
#include <garlic/providers/rapidjson.h>

static rapidjson::Document& CreateLargeRapidJsonDocument(int iterations) {
  static rapidjson::Document* d;
  if (!d) {
    d = new rapidjson::Document();
    d->SetArray();
    for (size_t i = 0; i < iterations; i++) {
      auto object = rapidjson::Value();
      object.SetObject();
      object.AddMember(
          "text",
          "This is a long text just for the purpose of testing.",
          d->GetAllocator());
      object.AddMember("value", 18231, d->GetAllocator());
      d->PushBack(std::move(object), d->GetAllocator());
    }
  }
  return *d;
}

static long long native_way(rapidjson::Document& doc) {
  long long character_count = 0;
  long long total = 0;
  for (const auto& object_it : doc.GetArray()) {
    if (auto y = object_it.FindMember("text"); y != object_it.MemberEnd()) {
      character_count += strlen(y->value.GetString());
    }
    if (auto y = object_it.FindMember("value"); y != object_it.MemberEnd()) {
      total += y->value.GetInt();
    }
  }
  return character_count + total;
}

static long long garlic_way(rapidjson::Document& doc) {
  garlic::providers::rapidjson::JsonView view {doc};
  long long character_count = 0;
  long long total = 0;
  for (const auto& object_it : view.get_list()) {
    if (auto y = object_it.find_member("text"); y != object_it.end_member()) {
      character_count += strlen((*y).value.get_cstr());
    }
    if (auto y = object_it.find_member("value"); y != object_it.end_member()) {
      total += (*y).value.get_int();
    }
  }
  return character_count + total;
}

  template<typename T>
  concept WrapContainer = requires(T container) {
    typename T::output_type;
    typename T::iterator_type;

    { container.iterator };
    { container.wrap() } -> std::same_as<typename T::output_type>;
  };


int main()
{
  int iterations;
  std::cin >> iterations;
  auto& doc = CreateLargeRapidJsonDocument(iterations);
  printf("%lld", garlic_way(doc));
  return 0;
}
