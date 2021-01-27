#ifndef GARLIC_LAYER_H
#define GARLIC_LAYER_H

#include <concepts>
#include <cstddef>
#include <string>
#include <iterator>
#include <type_traits>
#include <utility>


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

  template<typename ValueType, typename KeyType = ValueType>
  struct MemberWrapper {
    KeyType key;
    ValueType value;
  };

  namespace internal {
    template<typename ValueType, typename Iterator>
    class IteratorWrapperImpl {
    public:

      IteratorWrapperImpl() = default;
      IteratorWrapperImpl(Iterator& iterator) : iterator_(iterator) {}
      IteratorWrapperImpl(const Iterator& iterator) : iterator_(iterator) {}

      ValueType operator * () const {
        return ValueType { *iterator_ };
      }

    protected:
      Iterator iterator_;
    };

    template<typename T>
    class IteratorWrapperOperators : public T {
    public:
      using T::T;

      IteratorWrapperOperators& operator ++ () {
        ++this->iterator_;
        return *this;
      }

      IteratorWrapperOperators operator ++ (int) {
        auto it = *this;
        ++this->iterator_;
        return it;
      }

      bool operator == (const IteratorWrapperOperators& other) const {
        return other.iterator_ == this->iterator_;
      }

      bool operator != (const IteratorWrapperOperators& other) const {
        return !(other == *this);
      }

      auto& get_inner_iterator() { return this->iterator_; }
      const auto& get_inner_iterator() const { return this->iterator_; }
      auto GetInnerValue() const { return *this->iterator_; }
    };

    template<typename ValueType, typename Iterator>
    class LayerMemberIteratorImpl
      : public IteratorWrapperImpl<MemberWrapper<ValueType>, Iterator> {
    public:
      using IteratorWrapperImpl<MemberWrapper<ValueType>, Iterator>::IteratorWrapperBase;

      MemberWrapper<ValueType> operator * () const {
        return MemberWrapper<ValueType> {
          ValueType{ (*this->iterator_).key },
          ValueType{ (*this->iterator_).value }
        };
      }
    };

    template<typename ValueType, typename Iterator, typename AllocatorType>
    class AllocatorIteratorWrapperImpl : public IteratorWrapperImpl<ValueType, Iterator> {
    public:
      using Base = IteratorWrapperImpl<ValueType, Iterator>;

      AllocatorIteratorWrapperImpl() = default;

      AllocatorIteratorWrapperImpl(
          const Iterator& iterator,
          AllocatorType& allocator
          ) : allocator_(&allocator), Base(iterator) {}

      AllocatorIteratorWrapperImpl(
          Iterator&& iterator,
          AllocatorType& allocator
          ) : allocator_(&allocator), Base(std::move(iterator)) {}

      ValueType operator * () const {
        return ValueType { *this->iterator_, *this->allocator_ };
      }

    protected:
      AllocatorType* allocator_;
    };
  }

  template<typename T>
  using add_iterator_operators = internal::IteratorWrapperOperators<T>;

  template<typename ValueType, typename Iterator>
  using IteratorWrapper = add_iterator_operators<
    internal::IteratorWrapperImpl<ValueType, Iterator>
    >;

  template<typename ValueType, typename Iterator>
  using LayerMemberIterator = add_iterator_operators<
    internal::LayerMemberIteratorImpl<ValueType, Iterator>
  >;

  template<typename ValueType, typename Iterator, typename Allocator>
  using AllocatorIteratorWrapper = add_iterator_operators<
    internal::AllocatorIteratorWrapperImpl<ValueType, Iterator, Allocator>
    >;

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
