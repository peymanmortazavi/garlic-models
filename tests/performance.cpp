#include <iostream>
#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include "garlic/providers/libyaml.h"
#include "test_utility.h"


static void BM_StringCreation(benchmark::State& state) {
  for (auto _ : state) {
    garlic::Module<garlic::providers::libyaml::YamlView> module;
    load_libyaml_module(module, "data/performance/module.yaml");
  }
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

BENCHMARK_MAIN();
