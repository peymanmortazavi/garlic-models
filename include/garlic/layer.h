#ifndef LAYER_H
#define LAYER_H

#include <concepts>
#include <string>
#include <iterator>


namespace garlic {

  template<typename T> using DataResult = const T*;
  template<typename T> using ConstValueIterator = typename T::ConstValueIterator;

  template<typename T> concept ReadableLayer = requires(const T& t) {
    typename ConstValueIterator<T>;

    { t.is_null() } -> std::convertible_to<bool>;
    { t.is_int() } -> std::convertible_to<bool>;
    { t.is_string() } -> std::convertible_to<bool>;
    { t.is_double() } -> std::convertible_to<bool>;
    { t.is_object() } -> std::convertible_to<bool>;
    { t.is_list() } -> std::convertible_to<bool>;
    { t.is_bool() } -> std::convertible_to<bool>;

    { t.get_int() } -> std::convertible_to<int>;
    { t.get_cstr() } -> std::convertible_to<const char*>;
    { t.get_string() } -> std::convertible_to<std::string>;
    { t.get_string_view() } -> std::convertible_to<std::string_view>;
    { t.get_double() } -> std::convertible_to<double>;
    { t.get_bool() } -> std::convertible_to<bool>;

    { t.begin_list() } -> std::forward_iterator;  // todo: make sure this iterator yields layers.
    { t.end_list() } -> std::forward_iterator;  // todo: same as above.
  };

}

#endif /* end of include guard: LAYER_H */
