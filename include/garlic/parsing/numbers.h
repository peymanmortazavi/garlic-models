#ifndef GARLIC_PARSING_NUMBERS
#define GARLIC_PARSING_NUMBERS

#include <bits/stdint-uintn.h>
#include <cstddef>


namespace garlic::parsing {

  class NumberStream {
  public:
    NumberStream(const char* data) : data_(data) {}

    const char& Peek() const { return data_[index_]; }
    const char& Take() { return data_[index_++]; }
    bool Consume(const char input) {
      if (Peek() == input) {
        Take();
        return true;
      }
      return false;
    }
    inline bool Exhausted() const {
      return Peek() == '\0';
    }

  protected:
    const char* data_;
    size_t index_ = 0;
  };

  static bool ParseInt(const char* input, int& output) {
    auto is = NumberStream(input);
    bool minus = is.Consume('-');
    output = 0;
    if (minus) {
      while (is.Peek() >= '0' && is.Peek() <= '9') {
        if (output >= 214748364) { // 2^31 = 2147483648
          if (output != 214748364 || is.Peek() > '8') {
            return false;
          }
        }
        output = output * 10 + static_cast<unsigned>(is.Take() - '0');
      }
    }
    else {
      while (is.Peek() >= '0' && is.Peek() <= '9') {
        if (output >= 429496729) { // 2^32 - 1 = 4294967295
          if (output != 429496729 || is.Peek() > '5') {
            return false;
          }
        }
        output = output * 10 + static_cast<unsigned>(is.Take() - '0');
      }
    }
    return is.Exhausted();
  }

  static bool ParseDouble(const char* input, double& output) {
    auto is = NumberStream(input);
    auto minus = is.Consume('-');
    output = 0;
    while (is.Peek() >= '0' && is.Peek() <= '9') {
      output = output * 10 + (is.Take() - '0');
    }
    if (is.Consume('.')) {
      // handle fractions.
    }
    return is.Exhausted();
  }

}

#endif /* end of include guard: GARLIC_PARSING_NUMBERS */
