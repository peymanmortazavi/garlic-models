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

  template<typename ValueType, typename IteratorType>
  class IteratorWrapperBase {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = ValueType;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    IteratorWrapperBase() = default;
    explicit IteratorWrapperBase(const IteratorType& iterator)
      : iterator_(iterator) {}
    explicit IteratorWrapperBase(IteratorType&& iterator)
      : iterator_(std::move(iterator)) {}

    IteratorType& get_inner_iterator() { return iterator_; }
    const IteratorType& get_inner_iterator() const { return iterator_; }
    auto GetInnerValue() const { return *this->iterator_; }

  protected:
    IteratorType iterator_;
  };

  template<
    template <typename, typename, typename...> class T,
    typename ValueType,
    typename Iterator,
    typename... Args>
  class IteratorWrapperOperators : public T<ValueType, Iterator, Args...> {
  public:
    using Base = T<ValueType, Iterator, Args...>;
    using Base::Base;

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

    template<typename ExportType>
    using ExportIterator = IteratorWrapperOperators<T, ExportType, Iterator, Args...>;

    template<typename ExportType>
    inline auto Export() const -> ExportIterator<ExportType> {
      return Base::template Export<ExportIterator<ExportType>>();
    }

  };

  template<
    template <typename, typename, typename...> class T,
    typename ValueType,
    typename IteratorType,
    typename... Args>
  using add_iterator_wrapper_operators = IteratorWrapperOperators<
    T, ValueType, IteratorType, Args...
    >;

  namespace internal {
    template<typename ValueType, typename Iterator>
    class IteratorWrapperImpl : public IteratorWrapperBase<ValueType, Iterator> {
    public:
      using IteratorWrapperBase<ValueType, Iterator>::IteratorWrapperBase;

      template<typename ExportType>
      inline auto Export() const -> ExportType {
        return ExportType(this->iterator_);
      }

      ValueType operator * () const {
        return ValueType { *this->iterator_ };
      }
    };

    template<typename ValueType, typename Iterator, typename AllocatorType>
    class AllocatorIteratorWrapperImpl : public IteratorWrapperBase<ValueType, Iterator> {
    public:
      using Base = IteratorWrapperBase<ValueType, Iterator>;

      AllocatorIteratorWrapperImpl() = default;

      AllocatorIteratorWrapperImpl(
          const Iterator& iterator,
          AllocatorType& allocator
          ) : allocator_(&allocator), Base(iterator) {}

      AllocatorIteratorWrapperImpl(
          Iterator&& iterator,
          AllocatorType& allocator
          ) : allocator_(&allocator), Base(std::move(iterator)) {}

      template<typename ExportType>
      inline auto Export() const -> ExportType {
        return ExportType(this->iterator_);
      }

      ValueType operator * () const {
        return ValueType { *this->iterator_, *this->allocator_ };
      }

    protected:
      AllocatorType* allocator_;
    };
  }

  template<typename ValueType, typename Iterator>
  using IteratorWrapper = add_iterator_wrapper_operators<
    internal::IteratorWrapperImpl,
    ValueType,
    Iterator
    >;

  template<typename ValueType, typename Iterator, typename Allocator>
  using AllocatorIteratorWrapper = add_iterator_wrapper_operators<
    internal::AllocatorIteratorWrapperImpl,
    ValueType,
    Iterator,
    Allocator
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

  template<class, class = void>
  struct has_string_view_support : std::false_type {};

  template<typename T>
  struct has_string_view_support<T, 
        std::void_t<decltype(std::declval<T>().get_string_view())>
      > : std::true_type {};

  template<class, class = void>
  struct has_string_support : std::false_type {};

  template<typename T>
  struct has_string_support<T, 
        std::void_t<decltype(std::declval<T>().get_string())>
      > : std::true_type {};

  template<class Bridge>
  class ObjectView : public Bridge {
  public:
    using ConstValueIterator =
      typename Bridge::ConstValueIterator::template ExportIterator<ObjectView>;
    using ConstMemberIterator =
      typename Bridge::ConstMemberIterator::template ExportIterator<ObjectView>;

    using Bridge::Bridge;

    // get_string_view
    template<typename T = Bridge>
    std::enable_if_t<has_string_view_support<T>::value, std::string_view>
    get_string_view() const {
      return Bridge::get_string_view();
    }

    template<typename T = Bridge>
    std::enable_if_t<!has_string_view_support<T>::value, std::string_view>
    get_string_view() const noexcept {
      return std::string_view{this->get_cstr()};
    }

    // get_string
    template<typename T = Bridge>
    std::enable_if_t<has_string_support<T>::value, std::string>
    get_string() const {
      return Bridge::get_string();
    }

    template<typename T = Bridge>
    std::enable_if_t<!has_string_support<T>::value, std::string>
    get_string() const noexcept {
      return std::string{this->get_cstr()};
    }

    // List Methods
    inline ConstValueIterator begin_list() const {
      return Bridge::begin_list().template Export<ObjectView>();
    }

    inline ConstValueIterator end_list() const {
      return Bridge::end_list().template Export<ObjectView>();
    }

    auto get_list() const { return ConstListRange<ObjectView>{*this}; }

    // Member Methods
    inline ConstMemberIterator begin_member() const {
      return Bridge::begin_member().template Export<ObjectView>();
    }

    inline ConstMemberIterator end_member() const {
      return Bridge::end_member().template Export<ObjectView>();
    }

    inline ConstMemberIterator find_member(const char* key) const {
      return Bridge::find_member(key).template Export<ObjectView>();
    }

    inline ConstMemberIterator find_member(std::string_view key) const {
      return Bridge::find_member(key).template Export<ObjectView>();
    }

    auto get_object() const { return ConstMemberRange<ObjectView>{*this}; }
  };

  template<class Bridge>
  class ObjectHelper : public Bridge {
  public:
    using Bridge::Bridge;
    using Bridge::begin_list;
    using Bridge::end_list;
    using Bridge::begin_member;
    using Bridge::end_member;
    using Bridge::find_member;
    using Bridge::get_list;
    using Bridge::get_object;

    using ValueIterator =
      typename Bridge::ValueIterator::template ExportIterator<ObjectHelper>;
    using MemberIterator =
      typename Bridge::MemberIterator::template ExportIterator<ObjectHelper>;

    inline ValueIterator begin_list() {
      return Bridge::begin_list().template Export<ObjectHelper>();
    }

    inline ValueIterator end_list() {
      return Bridge::end_list().template Export<ObjectHelper>();
    }

    auto get_list() { return ListRange<ObjectHelper>{*this}; }

    inline MemberIterator begin_member() {
      return Bridge::begin_member().template Export<ObjectHelper>();
    }

    inline MemberIterator end_member() {
      return Bridge::end_member().template Export<ObjectHelper>();
    }

    inline MemberIterator find_member(const char* key) {
      return Bridge::find_member(key).template Export<ObjectHelper>();
    }

    inline MemberIterator find_member(std::string_view key) {
      return Bridge::find_member(key).template Export<ObjectHelper>();
    }

    auto get_object() { return MemberRange<ObjectHelper>{*this}; }

    ObjectHelper& operator = (double value) { this->set_double(value); return *this; }
    ObjectHelper& operator = (int value) { this->set_int(value); return *this; }
    ObjectHelper& operator = (const char* value) { this->set_string(value); return *this; }
    ObjectHelper& operator = (const std::string& value) { this->set_string(value); return *this; }
    ObjectHelper& operator = (std::string_view value) { this->set_string(value); return *this; }
    ObjectHelper& operator = (bool value) { this->set_bool(value); return *this; }
  };

}

#endif /* end of include guard: GARLIC_LAYER_H */
