#ifndef GARLIC_LAYER_H
#define GARLIC_LAYER_H

/*! \file layer.h
 *  \brief This file contains concepts for C++ 20 along with some helper types to
 *         make it easier to produce iterators for the container wrappers (providers).
 */

#if __cpp_concepts >= 201907L
#include <concepts>
#define GARLIC_USE_CONCEPTS
#define GARLIC_VIEW garlic::ViewLayer
#define GARLIC_REF garlic::RefLayer
#define GARLIC_ITERATOR_WRAPPER garlic::IteratorWrapper
#else
#define GARLIC_VIEW typename
#define GARLIC_REF typename
#define GARLIC_ITERATOR_WRAPPER typename
#endif

#include <cstddef>
#include <string>
#include <iterator>

namespace garlic {

  enum TypeFlag : uint8_t {
    Null    = 0x1 << 1,
    Boolean = 0x1 << 2,
    String  = 0x1 << 3,
    Integer = 0x1 << 4,
    Double  = 0x1 << 5,
    Object  = 0x1 << 6,
    List    = 0x1 << 7,
  };

  template<typename T> using ConstValueIteratorOf = typename std::decay_t<T>::ConstValueIterator;
  template<typename T> using ValueIteratorOf = typename std::decay_t<T>::ValueIterator;
  template<typename T> using ConstMemberIteratorOf = typename std::decay_t<T>::ConstMemberIterator;
  template<typename T> using MemberIteratorOf = typename std::decay_t<T>::MemberIterator;

  /*! \class garlic::ViewLayer
   *  \brief Concept for reading values from containers.
   *
   *  \code
   *  concept ViewLayer {
   *
   *    // Const value iterator. This iterator can yield any type that conforms to garlic::ViewLayer concept.
   *    typename ConstValueIterator;
   *
   *    // Const member iterator. This iterator can yield any type that conforms to garlic::ViewLayer concept.
   *    typename ConstMemberIterator;
   *
   *    bool is_null() const;
   *    bool is_int() const;
   *    bool is_string() const;
   *    bool is_double() const;
   *    bool is_object() const;
   *    bool is_list() const;
   *    bool is_bool() const;
   *
   *    int get_int() const;
   *    const char* get_cstr() const;
   *    std::string get_string() const;
   *    std::string_view get_string_view() const;
   *    double get_double() const;
   *    bool get_bool() const;
   *
   *    ConstValueIterator begin_list() const;
   *    ConstValueIterator end_list() const;
   *    ConstListRange get_list() const;  // must return any object that has begin() and end()
   *
   *    ConstMemberIterator begin_member() const;
   *    ConstMemberIterator end_member() const;
   *    ConstMemberIterator find_member(const char*) const;
   *    ConstMemberIterator find_member(std::string_view) const;
   *    ConstMemberRange get_object() const;  // must return any object that has begin() and end()
   *
   *    T get_view() const;  // must return a garlic::ViewLayer of the current layer without copying its content.
   *  }
   *  \endcode
   */

  /*! \class garlic::RefLayer
   *  \brief Concept for reading from and writing to containers.
   *  \note RefLayer concept requires conforming to garlic::ViewLayer
   *
   *  \code
   *  concept RefLayer {
   *
   *    // Value iterator. This iterator can yield any type that conforms to RefLayer concept.
   *    typename ValueIterator;
   *
   *    // Member iterator. This iterator can yield any type that conforms to RefLayer concept.
   *    typename MemberIterator;
   *
   *    void set_string(const char*);
   *    void set_string(const std::string&);  // optional but recommended to have std::string&& as well.
   *    void set_string(std::string_view);
   *    void set_bool(bool);
   *    void set_int(int);
   *    void set_double(double);
   *    void set_null();
   *    void set_list();
   *    void set_object();
   *
   *    ValueIterator begin_list();
   *    ValueIterator end_list();
   *    ListRange get_list();  // must return any object that has begin() and end()
   *
   *    MemberIterator begin_member();
   *    MemberIterator end_member();
   *    MemberIterator find_member(const char*);
   *    MemberIterator find_member(std::string_view);
   *    MemberRange get_object();  // must return any object that has begin() and end()
   *
   *    void clear();  // clears the list of any elements.
   *    void push_back();  // push a null value.
   *    void push_back(const char*);
   *    void push_back(bool);
   *    void push_back(int);
   *    void push_back(double);
   *
   *    // must prepare a value, pass to the callback function to get setup, then push that value back.
   *    // the argument is a callback method that can take any type that conforms to RefLayer
   *    void push_back_builder(void(*)(T));
   *    void pop_back();
   *    void erase(ValueIterator);
   *    void erase(ValueIterator, ValueIterator);  // erase anything between the two iterators.
   *
   *    void add_member(const char*);  // add a member with the specified key and null as the value.
   *    void add_member(const char*, const char*);
   *    void add_member(const char*, bool);
   *    void add_member(const char*, int);
   *    void add_member(const char*, double);
   *
   *    // must prepare a value, pass to the callback function to get setup, then add that value.
   *    // the last argument is a callback method that can take any type that conforms to RefLayer
   *    void add_member_builder(const char*, void(*)(T));
   *    void remove_member(const char*);
   *    void erase_member(MemberIterator);
   *
   *    T get_reference();  // must return a RefLayer of the current layer without copying its content.
   *  }
   *  \endcode
   */

