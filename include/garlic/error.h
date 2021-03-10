#ifndef GARLIC_ERROR_H
#define GARLIC_ERROR_H

#include <system_error>
#include <type_traits>

namespace garlic {

  enum class GarlicError {
    Redefinition = 1
  };

  namespace error {

    class GarlicErrorCategory : public std::error_category {
      public:
        const char* name() const noexcept override { return "garlic"; }
        std::string message(int code) const override {
          switch (static_cast<GarlicError>(code)) {
            case GarlicError::Redefinition:
              return "An element with the same identification is already defined. Redifinition is not allowed.";
            default:
              return "unknown";
          }
        }
    };

  }

  inline std::error_code
  make_error_code(garlic::GarlicError error) {
    static const garlic::error::GarlicErrorCategory category{};
    return {static_cast<int>(error), category};
  }


}

namespace std {
  template<>
  struct is_error_code_enum<garlic::GarlicError> : true_type {};
}

#endif /* end of include guard: GARLIC_ERROR_H */
