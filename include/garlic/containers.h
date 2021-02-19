#ifndef GARLIC_CONTAINERS_H
#define GARLIC_CONTAINERS_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace garlic {

  enum class text_type : uint8_t {
    reference,
    copy,
  };

  template<typename Ch, typename SizeType = unsigned>
  class basic_text {
  public:
    basic_text(const Ch* data, text_type type = text_type::reference) : type_(type) {
      size_ = strlen(data);
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data);
      else
        data_ = data;
    }

    basic_text(const Ch* data, SizeType size, text_type type = text_type::reference) : type_(type), size_(size) {
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), data);
      else
        data_ = data;
    }

    basic_text(
        const std::basic_string<Ch>& value,
        text_type type = text_type::reference) : type_(type), size_(value.size()) {
      if (type == text_type::copy && size_)
        data_ = strcpy((Ch*)malloc((size_ + 1) * sizeof(Ch)), value.data());
      else
        data_ = value.data();
    }

    basic_text(const basic_text&) = delete;  // no copy

    ~basic_text() {
      if (type_ == text_type::copy && size_) std::free((void*)data_);
    }

    const Ch* data() const { return data_; }

    inline const Ch* begin() const { return data_; }
    inline const Ch* end() const { return data_ + size_; }

  private:
    const Ch* data_;
    SizeType size_;
    text_type type_;
  };

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

    ~sequence() {
      if (capacity_)
        free(items_);
    }

    void push_back(value_type&& value) {
      reserve_item();
      items_[size_++] = std::move(value);
    }

    void push_back(const_reference value) {
      reserve_item();
      items_[size_++] = value;
    }

    inline iterator begin() { return items_; }
    inline iterator end() { return items_ + size_; }

    inline const_iterator begin() const { return items_; }
    inline const_iterator end() const { return items_ + size_; }

    inline SizeType size() const noexcept { return size_; }
    inline SizeType capacity() const noexcept { return capacity_; }
    inline bool empty() const noexcept { return !size_; }

  protected:
    pointer items_;
    SizeType capacity_;
    SizeType size_;

  private:
    inline void reserve_item() {
      if (size_ == capacity_) {
        capacity_ += (capacity_ + 1) / 2;
        items_ = reinterpret_cast<pointer>(std::realloc(items_, capacity_ * sizeof(ValueType)));
      }
    }
  };

}

#endif /* end of include guard: GARLIC_CONTAINERS_H */
