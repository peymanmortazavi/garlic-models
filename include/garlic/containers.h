#ifndef GARLIC_CONTAINERS_H
#define GARLIC_CONTAINERS_H

/*! @file containers.h
 *  @brief Supporting containers for the garlic model.
 *
 *  @attention The containers defined in this file behave differently than the common containers
 *             in the STL. In order to support the specific needs of the constraitns and modules
 *             they use very specific behavior to increase performance and reduce memory consumption.
 *             It is highly recommended **NOT** to use these containers in your own application unless
 *             you read the code and understand the specific ways they behave.
 *
 *  The code here is documented because they appear in certain types that are public but due to their
 *  limited features and different behavior that other STL containers, it's best to stay away from these
 *  containers.
 */

#include "garlic.h"

namespace garlic {

  enum class text_type : uint8_t {
    reference,
    copy,
  };

  template<typename Ch, typename SizeType = unsigned>
  class basic_string_ref {
  private:
    const Ch* data_;
    SizeType size_;

  public:
    explicit constexpr basic_string_ref(const Ch* data) : data_(data) {
      size_ = strlen(data);
    }

    explicit constexpr basic_string_ref(const Ch* data, SizeType size) : data_(data), size_(size) {}

    explicit constexpr basic_string_ref(
        const std::basic_string_view<Ch>& data) : data_(data.data()), size_(data.size()) {}

    explicit basic_string_ref(
        const std::basic_string<Ch>& data) : data_(data.data()), size_(data.size()) {}

    constexpr basic_string_ref(const basic_string_ref& value) : data_(value.data_), size_(value.size_) {};
    constexpr basic_string_ref(basic_string_ref&& value) : data_(value.data_), size_(value.size_) {};

    constexpr const Ch* data() const { return data_; }
    constexpr SizeType size() const { return size_; }
  };

  using string_ref = basic_string_ref<char>;

  //! A container for storing small strings that can either act as a view or as an owner.
  /*!
   * @code
   * text view = "Some Text";  // only a view.
   * text owner = text::copy("Some Text");  // copies the string.
   * view.is_view();  // true
   * owners.is_view();  // false
   * text clone = view.clone();  // copies the string.
   * clone.is_view();  // false
   * @endcode
   */
  template<typename Ch, typename SizeType = unsigned>
  class basic_text {
  public:
    constexpr basic_text() : size_(0), type_(text_type::reference) {}

    constexpr basic_text(const Ch* data, text_type type = text_type::reference) : type_(type) {
      size_ = strlen(data);
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data);
      else
        data_ = data;
    }

