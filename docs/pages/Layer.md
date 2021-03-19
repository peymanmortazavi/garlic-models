# Layer / Data Container Protocol

The whole idea of this library got started because I wanted to build a
schema validation library but it obviously had to work with multiple JSON and Yaml
libraries because people use different libraries and some make their own.

How do we make this library work for other folks? we make a wrapper!
This is nothing new, there are already some other libraries that do
a very similar thing if not the same to accomplish this.

This libraries tries to use concepts and provide easy ways to establish
some sort of contract for all these wrappers without relying on any
virtual tables so we can remain fast and yet provide a somewhat easy
way to make wrappers for arbitrary data containers.

## What's the benefit?

* Use schema validation in this library to validate data represented by various different data containers.
* You could acutally make your code agnostic to what library you use for data containers (JSON, Yaml, etc.)
  This way you could rely on different libraries for different architectures if needed without modifying your code much.
* Depending on capabilities of the data container, you could pay almost no cost for this abstraction.

**The two main concepts are garlic::ViewLayer and garlic::RefLayer** and the idea is that
you either want a read-only layer to be able to consume values from it or you want to
be able to write to it as well. garlic::RefLayer is a garlic::ViewLayer that has writing
methods as well.

These concepts define the bare-minimum set of functions and type alises to make a layer readable.
But some layers have additional optional features. You can take adventage of these as well.

Example:

```cpp
#include <rapidjson/document.h>

#include <garlic/encoding.h>
#include <garlic/providers/rapidjson.h>

using garlic::providers::rapidjson::JsonView;

struct Config {
   std::string host;
   unsigned port;

   template<GARLIC_VIEW Layer>
   static Config decode(Layer&& layer) {
      Config config;
      config.host = garlic::get(layer, "hostname", "localhost");
      config.port = garlic::get(layer, "port", 5000);
      return config;
   }
};

int main() {
   rapidjson::Document doc;  // use a rapidjson document.
   auto config = garlic::decode<Config>(JsonView(doc));
}
```

See [Encoding Documentation](docs/pages/Encoding.md) for more details.

As you can see in the above example, you can use a wrapper or a *provider* for RapidJSON, a populate and fast
JSON library to populate a custom class. Because of this concept, as long as you can provide these wrappers
for your data containers you can use them with garlic for schema validation and encoding/decoding. You don't
have to change your decoding logic for your Config yet you can support reading files from MessagePack, JSON
and Yaml and even your own custom containers. In many instances including the example above, compiler might
optimize away all these wrappers giving you a near zero cost for this abstraction.

You will see a lot of `View`, `Reference` and `Value` or `Document` in garlic.

### View
View is similar to std::string_view in that it is simply a view object. You can wrap
various different data container types using a view wrapper like `JsonView` for RapidJSON
and call your generic methods that know how to read from a garlic::ViewLayer. This is ideal for the
following situation:

```cpp
static void load_config(const rapidjson::Value& value) {
   // you only have access to a const reference.
   auto config = Config::decode(JsonView(value));
}

static void load_config(const yaml_document_t* doc) {
   // you only have access to a const libyaml document pointer.
   auto config = Config::decode(YamlView(doc));
}
```

### Reference

Reference is similar to passing a value by reference but in this case a small wrapper object carries the reference.

```cpp
template<GARLIC_REF Layer>
static void write_config(Layer&& layer, const Config& config) {
   layer.set_object();
   layer.add_member("hostname", config.host.c_str());
   layer.add_member("port", static_cast<int>(config.port));
}

static void save_json_config(rapidjson::Value& value, const Config& config) {
   write_config(JsonRef(value), config);
}

...
```

Why is this reference needed? mostly so you don't own an object and also because using
iterators would have to return references but obviously it would have to be a wrapper type
conforming to garlic::RefLayer

```cpp
template<typename Layer>
add_bonus(Layer&& layer) {
   unsigned score = 0;
   for (auto item : layer.get_list()) {  // item is a reference object!
      item.set_int(item.get_int() + 10);
   }
}

int main () {
   rapidjson::Document doc;  // assume it's a list of numbers.
   add_bonus(JsonRef(doc));
}
```

As you can see, it's a bit counter-intuitive that item in the for loop is actually an object
you can pass by value cheaply since it's just a reference and is likely to get optimized away.
But the important part is that because that is a wrapper object that holds a reference to inner
rapidjson::Value instances, it cannot bind to lvalue references in for loops like in the example.


### Document and Value
Value is an object that actually manages and holds a data container inside it. You can avoid using
rapidjson types altogether and use garlic providers instead.

Document is similar to *Value* but in some libraries a *Document* manages the allocations.

```cpp
#include <garlic/providers/rapidjson.h>

using garlic::providers::rapidjson::JsonDocument;

int main() {
   JsonDocument doc;  // uses a ::rapidjson::Document internally.
   Config config{"localhost", 4000};
   // we don't use wrappers here. JsonDocument is already a wrapper conforming to garlic::RefLayer.
   write_config(doc, config);
   auto& inner_rapidjson_doc = doc.get_inner_value();  // refrence to the inner ::rapidjson::Document
}
```

There are a very limited set of providers at the moment but the list will grow with time. You can help making them!

> There is an existing data container that one can use without relying on any library. See garlic/clove.h

That might be a good choice with libyaml because it occupies less memory than `yaml_document_t` and it is faster
when getting numbers and boolean values because yaml_document_t only holds strings and everytime you use `get_int()`,
`get_double()` or `get_bool()` a conversion from string to the related type has to happen.


> **NOTE** Take a look at the garlic/utility.h for some utility functions you can use with any type conforming to layer concepts.
