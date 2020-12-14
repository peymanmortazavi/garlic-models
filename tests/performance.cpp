#include <iostream>
#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include "garlic/providers/libyaml.h"
#include "garlic/providers/rapidjson.h"
#include "test_utility.h"


static void BM_LibYamlModuleLoad(benchmark::State& state) {
  for (auto _ : state) {
    garlic::Module<garlic::providers::libyaml::YamlView> module;
    load_libyaml_module(module, "data/performance/module.yaml");
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