  /*! \class garlic::IteratorWrapper
   *  \brief Concept for iterator wrapping guidelines.
   *
   *  There are base iterator instances like ForwardIterator and RandomAccessIterator 
   *  that take a IteratorWrapper to provide the code to wrap an iterator's value
   *  with another type to produce a full and complete iterator. This is mostly to avoid
   *  repeated iterator code.
   *
   *  \code
   *  concept IteratorWrapper {
   *
   *    // The type of the resulting wrapped type.
   *    typename output_type;
   *
   *    // The type of the underlying iterator.
   *    typename iterator_type;
   *
   *    iterator_type iterator;  // iterator member.
   *
   *    output_type wrap() const;
   *  }
   *  \endcode
   */

#ifdef GARLIC_USE_CONCEPTS

  template<typename T>
  concept member_pair = requires(T pair) {
    { pair.key };
    { pair.value };
  };

  template<typename T>
  concept forward_pair_iterator = std::forward_iterator<T> && requires(T it) {
    { *it } -> member_pair;
  };

  template<typename T> concept ViewLayer = requires(const T& t) {
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

    { t.get_view() };
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
    { t.push_back_builder(std::declval<void(*)(T)>()) };
    { t.pop_back() };
    { t.erase(std::declval<ValueIteratorOf<T>>()) };
    { t.erase(std::declval<ValueIteratorOf<T>>(), std::declval<ValueIteratorOf<T>>()) };

    { t.add_member(std::declval<const char*>()) };
    { t.add_member(std::declval<const char*>(), std::declval<const char*>()) };
    { t.add_member(std::declval<const char*>(), std::declval<bool>()) };
    { t.add_member(std::declval<const char*>(), std::declval<int>()) };
    { t.add_member(std::declval<const char*>(), std::declval<double>()) };
    { t.add_member_builder(std::declval<const char*>(), std::declval<void(*)(T)>()) };
    { t.remove_member(std::declval<const char*>()) };
    { t.erase_member(std::declval<MemberIteratorOf<T>>()) };

    { t.get_reference() };
  };

  template<typename T>
  concept TypeAbstraction = requires {
    { T::Null };
    { T::Boolean };
    { T::String };
    { T::Number };
    { T::Object };
    { T::List };
  };

  template<typename T>
  concept TypeProvider = requires (const T& value) {
    { T::type_flag } -> TypeAbstraction;
    { value.get_type() } -> std::same_as<typename T::type_flag>;
  };

  template<typename T>
  concept IteratorWrapper = requires(T container) {
    typename T::output_type;
    typename T::iterator_type;

    { container.iterator };
    /* TODO:  <28-01-21, Peyman> Figure out why the next line doesn't compile. */
    // { container.wrap() } -> std::same_as<typename T::output_type>;
  };
#endif

  /*! \brief A trivial and basic iterator wrapper that conforms to the **IteratorWrapper** concept.
   *
   * The result of the *wrap()* method will be **ValueType { *iterator };**
   *
   * \tparam ValueType must be a garlic Layer.
   * \tparam Iterator is the underlying iterator type whose value will be wrapped by the **ValueType**
   */
  template<typename ValueType, typename Iterator>
  struct BasicIteratorWrapper {
    using output_type = ValueType;
    using iterator_type = Iterator;

    iterator_type iterator;

    inline ValueType wrap() const {
      return ValueType { *iterator };
    }
  };

