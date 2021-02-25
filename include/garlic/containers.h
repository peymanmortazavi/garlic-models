#ifndef GARLIC_CONTAINERS_H
#define GARLIC_CONTAINERS_H

#include <cstring>
#include <cstdlib>
#include <memory>

namespace garlic {

  enum class text_type : uint8_t {
    reference,
    copy,
  };

  template<typename Ch, typename SizeType = unsigned>
  class basic_text {
  public:
    basic_text() : size_(0), type_(text_type::reference) {}

    basic_text(const Ch* data, text_type type = text_type::reference) : type_(type) {
      size_ = strlen(data);
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data);
      else
        data_ = data;
    }

    basic_text(
        const Ch* data, SizeType size,
        text_type type = text_type::reference) : size_(size), type_(type) {
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data);
      else
        data_ = data;
    }

    basic_text(
        const std::basic_string<Ch>& value,
        text_type type = text_type::reference) : size_(value.size()), type_(type) {
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), value.data());
      else
        data_ = value.data();
    }

    basic_text(
        const basic_text& other
        ) : data_(other.data_), size_(other.size_), type_(text_type::reference) {};

    basic_text(basic_text&& old) : data_(old.data_), size_(old.size_), type_(old.type_) {
      old.size_ = 0;
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

    basic_text copy() const {
      return basic_text(data_, size_);
    }

    basic_text deep_copy() const {
      return basic_text(data_, size_, text_type::copy);
    }

    ~basic_text() { destroy(); }

    inline bool operator ==(const basic_text& another) const noexcept {
      return (another.data_ == data_ && another.size_ == size_) || !strcmp(data_, another.data_);
    }

    const Ch* data() const { return data_; }

    inline void pop_back() noexcept {
      if (size_)
        --size_;
    }

    inline char back() const { return data_[size_ - 1]; }

    inline const Ch* begin() const { return data_; }
    inline const Ch* end() const { return data_ + size_; }

    inline SizeType size() const noexcept { return size_; }
    inline bool empty() const noexcept { return !size_; }

    constexpr static inline basic_text no_text() noexcept {
      return basic_text("");
    }

  private:
    const Ch* data_;
    SizeType size_;
    text_type type_;

    inline void destroy() noexcept {
      if (type_ == text_type::copy && size_)
        std::free((void*)data_);
    }
  };

  template<typename Ch, typename SizeType>
  static inline std::ostream& operator << (std::ostream& output, const basic_text<Ch, SizeType>& text) {
    output.write(text.data(), text.size());
    return output;
  }

  using text = basic_text<char>;

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

    inline iterator begin() { return items_; }
    inline iterator end() { return items_ + size_; }

    inline const_iterator begin() const { return items_; }
    inline const_iterator end() const { return items_ + size_; }

    inline SizeType size() const noexcept { return size_; }
    inline SizeType capacity() const noexcept { return capacity_; }
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
      return hash<const Ch*>()(value.data());
    }
  };
}

#endif /* end of include guard: GARLIC_CONTAINERS_H */
