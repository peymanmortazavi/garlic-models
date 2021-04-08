#include "garlic/adapters/rapidjson/reader.h"
#include "rapidjson/filereadstream.h"
#include <benchmark/benchmark.h>
#include <rapidjson/document.h>
#include <garlic/garlic.h>
#include <garlic/adapters/rapidjson.h>

using namespace garlic;
using namespace garlic::adapters::rapidjson;

static rapidjson::Document& CreateLargeRapidJsonDocument() {
  static rapidjson::Document* d;
  if (!d) {
    int iterations = (rand() % 10) + 100000;
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

static void BM_CreateJsonDocument_Native(benchmark::State& state) {
  int iterations = (rand() % 1) + 100000;
  for (auto _ : state) {
    rapidjson::Document doc;
    doc.SetArray();
    for (auto i = 0; i < iterations; ++i) {
      doc.PushBack(::rapidjson::Value(12).Move(), doc.GetAllocator());
      doc.PushBack(
          ::rapidjson::Value().SetString(
            "Some Text Value Here", doc.GetAllocator()), doc.GetAllocator());
      doc.PushBack(
          ::rapidjson::Value().SetArray()
            .PushBack(::rapidjson::Value(12).Move(), doc.GetAllocator()),
          doc.GetAllocator()
      );
    }
    state.counters["array"] += doc.Capacity();
  }
}

static void BM_CreateJsonDocument_Garlic(benchmark::State& state) {
  int iterations = (rand() % 1) + 100000;
  for (auto _ : state) {
    JsonDocument doc;
    doc.set_list();
    for (auto i = 0; i < iterations; ++i) {
      doc.push_back(12);
      doc.push_back("Some Text Value Here");
      doc.push_back_builder([](auto item) {
          item.set_list();
          item.push_back(12);
          });
    }
    state.counters["array"] += doc.get_inner_value().Capacity();
  }
}

BENCHMARK(BM_CreateJsonDocument_Garlic);
BENCHMARK(BM_CreateJsonDocument_Native);


static void BM_ReadDocument(benchmark::State& state) {
  for (auto _ : state) {
    auto file = fopen("data/test.json", "r");
    rapidjson::Document doc;
    char buffer[256];
    auto input_stream = rapidjson::FileReadStream(file, buffer, sizeof(buffer));
    doc.ParseStream(input_stream);
    fclose(file);
  }
}

static void BM_ReadLayer(benchmark::State& state) {
  for (auto _ : state) {
    auto file = fopen("data/test.json", "r");
    rapidjson::Reader reader;
    rapidjson::Document doc;
    char buffer[256];
    auto input_stream = rapidjson::FileReadStream(file, buffer, sizeof(buffer));
    auto handler = make_handler(JsonRef(doc));
    reader.Parse(input_stream, handler);
    fclose(file);
  }
}

BENCHMARK(BM_ReadDocument);
BENCHMARK(BM_ReadLayer);

BENCHMARK_MAIN();
