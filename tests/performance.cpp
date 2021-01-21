#include <cstring>
#include <iostream>
#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include "garlic/layer.h"
#include "garlic/providers/libyaml.h"
#include "garlic/providers/rapidjson.h"
#include "rapidjson/document.h"
#include "test_utility.h"


static void BM_LibYamlModuleLoad(benchmark::State& state) {
  auto doc = get_libyaml_document("data/performance/module.yaml");
  for (auto _ : state) {
    garlic::Module<garlic::providers::libyaml::YamlView> module;
    module.parse(doc.get_view());
  }
}
BENCHMARK(BM_LibYamlModuleLoad);

static void BM_LoadRapidJsonModule(benchmark::State& state) {
  garlic::Module<garlic::providers::rapidjson::JsonView> module;
  load_libyaml_module(module, "data/performance/module.yaml");
  auto doc = get_rapidjson_document("data/performance/test1.json");
  for (auto _ : state) {
    module.get_model("Module")->validate(doc.get_view());
  }
}
BENCHMARK(BM_LoadRapidJsonModule);

static rapidjson::Document& CreateLargeRapidJsonDocument() {
  static rapidjson::Document* d;
  if (!d) {
    d = new rapidjson::Document();
    d->SetArray();
    for (size_t i = 0; i < 1000; i++) {
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

static void BM_LoadRapidJsonDocument_Native(benchmark::State& state) {
  auto& doc = CreateLargeRapidJsonDocument();
  long long character_count = 0;
  long long total = 0;
  for (auto _ : state) {
    for (const auto& object_it : doc.GetArray()) {
      if (auto y = object_it.FindMember("text"); y != object_it.MemberEnd()) {
        character_count += strlen(y->value.GetString());
      }
      if (auto y = object_it.FindMember("value"); y != object_it.MemberEnd()) {
        total += y->value.GetInt();
      }
    }
  }
  printf("%lld - %lld\n", character_count, total);
}
BENCHMARK(BM_LoadRapidJsonDocument_Native);

static void BM_LoadRapidJsonDocument_Garlic(benchmark::State& state) {
  garlic::providers::rapidjson::JsonView view {CreateLargeRapidJsonDocument()};
  long long character_count = 0;
  long long total = 0;
  for (auto _ : state) {
    for (const auto& object_it : view.get_list()) {
      if (auto y = object_it.find_member("text"); y != object_it.end_member()) {
        //character_count += strlen((*y).value.get_cstr());
        character_count += (*y).value.get_string_view().size();
      }
      if (auto y = object_it.find_member("value"); y != object_it.end_member()) {
        total += (*y).value.get_int();
      }
    }
  }
  printf("%lld - %lld\n", character_count, total);
}
BENCHMARK(BM_LoadRapidJsonDocument_Garlic);

static void BM_LoadRapidJsonDocument_GarlicObject(benchmark::State& state) {
  auto view = garlic::ObjectView<garlic::providers::rapidjson::JsonView2>(CreateLargeRapidJsonDocument());
  long long character_count = 0;
  long long total = 0;
  for (auto _ : state) {
    for (const auto& object_it : view.get_list()) {
      if (auto y = object_it.find_member("text"); y != object_it.end_member()) {
        //character_count += strlen((*y).value.get_cstr());
        character_count += (*y).value.get_string_view().size();
      }
      if (auto y = object_it.find_member("value"); y != object_it.end_member()) {
        total += (*y).value.get_int();
      }
    }
  }
  printf("%lld - %lld\n", character_count, total);
}
BENCHMARK(BM_LoadRapidJsonDocument_GarlicObject);

BENCHMARK_MAIN();