    constexpr basic_text(
        const Ch* data, SizeType size,
        text_type type = text_type::reference) : size_(size), type_(type) {
      if (type == text_type::copy && size_) {
        data_ = strncpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data, size_ + 1);
      }
      else
        data_ = data;
    }

    basic_text(
        const std::basic_string<Ch>& value,
        text_type type = text_type::reference) : basic_text(value.data(), value.size(), type) {}

    constexpr basic_text(
        const std::basic_string_view<Ch>& value,
        text_type type = text_type::reference) : basic_text(value.data(), value.size(), type) {}

    constexpr basic_text(
        const basic_string_ref<Ch>& value,
        text_type type = text_type::reference) : basic_text(value.data(), value.size(), type) {}

    constexpr basic_text(
        const basic_text& other
        ) : data_(other.data_), size_(other.size_), type_(text_type::reference) {};

    basic_text(basic_text&& old) : data_(old.data_), size_(old.size_), type_(old.type_) {
      old.size_ = 0;
    }

    static inline basic_text copy(const Ch* data) { return basic_text(data, text_type::copy); }
    static inline basic_text copy(const Ch* data, SizeType size) { return basic_text(data, size, text_type::copy); }
    static inline basic_text copy(const std::basic_string<Ch>& value) { return basic_text(value, text_type::copy); }
    static inline basic_text copy(const std::basic_string_view<Ch>& value) { return basic_text(value, text_type::copy); }
    static inline basic_text copy(const basic_text& another) {
      return basic_text(another.data(), another.size(), text_type::copy);
    }

    inline constexpr basic_text& operator =(const basic_text& other) {
      destroy();
      data_ = other.data_;
      size_ = other.size_;
      type_ = text_type::reference;
      return *this;
    }

    inline constexpr basic_text& operator =(basic_text&& other) {
      destroy();
      data_ = other.data_;
      size_ = other.size_;
      type_ = other.type_;
      other.size_ = 0;
      return *this;
    }

    inline basic_text clone() const noexcept {
      return basic_text(data_, size_, text_type::copy);
    }
    inline basic_text view() const noexcept {
      return basic_text(data_, size_, text_type::reference);
    }
    inline basic_string_ref<Ch> string_ref() const noexcept {
      return basic_string_ref<Ch>(data_, size_);
    }

    constexpr ~basic_text() { destroy(); }

    inline constexpr bool operator ==(const basic_text& another) const noexcept {
      return (another.data_ == data_ && another.size_ == size_) || !strncmp(data_, another.data_, size_);
    }
    inline bool operator ==(const std::basic_string<Ch>& value) const noexcept {
      return !strncmp(data_, value.data(), size_);
    }
    inline constexpr bool operator ==(const std::basic_string_view<Ch>& value) const noexcept {
      return !strncmp(data_, value.data(), size_);
    }

    const Ch* data() const { return data_; }

    inline void pop_back() noexcept {
      if (size_)
        --size_;
    }

    inline constexpr char back() const { return data_[size_ - 1]; }

    inline constexpr const Ch* begin() const { return data_; }
    inline constexpr const Ch* end() const { return data_ + size_; }

    inline constexpr SizeType size() const noexcept { return size_; }
    inline constexpr bool empty() const noexcept { return !size_; }
    inline constexpr bool is_view() const noexcept { return type_ == text_type::reference; }
    
    inline int compare(const char* value) {
      return strncmp(value, data_, size_);
    }

    inline int compare(const basic_text& value) {
      return strncmp(value.data_, data_, size_);
    }

    constexpr static inline basic_text no_text() noexcept {
      return basic_text("");
    }

    friend inline std::ostream& operator << (std::ostream& output, const basic_text& text) {
      output.write(text.data(), text.size());
      return output;
    }

  private:
    const Ch* data_;
    SizeType size_;
    text_type type_;

    constexpr inline void destroy() noexcept {
      if (type_ == text_type::copy && size_)
        std::free((void*)data_);
    }
  };

  using text = basic_text<char>;

  //! A container to store a list of items, similar to std::vector but far more limited.
  /*! This container is mostly used in constrains and modules where only a small number
   *  of elements are stored so it is designed to work with small counts.
   */
  template<typename ValueType, typename SizeType = unsigned>
  class sequence {
  public:
    using value_type = ValueType;
    using const_reference = const ValueType&;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator = ValueType*;
    using const_iterator = const ValueType*;

    sequence(SizeType initial_capacity = 8) : capacity_(initial_capacity), size_(0) {
      if (capacity_)
        items_ = reinterpret_cast<pointer>(std::malloc(capacity_ * sizeof(ValueType)));
    }

    sequence(std::initializer_list<value_type> list) : sequence(list.size()) {
      push_front(list.begin(), list.end());
    }

    sequence(const sequence&) = delete;
    sequence(sequence&& old) : items_(old.items_), capacity_(old.capacity_), size_(old.size_) {
      old.capacity_ = 0;
    }

    ~sequence() { destroy(); }

    void push_back(value_type&& value) {
      reserve_item();
      new (items_ + size_++) ValueType(std::forward<ValueType>(value));
    }

    void push_back(const_reference value) {
      reserve_item();
      new (items_ + size_++) ValueType(value);
    }

    void push_front(const_iterator begin, const_iterator end) noexcept {
      auto count = end - begin;
      if (count > capacity_ - size_) {
        capacity_ += count;
        items_ = reinterpret_cast<pointer>(std::realloc(items_, capacity_ * sizeof(ValueType)));
      }
      if (size_)
        memmove(items_ + count, items_, count * sizeof(ValueType));
      for (auto i = 0; i < count; ++i) new (items_ + i) ValueType(begin[i]);
      size_ += count;
    }

    inline constexpr sequence& operator =(sequence&& old) {
      destroy();
      size_ = old.size_;
      items_ = old.items_;
      capacity_ = old.capacity_;
      old.capacity_ = 0;
      return *this;
    };

    reference operator [](SizeType index) { return items_[index]; }
    const_reference operator [](SizeType index) const { return items_[index]; }

    //! @return a const iterator that points to the first item in the sequence.
    inline iterator begin() { return items_; }

    //! @return a const iterator that points to the one past the last item in the sequence. 
    inline iterator end() { return items_ + size_; }

    //! @return a iterator that points to the first item in the sequence.
    inline const_iterator begin() const { return items_; }

    //! @return a iterator that points to the one past the last item in the sequence. 
    inline const_iterator end() const { return items_ + size_; }

    //! @return the number of the elements stored in the sequence.
    inline SizeType size() const noexcept { return size_; }

    //! @return the current capacity of the sequence.
    inline SizeType capacity() const noexcept { return capacity_; }

    //! @return whether or not the sequence is empty.
    inline bool empty() const noexcept { return !size_; }

    constexpr static inline sequence no_sequence() noexcept {
      return sequence(0);
    }

  protected:
    pointer items_;
    SizeType capacity_;
    SizeType size_;

  private:
    inline void reserve_item() noexcept {
      if (capacity_ == 0) {
        capacity_ = 8;
        items_ = reinterpret_cast<pointer>(std::malloc(capacity_ * sizeof(ValueType)));
        return;
      }
      if (size_ == capacity_) {
        capacity_ += (capacity_ + 1) / 2;
        items_ = reinterpret_cast<pointer>(std::realloc(items_, capacity_ * sizeof(ValueType)));
      }
    }

    inline void destroy() noexcept {
      if (capacity_) {
        for (auto it = items_; it < (items_ + size_); ++it) it->~ValueType();
        free(items_);
      }
    }
  };

}

namespace std {
  template<typename Ch, typename SizeType>
  struct hash<garlic::basic_text<Ch, SizeType>> {
    size_t operator() (const garlic::basic_text<Ch, SizeType>& value) const {
      return hash<std::string_view>()(std::string_view(value.data(), value.size()));
    }
  };
}

#endif /* end of include guard: GARLIC_CONTAINERS_H */