  //! A forward iterator that takes an *IteratorWrapper* as a guide to produce a complete iterator.
  template<GARLIC_ITERATOR_WRAPPER Container, typename DifferenceType = std::ptrdiff_t>
  class ForwardIterator {
  public:
    using difference_type = DifferenceType;
    using value_type = typename Container::output_type;
    using reference = typename Container::output_type&;
    using pointer = typename Container::output_type*;
    using iterator_category = std::forward_iterator_tag;
    using iterator_type = typename Container::iterator_type;
    using self = ForwardIterator;

    ForwardIterator() = default;
    ForwardIterator(Container container)
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

    //! \return a reference to the underlying iterator.
    iterator_type& get_inner_iterator() { return container_.iterator; }

    //! \return a const reference to the underlying iterator.
    const iterator_type& get_inner_iterator() const { return container_.iterator; }

    //! \return a Layer value returned by the supplied *IteratorWrapper*
    inline value_type operator * () const { return container_.wrap(); }

  private:
    Container container_;
  };

  //! A random access iterator that takes an *IteratorWrapper* as a guide to produce a complete iterator.
  template<GARLIC_ITERATOR_WRAPPER Container, typename DifferenceType = std::ptrdiff_t>
  class RandomAccessIterator {
  public:
    using difference_type = DifferenceType;
    using value_type = typename Container::output_type;
    using reference_type = value_type;
    using pointer_type = value_type*;
    using iterator_category = std::random_access_iterator_tag;
    using iterator_type = typename Container::iterator_type;
    using self = RandomAccessIterator;

    RandomAccessIterator() = default;
    RandomAccessIterator(Container container)
      : container_(container) {}

    self& operator ++ () {
      ++container_.iterator;
      return *this;
    }

    self operator ++ (int) {
      auto it = *this;
      ++container_.iterator;
      return it;
    }

    self& operator -- () {
      --container_.iterator;
      return *this;
    }

    self operator -- (int) {
      auto it = *this;
      --container_.iterator;
      return it;
    }

    bool operator == (const self& other) const {
      return container_.iterator == other.container_.iterator;
    }

    bool operator != (const self& other) const {
      return container_.iterator != other.container_.iterator;
    }

    bool operator < (const self& other) const {
      return container_.iterator < other.container_.iterator;
    }

    bool operator > (const self& other) const {
      return container_.iterator > other.container_.iterator;
    }

    bool operator <= (const self& other) const {
      return container_.iterator <= other.container_.iterator;
    }

    bool operator >= (const self& other) const {
      return container_.iterator >= other.container_.iterator;
    }

    self operator + (int value) const {
      auto it = *this;
      it.container_.iterator += value;
      return it;
    }

    self& operator += (int value) {
      container_.iterator += value;
      return *this;
    }

    self operator - (int value) const {
      auto it = *this;
      it.container_.iterator -= value;
      return it;
    }

    self& operator -= (int value) {
      container_.iterator -= value;
      return *this;
    }

    reference_type operator [] (int index) const {
      return *(*this + index);
    }

    //! \return a reference to the underlying iterator.
    iterator_type& get_inner_iterator() { return container_.iterator; }

    //! \return a const reference to the underlying iterator.
    const iterator_type& get_inner_iterator() const {
      return container_.iterator;
    }

    //! \return a Layer value returned by the supplied *IteratorWrapper*
    inline value_type operator * () const {
      return container_.wrap();
    }

    friend inline RandomAccessIterator<Container, DifferenceType>
    operator + (
        int left,
        RandomAccessIterator<Container, DifferenceType> right) {
      return right += left;
    }

    friend inline RandomAccessIterator<Container, DifferenceType>
    operator - (
        int left,
        RandomAccessIterator<Container, DifferenceType> right) {
      return right -= left;
    }

    friend inline DifferenceType
    operator - (
        const RandomAccessIterator<Container, DifferenceType>& left,
        const RandomAccessIterator<Container, DifferenceType>& right) {
      return left.get_inner_iterator() - right.get_inner_iterator();
    }

  private:
    Container container_;
  };

  /*! \brief A generic bask inserter iterator for all garlic Layer types.
   *
   * This is similar to std::back_inserter_iterator but it works on garlic Layer types.
   * Usage is the same exact way as the STL counterpart.
   *
   * \tparam LayerType any type that conforms to the RefLayer concept.
   *
   * \code{.cpp}
   * CloveDocument doc;
   * doc.set_list();
   * int values[] = {1, 2, 3};
   * std::copy(std::begin(values), std::end(values), garlic::back_inserter(doc));
   * \endcode
   */
  template<GARLIC_REF LayerType>
  class back_inserter_iterator {
  public:
    using difference_type = ptrdiff_t;
    using value_type = void;
    using reference_type = void;
    using pointer_type = void;
    using iterator_category = std::output_iterator_tag;
    using container_type = LayerType;

