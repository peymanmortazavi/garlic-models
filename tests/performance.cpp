#include <iostream>
#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include "garlic/providers/libyaml.h"
#include "garlic/providers/rapidjson.h"
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

BENCHMARK_MAIN();
