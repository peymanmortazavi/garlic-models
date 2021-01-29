#ifndef GARLIC_LAYER_H
#define GARLIC_LAYER_H

#include <concepts>
#include <cstddef>
#include <string>
#include <iterator>


namespace garlic {

  enum TypeFlag : uint8_t {
    Null    = 0x1 << 0,
    Boolean = 0x1 << 1,
    String  = 0x1 << 2,
    Integer = 0x1 << 3,
    Double  = 0x1 << 4,
    Object  = 0x1 << 5,
    List    = 0x1 << 6,
  };

  template<typename T> using ConstValueIteratorOf = typename T::ConstValueIterator;
  template<typename T> using ValueIteratorOf = typename T::ValueIterator;
  template<typename T> using ConstMemberIteratorOf = typename T::ConstMemberIterator;
  template<typename T> using MemberIteratorOf = typename T::MemberIterator;

  template<typename T, typename OfType>
  concept forward_iterator = std::forward_iterator<T> && requires(T it) {
    { *it } -> std::same_as<OfType>;
  };

  template<typename T> concept ViewLayer = std::copy_constructible<T> && requires(const T& t) {
    typename ConstValueIteratorOf<T>;
    typename ConstMemberIteratorOf<T>;

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

    { t.begin_list() } -> forward_iterator<T>;
    { t.end_list() } -> forward_iterator<T>;
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> std::forward_iterator;  // todo: make sure this iterator yields layers.
    { t.end_member() } -> std::forward_iterator;  // todo: same as above.
    { t.find_member(std::declval<const char*>()) } -> std::forward_iterator;
    { t.find_member(std::declval<std::string_view>()) } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;
  };

  template<typename T> concept RefLayer = ViewLayer<T> && requires(T t) {
    typename ValueIteratorOf<T>;
    typename MemberIteratorOf<T>;

    { t.set_string(std::declval<const char*>()) };
    { t.set_string(std::declval<std::string>()) };
    { t.set_string(std::declval<std::string_view>()) };
    { t.set_bool(std::declval<bool>()) };
    { t.set_int(std::declval<int>()) };
    { t.set_double(std::declval<double>()) };
    { t.set_null() };
    { t.set_list() };
    { t.set_object() };

    { t.begin_list() } -> std::forward_iterator;
    { t.end_list() } -> std::forward_iterator;
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> std::forward_iterator;
    { t.end_member() } -> std::forward_iterator;
    { t.find_member(std::declval<const char*>()) } -> std::forward_iterator;
    { t.find_member(std::declval<std::string_view>()) } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;

    /* TODO:  <29-01-21, Peyman> Add a constraint to support pushing T. */
    { t.clear() };
    { t.push_back() };
    { t.push_back(std::declval<const char*>()) };
    { t.push_back(std::declval<bool>()) };
    { t.push_back(std::declval<int>()) };
    { t.push_back(std::declval<double>()) };
    { t.pop_back() };
    { t.erase(std::declval<ValueIteratorOf<T>>()) };
    { t.erase(std::declval<ValueIteratorOf<T>>(), std::declval<ValueIteratorOf<T>>()) };

    { t.add_member(std::declval<const char*>()) };
    { t.add_member(std::declval<const char*>(), std::declval<const char*>()) };
    { t.add_member(std::declval<const char*>(), std::declval<bool>()) };
    { t.add_member(std::declval<const char*>(), std::declval<int>()) };
    { t.add_member(std::declval<const char*>(), std::declval<double>()) };
    { t.remove_member(std::declval<const char*>()) };
    { t.erase_member(std::declval<MemberIteratorOf<T>>()) };
  };

  template<typename T>
  concept IteratorWrapper = requires(T container) {
    typename T::output_type;
    typename T::iterator_type;

    { container.iterator };
    /* TODO:  <28-01-21, Peyman> Figure out why the next line doesn't compile. */
    // { container.wrap() } -> std::same_as<typename T::output_type>;
  };

  template<typename ValueType, typename Iterator>
  struct BasicIteratorWrapper {
    using output_type = ValueType;
    using iterator_type = Iterator;

    iterator_type iterator;

    inline ValueType wrap() const {
      return ValueType { *iterator };
    }
  };

  template<IteratorWrapper Container, typename DifferenceType = std::ptrdiff_t>
  class LayerForwardIterator {
  public:
    using difference_type = DifferenceType;
    using value_type = typename Container::output_type;
    using reference = typename Container::output_type&;
    using pointer = typename Container::output_type*;
    using iterator_category = std::forward_iterator_tag;
    using iterator_type = typename Container::iterator_type;
    using self = LayerForwardIterator;

    LayerForwardIterator() = default;
    explicit LayerForwardIterator(Container container)
      : container_(std::move(container)) {}

    self& operator ++ () {
      ++container_.iterator;
      return *this;
    }

    self operator ++ (int) {
      auto it = *this;
      ++container_.iterator;
      return it;
    }

    bool operator == (const self& other) const { 
      return other.container_.iterator == container_.iterator;
    }

    bool operator != (const self& other) const { return !(other == *this); }

    iterator_type& get_inner_iterator() { return container_.iterator; }
    const iterator_type& get_inner_iterator() const { return container_.iterator; }
    value_type operator * () const { return container_.wrap(); }

  private:
    Container container_;
  };

  template<typename ValueType, typename Iterator>
  using BasicLayerForwardIterator = LayerForwardIterator<
    BasicIteratorWrapper<ValueType, Iterator>
    >;

  template<typename ValueType, typename KeyType = ValueType>
  struct MemberPair {
    KeyType key;
    ValueType value;
  };

  template <typename LayerType>
  struct ConstListRange {
    const LayerType& layer;

    ConstValueIteratorOf<LayerType> begin() const { return layer.begin_list(); }
    ConstValueIteratorOf<LayerType> end() const { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ListRange {
    LayerType& layer;

    ValueIteratorOf<LayerType> begin() { return layer.begin_list(); }
    ValueIteratorOf<LayerType> end() { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ConstMemberRange {
    const LayerType& layer;

    ConstMemberIteratorOf<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIteratorOf<LayerType> end() const { return layer.end_member(); }
  };

  template <typename LayerType>
  struct MemberRange {
    LayerType& layer;

    MemberIteratorOf<LayerType> begin() { return layer.begin_member(); }
    MemberIteratorOf<LayerType> end() { return layer.end_member(); }
  };

}

#endif /* end of include guard: GARLIC_LAYER_H */