    back_inserter_iterator(LayerType&& layer) : layer_(std::forward<LayerType>(layer)) {}

    template<typename T>
    inline void operator = (T&& value) noexcept {
      layer_.push_back(std::forward<T>(value));
    }

    back_inserter_iterator& operator * () { return *this; }
    back_inserter_iterator& operator ++() { return *this; }
    back_inserter_iterator& operator ++(int) { return *this; }

  protected:
    LayerType layer_;
  };

  //! Helper method to produce garlic back inserter iterators.
  template<GARLIC_REF LayerType>
  static inline back_inserter_iterator<LayerType>
  back_inserter(LayerType&& layer) {
    return back_inserter_iterator<LayerType>(std::forward<LayerType>(layer));
  }

  /*! Very basic forward iterator useful for making wrapper iterators for new providers.
   *
   * \tparam ValueType is the output garlic Layer type.
   * \tparam Iterator is the underlying iterator type whose values get wrapped by the **ValueType**.
   */
  template<typename ValueType, typename Iterator>
  using BasicForwardIterator = ForwardIterator<
    BasicIteratorWrapper<ValueType, Iterator>
    >;

  /*! \brief Very basic random access iterator useful for making wrapper iterators for new providers.
   *
   * \tparam ValueType is the output garlic Layer type.
   * \tparam Iterator is the underlying iterator type whose values get wrapped by the **ValueType**.
   */
  template<typename ValueType, typename Iterator>
  using BasicRandomAccessIterator = RandomAccessIterator<
    BasicIteratorWrapper<ValueType, Iterator>
    >;

  //! Generic struct to hold a pair to be used to store key-value associations in an object.
  template<typename ValueType, typename KeyType = ValueType>
  struct MemberPair {
    KeyType key;
    ValueType value;
  };

  /*! \brief Wrapper struct that can be used in a for-loop range iterating over lists.
   *
   * Since a Layer must contain **begin_list()** and **end_list()** methods, it can't be used in for-loops.
   *
   * \code{.cpp}
   * ConstListRange get_list() const { return ConstListRange<LayerType>(layer); }
   * \endcode
   */
  template <typename LayerType>
  struct ConstListRange {
    const LayerType& layer;

    ConstValueIteratorOf<LayerType> begin() const { return layer.begin_list(); }
    ConstValueIteratorOf<LayerType> end() const { return layer.end_list(); }
  };

  /*! \brief Wrapper struct that can be used in a for-loop range iterating over lists.
   *
   * Since a Layer must contain **begin_list()** and **end_list()** methods, it can't be used in for-loops.
   *
   * \code{.cpp}
   * ListRange get_list() { return ListRange<LayerType>(layer); }
   * \endcode
   */
  template <typename LayerType>
  struct ListRange {
    LayerType& layer;

    ValueIteratorOf<LayerType> begin() { return layer.begin_list(); }
    ValueIteratorOf<LayerType> end() { return layer.end_list(); }
  };

  /*! \brief Wrapper struct that can be used in a for-loop range iterating over an object's members.
   *
   * Since a Layer must contain **begin_member()** and **end_member()** methods, it can't be used in for-loops.
   *
   * \code{.cpp}
   * ConstMemberRange get_member() const { return ConstMemberRange<LayerType>(layer); }
   * \endcode
   */
  template <typename LayerType>
  struct ConstMemberRange {
    const LayerType& layer;

    ConstMemberIteratorOf<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIteratorOf<LayerType> end() const { return layer.end_member(); }
  };

  /*! \brief Wrapper struct that can be used in a for-loop range iterating over an object's members.
   *
   * Since a Layer must contain **begin_member()** and **end_member()** methods, it can't be used in for-loops.
   *
   * \code{.cpp}
   * MemberRange get_member() { return MemberRange<LayerType>(layer); }
   * \endcode
   */
  template <typename LayerType>
  struct MemberRange {
    LayerType& layer;

    MemberIteratorOf<LayerType> begin() { return layer.begin_member(); }
    MemberIteratorOf<LayerType> end() { return layer.end_member(); }
  };

}

#endif /* end of include guard: GARLIC_LAYER_H */
