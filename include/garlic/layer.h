#ifndef LAYER_H
#define LAYER_H

#include <concepts>
#include <string>
#include <iterator>


namespace garlic {


  template<typename T> using ConstValueIterator = typename T::ConstValueIterator;
  template<typename T> using ValueIterator = typename T::ValueIterator;
  template<typename T> using ConstMemberIterator = typename T::ConstMemberIterator;
  template<typename T> using MemberIterator = typename T::MemberIterator;

  template<typename T> concept ReadableLayer = requires(const T& t) {
    typename ConstValueIterator<T>;
    typename ConstMemberIterator<T>;

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
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> std::forward_iterator;  // todo: make sure this iterator yields layers.
    { t.end_member() } -> std::forward_iterator;  // todo: same as above.
    { t.find_member("") } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;
  };

  template<typename T> concept GarlicLayer = ReadableLayer<T> && requires(T t) {
    { t.set_string("") };
    { t.set_string(std::string{}) };
    { t.set_string(std::string_view{""}) };
    { t.set_bool(true) };
    { t.set_int(25) };
    { t.set_double(0.25) };
    { t = "" };
    { t = std::string{} };
    { t = std::string_view{""} };
    { t = true };
    { t = 25 };
    { t = 0.25 };

    { t.begin_list() } -> std::forward_iterator;
    { t.end_list() } -> std::forward_iterator;
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> std::forward_iterator;
    { t.end_member() } -> std::forward_iterator;
    { t.find_member("") } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;
  };

}

#endif /* end of include guard: LAYER_H */
