# Introduction

A data container protocol and schema validation for C++ 20

> Currently this is only working for GCC 10 and C++ 20 but other compilers will be supported.

## What is this?

This library provides three things:


### 1. Data Container Protocol

Woooah what a buzz word. It's just two C++ concepts to provide a common interface
so that various different data containers could be described using a set of methods.

Long story short, most JSON and YAML libraries you find are probably going to have
ways to get strings, numbers, booleans, lists and objects. What if we make a contract that
designates one interface that includes a common set of functions these libraries provide.
Then we can wrap these types with adapters so they all have same methods then we can write code
agnostic to what library we're working with.

This is mostly to make the schema validation available to a variety of different formats and libraries that
hold data similar to JSON, YAML, XML, MessagePack, etc.

```cpp
template<garlic::ViewLayer Layer>
void iterate_over_lists(Layer&& layer) {
    for (const auto& item : layer.get_list()) {
        if (!item.is_string())
            continue;
        std::cout << item.get_string_view() << std::endl;
    }
}
```

### 2. Schema Validation to validate data containers

There is a constraint system you can work with. The plan is to add parsers for JSON Schema and other formats
to enable the developers use whatever format they like.

But there is also a built-in format for describing models and schemas.

Consider the following code:

```cpp
auto username_constraint = garlic::make_constraint<garlic::regex_tag>("\\w{3,12}");

rapidjson::Document json_doc;  // say you have a RapidJSON document.
if (auto result = username_constraint.test(JsonView(json_doc)); !result) {
    std::cout << "Invalid username: " << result.reason << std::endl;
}

yaml_document_t* yaml_doc;  // say you have a yaml_document_t;
if (!username_constraint.quick_test(YamlView(yaml_doc))) {  // don't care about details.
    std::cout << "Invalid username!" << std::endl;
}
```

You can also define all these models, fields and constraints in a YAML or JSON file and load a module.

```cpp
int main() {
    // Load the module file using libyaml and store it in a CloveDocument.
    FILE* module_file = fopen("models.yaml", "r");
    garlic::CloveDocument doc;
    garlic::providers::libyaml::load(module_file, doc);
    fclose(module_file);

    // Get a garlic::Module out of that! you can use any layer type here.
    // So long as it conforms to garlic::ViewLayer
    auto result = garlic::parsing::load_module(doc);
    if (!result) {
        std::cout << result.error().message() << std::endl;
        return 1;
    }

    // Say you want to load a config file for your http server.
    // Get the model HttpConfig from the module, this would be defined in your module file.
    auto config_model = result->get_model("HttpConfig");

    // Load the config file using rapidjson.
    FILE* config_file = fopen("configs.json", "r");
    auto configs = garlic::providers::rapidjson::Json::load(config_file);
    fclose(config_file);

    // Validate the configs.
    auto validation_results = config_model->validate(configs);
    if (!validation_results) {
        std::cout << validation_results.reason << std::endl;
        return 1;
    } else {
        std::cout << "Hey you have good configs! nice going!" << std::endl;
        // maybe decode the configs into something useful.
    }
    return 0;
}
```

The nice thing is that these wrappers likely get optimized away, leaving very minimal cost to this abstraction.

### 3. Encoding Utilities to serialize and deserialize custom C++ classes and types.


```cpp
struct User {
    std::string name;

    template<garlic::ViewLayer Layer>
    static User decode(Layer&& layer) {
        return User {
            .name = garlic::get<std::string>(layer, "name")
        };
    }
};

struct Group {
    std::vector<User> users;

    template<garlic::ViewLayer Layer>
    static Group decode(Layer&& layer) {
        Group group;
        std::for_each(
            layer.begin_list(), layer.end_list(),
            [&group](const auto& value) {
                group.users.emplace_back(garlic::decode<User>(value));
            });
    }
};

int main() {
    garlic::CloveDocument doc;

    FILE* file = fopen("data.yaml", "r");
    garlic::providers::libyaml::load(file, doc);  // use libyaml to populate CloveDocument.

    Group group = garlic::decode<Group>(doc);  // parse Group from CloveDocument.
    return 0;
}
```

## What's missing?

* There's just more work to be done to make this ready for production use and there are still
parts that might change since they're not battle tested.

* There is only support for RapidJSON and some limited support for libyaml and yaml-cpp. More
libraries need to be supported before this library can be really used by users who don't
use RapidJSON in their code.

* Currently only GCC 10 and C++ 20 is supported. We can work on the source code to bring that
down to at least C++ 17 and support more compilers. This is just very alpha at the moment.

* More tests and performance benchmarks to make sure this library is actually fast.

* Compilation takes forever! This is going to become annoying. There are ways to improve this.

* Better documentation, there are some but not sure if it's good to set someone up their way.

## How do I play with this?

You can either use the docker-compose file in the `docker` directory to compile all tests and lab.cpp
or if you have GCC 10 and C++ 20 around, you can just compile lab.cpp and take it for a ride!

You might want to disable running tests in `tests/CMakeLists.txt` if that long compilation bothers you.

## Performance Tests

Just don't run them :) they're not ready but if you do, run the following commands beforehand.

```bash
sudo cpupower frequency-set --governor performance
# run your performance tests.
sudo cpupower frequency-set --governor powersave
```
