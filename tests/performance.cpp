#include <benchmark/benchmark.h>
#include <garlic/garlic.h>
#include "garlic/providers/libyaml/parser.h"
#include "garlic/providers/rapidjson.h"
#include "garlic/containers.h"
#include "test_utility.h"


static void BM_LibYamlModuleLoad(benchmark::State& state) {
  auto doc = get_libyaml_document("data/performance/module.yaml");
  for (auto _ : state) {
    garlic::Module<garlic::providers::libyaml::YamlView> module;
    module.parse(doc.get_view());
  }
}
//BENCHMARK(BM_LibYamlModuleLoad);

static void BM_LoadRapidJsonModule(benchmark::State& state) {
  garlic::Module<garlic::providers::rapidjson::JsonView> module;
  load_libyaml_module(module, "data/performance/module.yaml");
  auto doc = get_rapidjson_document("data/performance/test1.json");
  for (auto _ : state) {
    module.get_model("Module")->validate(doc.get_view());
  }
}
//BENCHMARK(BM_LoadRapidJsonModule);

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
  garlic::providers::rapidjson::JsonView view {CreateLargeRapidJsonDocument()};
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
BENCHMARK(BM_Vector_LongDouble);
BENCHMARK(BM_Sequence_LongDouble);

static void BM_Vector_LargeConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 128;
  for (auto _ : state) {
    std::vector<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::scalar_field_failure("Some Text", "Blah"));
    }
  }
}

static void BM_Sequence_LargeConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 128;
  for (auto _ : state) {
    garlic::sequence<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::scalar_field_failure("Some Text", "Blah"));
    }
  }
}
BENCHMARK(BM_Vector_LargeConstraintResult);
BENCHMARK(BM_Sequence_LargeConstraintResult);

static void BM_Vector_ConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 7;
  for (auto _ : state) {
    std::vector<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::scalar_field_failure("Some Text", "Blah"));
    }
  }
}

static void BM_Sequence_ConstraintResult(benchmark::State& state) {
  int iterations = (rand() % 1) + 7;
  for (auto _ : state) {
    garlic::sequence<garlic::ConstraintResult> x(8);
    for (auto i = 0; i < iterations; ++i) {
      x.push_back(garlic::ConstraintResult::scalar_field_failure("Some Text", "Blah"));
    }
  }
}
BENCHMARK(BM_Vector_ConstraintResult);
BENCHMARK(BM_Sequence_ConstraintResult);

static void BM_std_string(benchmark::State& state) {
  for (auto _ : state) {
    std::string x("This is something quite long and we need to overlook this.");
  }
}

static void BM_garlic_text(benchmark::State& state) {
  for (auto _ : state) {
    garlic::text x("This is something quite long and we need to overlook this.", garlic::text_type::copy);
  }
}
BENCHMARK(BM_std_string);
BENCHMARK(BM_garlic_text);

//BENCHMARK(BM_LoadRapidJsonDocument_Native);
//BENCHMARK(BM_LoadRapidJsonDocument_Garlic);

BENCHMARK_MAIN();
