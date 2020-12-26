#ifndef GARLIC_LAYER_H
#define GARLIC_LAYER_H

#include <concepts>
#include <string>
#include <iterator>
#include <type_traits>


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

  template<typename T> using ConstValueIterator = typename T::ConstValueIterator;
  template<typename T> using ValueIterator = typename T::ValueIterator;
  template<typename T> using ConstMemberIterator = typename T::ConstMemberIterator;
  template<typename T> using MemberIterator = typename T::MemberIterator;

  template<typename T> concept ViewLayer = std::copy_constructible<T> && requires(const T& t) {
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
    { t.find_member(std::string_view("")) } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;
  };

  template<typename T> concept RefLayer = ViewLayer<T> && requires(T t) {
    typename ValueIterator<T>;
    typename MemberIterator<T>;

    { t.set_string("") };
    { t.set_string(std::string{}) };
    { t.set_string(std::string_view{""}) };
    { t.set_bool(true) };
    { t.set_int(25) };
    { t.set_double(0.25) };
    { t.set_null() };
    { t.set_list() };
    { t.set_object() };
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
    { t.find_member(std::string_view("")) } -> std::forward_iterator;
    { t.get_object() } -> std::ranges::range;

    { t.clear() };
    { t.push_back() };
    { t.push_back("") };
    { t.push_back(true) };
    { t.push_back(25) };
    { t.push_back(0.25) };
    { t.pop_back() };
    { t.erase(t.begin_list()) };
    { t.erase(t.begin_list(), t.end_list()) };

    { t.add_member("") };
    { t.add_member("", "") };
    { t.add_member("", true) };
    { t.add_member("", 25) };
    { t.add_member("", 0.25) };
    { t.remove_member("") };
    { t.erase_member(t.begin_member()) };
  };

  template<typename ValueType, typename Iterator>
  class ValueIteratorWrapper {
  public:
    using difference_type = int;
    using value_type = ValueType;
    using reference = ValueType&;
    using pointer = ValueType&;
    using iterator_category = std::forward_iterator_tag;

    explicit ValueIteratorWrapper() {}
    explicit ValueIteratorWrapper(const Iterator& iterator) : iterator_(iterator) {}
    explicit ValueIteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}

    ValueIteratorWrapper& operator ++ () { iterator_++; return *this; }
    ValueIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const ValueIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const ValueIteratorWrapper& other) const { return !(other == *this); }

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const { return ValueType{*this->iterator_}; }

  private:
    Iterator iterator_;
  };


  template<typename ValueType, typename Iterator, typename AllocatorType>
  class RefValueIteratorWrapper {
  public:
    using difference_type = int;
    using value_type = ValueType;
    using iterator_category = std::forward_iterator_tag;

    explicit RefValueIteratorWrapper() {}
    explicit RefValueIteratorWrapper(Iterator&& iterator, AllocatorType& allocator) : iterator_(std::move(iterator)), allocator_(&allocator) {}
    explicit RefValueIteratorWrapper(const Iterator& iterator, AllocatorType& allocator) : iterator_(iterator), allocator_(&allocator) {}

    RefValueIteratorWrapper& operator ++ () { iterator_++; return *this; }
    RefValueIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const RefValueIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const RefValueIteratorWrapper& other) const { return !(other == *this); }

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const { return ValueType{*this->iterator_, *allocator_}; }

  private:
    Iterator iterator_;
    AllocatorType* allocator_;
  };


  template <typename LayerType>
  struct ConstListRange {
    const LayerType& layer;

    ConstValueIterator<LayerType> begin() const { return layer.begin_list(); }
    ConstValueIterator<LayerType> end() const { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ListRange {
    LayerType& layer;

    ValueIterator<LayerType> begin() { return layer.begin_list(); }
    ValueIterator<LayerType> end() { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ConstMemberRange {
    const LayerType& layer;

    ConstMemberIterator<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIterator<LayerType> end() const { return layer.end_member(); }
  };

  template <typename LayerType>
  struct MemberRange {
    LayerType& layer;

    MemberIterator<LayerType> begin() { return layer.begin_member(); }
    MemberIterator<LayerType> end() { return layer.end_member(); }
  };

}

#endif /* end of include guard: GARLIC_LAYER_H */
