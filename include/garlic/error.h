#ifndef GARLIC_ERROR_H
#define GARLIC_ERROR_H

#include <system_error>
#include <type_traits>

namespace garlic {

  enum GarlicError {
    FieldNotFound = 1
  };

  namespace error {

    class GarlicErrorCategory : public std::error_category {
      public:
        const char* name() const noexcept override { return "garlic"; }
        std::string message(int code) const override {
          switch (static_cast<GarlicError>(code)) {
            case GarlicError::FieldNotFound:
              return "The requested field does not exist.";
            default:
              return "unknown";
          }
        }
    };

  }

}

namespace std {
  template<>
  struct is_error_code_enum<garlic::GarlicError> : true_type {};
}

inline std::error_code
make_error_code(garlic::GarlicError error) {
  static garlic::error::GarlicErrorCategory category;
  return {static_cast<int>(error), category};
}

#endif /* end of include guard: GARLIC_ERROR_H */
