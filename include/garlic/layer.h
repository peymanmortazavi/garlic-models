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

  template<typename T>
  concept member_pair = requires(T pair) {
    { pair.key };
    { pair.value };
  };

  template<typename T>
  concept forward_pair_iterator = std::forward_iterator<T> && requires(T it) {
    { *it } -> member_pair;
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

    { t.begin_list() } -> std::forward_iterator;
    { t.end_list() } -> std::forward_iterator;
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> forward_pair_iterator;
    { t.end_member() } -> forward_pair_iterator;
    { t.find_member(std::declval<const char*>()) } -> forward_pair_iterator;
    { t.find_member(std::declval<std::string_view>()) } -> forward_pair_iterator;
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

    { t.begin_member() } -> forward_pair_iterator;
    { t.end_member() } -> forward_pair_iterator;
    { t.find_member(std::declval<const char*>()) } -> forward_pair_iterator;
    { t.find_member(std::declval<std::string_view>()) } -> forward_pair_iterator;
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
    LayerForwardIterator(Container container)
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
    inline value_type operator * () const { return container_.wrap(); }

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
    LayerType layer;

    ConstMemberIteratorOf<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIteratorOf<LayerType> end() const { return layer.end_member(); }
  };

  template <typename LayerType>
  struct MemberRange {
    LayerType layer;

    MemberIteratorOf<LayerType> begin() { return layer.begin_member(); }
    MemberIteratorOf<LayerType> end() { return layer.end_member(); }
  };

  template<typename ValueType, typename Iterator>
  struct LayerMemberIteratorWrapper {
    using output_type = MemberPair<ValueType>;
    using iterator_type = Iterator;
    iterator_type iterator;
    inline output_type wrap() const {
      return output_type {
        ValueType{ (*iterator).key },
        ValueType{ (*iterator).value },
      };
    }
  };

  template<ViewLayer Layer>
  struct object_view {
    Layer layer;

    using ConstValueIterator = BasicLayerForwardIterator<
      object_view, ConstValueIteratorOf<Layer>>;
    using ConstMemberIterator = LayerForwardIterator<
      LayerMemberIteratorWrapper<object_view, ConstMemberIteratorOf<Layer>>>;

    ConstValueIterator
    begin_list() const { return ConstValueIterator({layer.begin_list()}); }

    ConstValueIterator
    end_list() const { return ConstValueIterator({layer.end_list()}); }

    ConstListRange<object_view>
    get_list() const { return ConstListRange<object_view>(*this); }

    ConstMemberIterator
    begin_member() const { return ConstMemberIterator({layer.begin_member()}); }

    ConstMemberIterator
    end_member() const { return ConstMemberIterator({layer.end_member()}); }

    ConstMemberIterator
    find_member(const char* key) const {
      return ConstMemberIterator({layer.find_member(key) });
    }

    ConstMemberIterator
    find_member(std::string_view key) const {
      return ConstMemberIterator({layer.find_member(key)});
    }

    ConstMemberRange<object_view>
    get_object() const { return ConstMemberRange<object_view>(*this); }

    bool is_null() const noexcept { return layer.is_null(); }
    bool is_int() const noexcept { return layer.is_int(); }
    bool is_string() const noexcept { return layer.is_string(); }
    bool is_double() const noexcept { return layer.is_double(); }
    bool is_object() const noexcept { return layer.is_object(); }
    bool is_list() const noexcept { return layer.is_list(); }
    bool is_bool() const noexcept { return layer.is_bool(); }

    int
    get_int() const noexcept { return layer.get_int(); }

    std::string
    get_string() const noexcept { return layer.get_string(); }

    std::string_view
    get_string_view() const noexcept { return layer.get_string_view(); }

    const char*
    get_cstr() const noexcept { return layer.get_cstr(); }

    double
    get_double() const noexcept { return layer.get_double(); }

    bool
    get_bool() const noexcept { return layer.get_bool(); }
  };

  template<RefLayer Layer, ViewLayer V = Layer>
  struct object : public object_view<V> {
    Layer layer_;

    object(Layer layer) : layer_(layer), object_view<V>(layer.get_view()) {}

    using ValueIterator = BasicLayerForwardIterator<
      object, ValueIteratorOf<Layer>>;
    using MemberIterator = LayerForwardIterator<
      LayerMemberIteratorWrapper<object, MemberIteratorOf<Layer>>>;
    using object_view<V>::begin_member;
    using object_view<V>::end_member;
    using object_view<V>::find_member;
    using object_view<V>::begin_list;
    using object_view<V>::end_list;
    using object_view<V>::get_list;
    using object_view<V>::get_object;

    inline void set_string(const char* value) { layer_.set_string(value); }
    inline void set_string(const std::string& value) { layer_.set_string(value); }
    inline void set_string(std::string_view value) { layer_.set_string(value); }
    inline void set_bool(bool value) { layer_.set_bool(value); }
    inline void set_int(int value) { layer_.set_int(value); }
    inline void set_double(double value) { layer_.set_double(value); }
    inline void set_null() { layer_.set_null(); }
    inline void set_list() { layer_.set_list(); }
    inline void set_object() { layer_.set_object(); }

    inline ValueIterator
    begin_list() { return ValueIterator({layer_.begin_list()}); }

    inline ValueIterator
    end_list() { return ValueIterator({layer_.end_list()}); }

    inline ListRange<object>
    get_list() { return ListRange<object>(*this); }

    inline MemberIterator
    begin_member() { return MemberIterator({layer_.begin_member()}); }

    inline MemberIterator
    end_member() { return MemberIterator({layer_.end_member()}); }

    inline MemberIterator
    find_member(const char* key) {
      return MemberIterator({layer_.find_member(key)});
    }

    inline MemberIterator
    find_member(std::string_view key) {
      return MemberIterator({layer_.find_member(key)});
    }

    inline MemberRange<object>
    get_object() { return MemberRange<object>(*this); }

    inline void clear() { layer_.clear(); }
    inline void push_back() { layer_.push_back(); }
    inline void push_back(const char* value) { layer_.push_back(value); }
    inline void push_back(bool value) { layer_.push_back(value); }
    inline void push_back(int value) { layer_.push_back(value); }
    inline void push_back(double value) { layer_.push_back(value); }
    inline void pop_back() { layer_.pop_back(); }
    inline void erase(ValueIterator position) {
      layer_.erase(position.get_inner_iterator());
    }
    inline void erase(ValueIterator start, ValueIterator end) {
      layer_.erase(start.get_inner_iterator(), end.get_inner_iterator());
    }

    inline void add_member(const char* key) { layer_.add_member(key); }
    inline void add_member(const char* key, const char* value) {
      layer_.add_member(key, value);
    }
    inline void add_member(const char* key, bool value) {
      layer_.add_member(key, value);
    }
    inline void add_member(const char* key, int value) {
      layer_.add_member(key, value);
    }
    inline void add_member(const char* key, double value) {
      layer_.add_member(key, value);
    }
    inline void remove_member(const char* key) {
      layer_.remove_member(key);
    }
    inline void erase_member(ValueIterator position) {
      layer_.erase_member(position.get_inner_iterator());
    }
  };

}

#endif /* end of include guard: GARLIC_LAYER_H */
