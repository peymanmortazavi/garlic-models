# Encoding and Decoding

Related file: garlic/encoding.h

One of the important things that is needed when dealing with
data containers is to be able to decode them into other types
and encode custom types to a container either to serialize and
deserialize or for other needs like loading a configuration file
and use to set up internal objects according to them.

There are libraries that offer macros that help with this.
At this time, garlic does **NOT** have any macros that.

Instead it provides means to define how an object can be decoded
and encoded and provide some helper functions to make it easier.

## Encoding

There are two ways, in order of precedence that one can define how
an object can be encoded.

#### 1. Provide an encode function in a specialization of garlic::coder<T> like below:

```cpp
namespace garlic {

    template<>
    struct coder<int> {

        template<GARLIC_REF Layer>
        static void encode(Layer&& layer, int value) {
            // populate the layer here.
        }
    };
}
```

#### 2. static encoder function for custom classes:

```cpp
struct User {
    std::string name;
    int score;

    template<GARLIC_REF Layer>
    static void encode(Layer&& layer, const User& user) {
        layer.set_object();
        layer.add_member("name", user.name);
        layer.add_member("score", user.score);
    }
};
```

> **NOTE** **garlic::encode()** method always prefers the first method (using **garlic::coder<T, Layer>**) over the second one.

Once there is an encoder available, you can use the **garlic::encode()** method.

```cpp
garlic::CloveDocument doc;
User user {"Zombie", 180};
garlic::encode(doc, user);
```

## Decoding

This is very similar to how encoding is done but there are some differences.

There are three difference primary ways, in order of precedence to define how to decode a type:

#### 1. Provide a decode function in a specialization of garlic::coder<T> like below:

```cpp
namespace garlic {
    
    template<>
    struct coder<User> {

        template<GARLIC_VIEW Layer>
        static User decode(Layer&& layer) {
            return User {
                .name = get<std::string>(layer, "name"),
                .score = get<int>(layer, "score)
            };
        }
    };

}
```

#### 2. Provide a static decoder function for custom classes:

```cpp
struct User {
    std::string name;
    int score;
    
    template<typename Layer>
    static User decode(Layer&& layer) {
        return User {
            .name = get<std::string>(layer, "name"),
            .score = get<int>(layer, "score")
        };
    }
};
```

#### 3. Provide a layer constructor in custom classes.

```cpp
struct User {
    std::string name;
    int score;

    template<GARLIC_VIEW Layer>
    User(Layer&& layer) : name(get(layer, "name")), score(get(layer, "score")) {}
};
```

#### Safe Decoding

Certainly, there are times that a class/type cannot be created from a layer because the
layer has missing critical information or has invalid values. To cover this, one can use
garlic::safe_decode() method. This method calls a callback function if and only if a type
is properly decoded from a layer. Similar to garlic::decode() methods, one can define safe_decode
methods using either of these ways in order of precedence.

##### 1. Provide a safe_decode function in a specialization of garlic::coder<T> like below:

```cpp
template<>
struct garlic::coder<User> {
    template<GARLIC_VIEW Layer, typename Callable>
    static void safe_decode(Layer&& layer, Callable&& cb) {
        User user;
        // setup user here.
        // if user is not valid, return to skip calling cb with the user.
        cb(std::move(user));  // if user is valid.
    }
};
```

##### 2. Provide a static safe_decoder function for custom classes:

```cpp
struct User {
    std::string name;
    int score;

    template<typename Layer, typename Callable>
    static void safe_decode(Layer&& layer, Callable&& cb) {
        User user;
        // if we can't create a user from layer, return without calling cb
        cb(std::move(user));  // otherwise call cb with user!
    }
};
```

#### Recommendations

In order to cover various different cases, there are various ways to define how encoding
and decoding should behave for different types. However, there are some recommendations:

##### For custom classes, it's more clean and easy to understand to just provide the static encode and decode functions:

```cpp
struct User {
    template<typename Layer>
    static void encode(Layer&& layer, const User& user);

    template<typename Layer>
    static User decode(Layer&& layer);
};
```

This way, it's easy to read the code and find the encoder and decoders. Defining a layer constructor
might make sense in some scenarios but for small data structures it'll force you to define other
constructors as well so you'd have to explicitly define move and copy constructors as well.
If this is not possible because you already have this method reserved for some other function, use the
`garlic::coder<T>` specialization.

##### Use GARLIC_VIEW and GARLIC_REF instead of typename:

These macros end up being garlic::ViewLayer and garlic::RefLayer when concepts are available
and *typename* otherwise. Some language servers can provide auto-completion for these concepts
and you'll get more clear error messages if things go wrong.

##### Be careful using garlic::get()

`garlic::get()` method is a great utility but it makes a lot of assumptions about the layer that
may not be true. Use them if you are certain there exists a key and it can be decoded.

If there is doubt, use the safe methods like `garlic::safe_get()` and use defaults or handle
failures properly.

```cpp
struct User {
    std::string name;
    int score;
  
    template<typename Layer, typename Callback>
    static void safe_decode(Layer&& layer, Callback&& cb) {
      User user {
        .name = safe_get(layer, "name", ""),
        .score = safe_get(layer, "score", 0)
      };
      if (user.name.empty())  // invalid User!
        return;
      cb(std::move(user));
    }
};
```

Ideal is to provide both safe_decode and decode methods so the proper method can be used
in different contexts. For instance, if you validate a config file and know an object
can be decoded, no need to use safe methods and benefit from the faster but unsafe methods.
