#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include <string>
#include <tuple>
#include "garlic/constraints.h"
#include "garlic/adapters/libyaml/parser.h"
#include "garlic/adapters/rapidjson.h"
#include "garlic/containers.h"
#include "test_utility.h"


static rapidjson::Document& CreateLargeRapidJsonDocument() {
  static rapidjson::Document* d;
  if (!d) {
    int iterations = (rand() % 10) + 100000;
    d = new rapidjson::Document();
    d->SetArray();
    for (size_t i = 0; i < iterations; i++) {
      auto object = rapidjson::Value();
      object.SetString(("A" + std::to_string(i)).c_str(), d->GetAllocator());
      //object.SetObject();
      //object.AddMember(
      //    "text",
      //    "This is a long text just for the purpose of testing.",
      //    d->GetAllocator());
      //object.AddMember("value", 18231, d->GetAllocator());
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
  state.counters["CharacterCount"] = character_count;
  state.counters["Total"] = total;
}

static void BM_LoadRapidJsonDocument_Garlic(benchmark::State& state) {
  garlic::adapters::rapidjson::JsonView view {CreateLargeRapidJsonDocument()};
  long long character_count = 0;
  long long total = 0;
  const char* text = "text";
  const char* value = "value";
  for (auto _ : state) {
    for (const auto& object_it : view.get_list()) {
      if (auto y = object_it.find_member(text); y != object_it.end_member()) {
        character_count += strlen((*y).value.get_cstr());
      }
      if (auto y = object_it.find_member(value); y != object_it.end_member()) {
        total += (*y).value.get_int();
      }
    }
  }
  state.counters["CharacterCount"] = character_count;
  state.counters["Total"] = total;
}

static void BM_Vector_LongDouble(benchmark::State& state) {
  int iterations = (rand() % 1) + 1000;
  for (auto _ : state) {
    std::vector<long double> x(8);
    for (auto i = 0; i < iterations; ++i) x.push_back(i);
  }
}

static void BM_Sequence_LongDouble(benchmark::State& state) {
  int iterations = (rand() % 1) + 1000;
  for (auto _ : state) {
    garlic::sequence<long double> x(8);
    for (auto i = 0; i < iterations; ++i) x.push_back(i);
  }
}
//BENCHMARK(BM_Vector_LongDouble);
//BENCHMARK(BM_Sequence_LongDouble);

static void BM_Vector_LargeConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 128;
  for (auto _ : state) {
    std::vector<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::leaf_field_failure("Some Text", "Blah"));
    }
  }
}

static void BM_Sequence_LargeConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 128;
  for (auto _ : state) {
    garlic::sequence<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::leaf_field_failure("Some Text", "Blah"));
    }
  }
}
//BENCHMARK(BM_Vector_LargeConstraintResult);
//BENCHMARK(BM_Sequence_LargeConstraintResult);

static void BM_Vector_ConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 7;
  for (auto _ : state) {
    std::vector<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::leaf_field_failure("Some Text", "Blah"));
    }
  }
}

static void BM_Sequence_ConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 7;
  for (auto _ : state) {
    garlic::sequence<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::leaf_field_failure("Some Text", "Blah"));
    }
  }
}
//BENCHMARK(BM_Vector_ConstraintResult);
//BENCHMARK(BM_Sequence_ConstraintResult);

static void BM_std_string(benchmark::State& state) {
  for (auto _ : state) {
    auto text = std::string("This is quite fascinating. A very large fucking string because blah blah.");
  }
}

static void BM_garlic_text(benchmark::State& state) {
  for (auto _ : state) {
    auto text = garlic::text::copy("This is quite fascinating. A very large fucking string because blah blah.");
  }
}
BENCHMARK(BM_garlic_text);
BENCHMARK(BM_std_string);

//BENCHMARK(BM_LoadRapidJsonDocument_Native);
//BENCHMARK(BM_LoadRapidJsonDocument_Garlic);

BENCHMARK_MAIN();
