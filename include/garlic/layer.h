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

    { t.begin_list() } -> std::forward_iterator;  // todo: make sure this iterator yields layers.
    { t.end_list() } -> std::forward_iterator;  // todo: same as above.
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
    { t = std::declval<const char*>() };
    { t = std::declval<std::string>() };
    { t = std::declval<std::string_view>() };
    { t = std::declval<bool>() };
    { t = std::declval<int>() };
    { t = std::declval<double>() };

    { t.begin_list() } -> std::forward_iterator;
    { t.end_list() } -> std::forward_iterator;
    { t.get_list() } -> std::ranges::range;

    { t.begin_member() } -> std::forward_iterator;
    { t.end_member() } -> std::forward_iterator;
    { t.find_member(std::declval<const char*>()) } -> std::forward_iterator;
    { t.find_member(std::declval<std::string_view>()) } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;

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

  /* TODO:  Use concepts to restrict the iterator to a forward iterator. */
  template<typename ValueType, typename Iterator>
  class IteratorWrapper {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    IteratorWrapper() = default;
    explicit IteratorWrapper(const Iterator& iterator) : iterator_(iterator) {}
    explicit IteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}

    IteratorWrapper& operator ++ () { ++iterator_; return *this; }
    IteratorWrapper operator ++ (int) { auto it = *this; ++iterator_; return it; }
    bool operator == (const IteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const IteratorWrapper& other) const { return !(other == *this); }

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const { return ValueType{*this->iterator_}; }

  protected:
    Iterator iterator_;
  };

  template<typename ValueType, typename Iterator, typename AllocatorType>
  class AllocatorIteratorWrapper {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    using self = AllocatorIteratorWrapper;

    AllocatorIteratorWrapper() = default;

    AllocatorIteratorWrapper(
        const Iterator& iterator,
        AllocatorType& allocator
        ) : allocator_(&allocator), iterator_(iterator) {}

    AllocatorIteratorWrapper(
        Iterator&& iterator,
        AllocatorType& allocator
        ) : allocator_(&allocator), iterator_(std::move(iterator)) {}

    self& operator ++ () {
      ++this->iterator_; return *this;
    }

    self operator ++ (int) {
      auto it = *this;
      ++this->iterator_;
      return it;
    }

    bool operator == (const self& other) const {
      return other.iterator_ == this->iterator_;
    }

    bool operator != (const self& other) const {
      return !(other == *this);
    }

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const {
      return ValueType{*this->iterator_, *allocator_};
    }

  private:
    Iterator iterator_;
    AllocatorType* allocator_;
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
