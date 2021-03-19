# Validation

This is the part of the library that allows validating a data container is valid!

## Why not use JSON Schema?

There is actually a plan to digest JSON Schema as well, this library just provides a way
to define constraints and enforce them. So one could build a JSON schema parser that
makes these constraints and then we could test the validity of any data container.

This library focuses on working with [garlic layers](docs/pages/Layer.md) for validation
meaning you can test against any type that conforms to garlic::ViewLayer and that could
include std::vector, RapidJSON types and your own custom types so long as you make wrappers for them.

## Constraint

A constraint is the smallest unit of validation, a constraint should take a layer and say if it is valid
or not. It has two primary methods.

* **garlic::Constraint::quick_test()** which is to run as fast as possible and only return a boolean value. `true` means valid, `false` means invalid.
* **garlic::Constraint::test()** which is to return a garlic::ConstraintResult that can provide more details including a message.

There are finite set of constraints and there is documentation for them in garlic/constraints.h

```cpp
auto regex = garlic::make_constraint<garlic::regex_tag>("\\d{1,3});
```

While this may seem like a clunky way to create these constraints, using the documentation and knowing the convention
helps a lot to make it easier to work with these.

Every constraint has a `name`, `message` in case it fails and whether or not it is `fatal`. It just means if there are three
constraints next to each other, if one of them fails, we break the loop. Otherwise, you continue despite the failure.
This will make more sense as you read through this documentation.

> **NOTE:** Every call to garlic::make_constraint() can **optionally** end with `name`, `custom message` and `fatal` args.

```cpp
auto age = make_constraint<regex_tag>("\\d{1,3}", "age_constraint", "Invalid Age!", true);
```

You can then use a constraint to validate any type conforming to garlic::ViewLayer

```cpp
CloveDocument clove;
JsonDocument json;

age.quick_test(clove);
age.quick_test(json);

if (auto result = age.test(clove); !result) {
  std::cout << result.reason << std::endl;
}
...
```

\note a garlic::Constraint is like a std::shared_ptr, copying it only copies a pointer to an inner context.

garlic::Constraint does not have any constructor, you instead need to either use garlic::Constraint::make
or use a more convenient shortcut garlic::make_constraint().

If you need to make an empty (null context really) constraint, you can use garlic::Constraint::empty()
but be careful, passing an empty constraint is like passing a nullptr, it can cause crashes if not handled properly.

```cpp
auto constraint = garlic::Constraint::empty();
if (...) {  // using the empty() method is really to cover cases like this.
  constraint = garlic::make_constraint<garlic::range_tag>(10, 25);
}
if (constraint)  // check if constraint is initialized
  auto result = constraint.quick_test(...);
```

## Field

A garlic::Field is just a group of constraints stored in a single entity with meta data and some properties.

```cpp
auto user_name = garlic::make_field("User Name");
user_name->add_constraint(garlic::make_constraint<garlic::regex_tag>("\\w{3,12}"));
user_name->add_constraint<garlic::regex_tag>("\\w{3,12}");  // more convenient

auto strict_user_name = garlic::make_field("Strict User Name", {... list of constraints...});
strict_user_name->inherit_constraints_from(user_name);
```

Fields can have meta data which is basically a dictionary mapping strings to strings.
```cpp
user_name->meta()["message"] = "Custom message when a field fails!";
user_name->meta()["db/type] = "VARCHAR";
```

That meta data is meant to be used to store arbitrary information so it can be used for
future tools to generate other formats like making migration scripts, or build html forms.

If you set a field to ignore details, it'll return a leaf garlic::ConstraintResult without
any details from its inner constraints. This is helpful when you want to use a Field as a
constraint that is made of other constraints. For instance, you can pack a bunch of constraints
that together validate whether a user name is valid not, but the resulting constraint should only
say "invalid username.".

```cpp
user_name->set_ignore_details(true);  // default is false.
```

## Model

A garlic::Model is a way to describe an object's structure. It is essentially a mapping of keys to a field descriptor object.

`key -> { required: bool, field: std::shared_ptr<garlic::Field> }`

For example, imagine a User object that must have a username and a phone number but phone is optional.

```cpp
auto user_model = garlic::make_model("User");
user_model->add_field("username", user_name);  // user_name from previous section.
user_model->add_field("phone",
                      garlic::make_field({garlic::make_constraint<garlic::regex_tag>("regex pattern")}),
                      false);  // false here indicates the field is NOT required.

CloveDocument doc;  // some arbitrary document.
auto result = user_model->validate(doc);
```

### Model and Field Tags
