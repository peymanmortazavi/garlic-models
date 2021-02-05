#ifndef GARLIC_META_H
#define GARLIC_META_H

#include <cstdint>
#include <string>
#include <type_traits>

namespace garlic {

  struct VoidType {};

  template<typename Left, typename Right>
  using comparable = decltype(std::declval<Left>() == std::declval<Right>());

  template<typename, typename, class = void>
  struct is_comparable : std::false_type{};

  template<typename Left, typename Right>
  struct is_comparable<Left, Right, std::void_t<comparable<Left, Right>>> : std::true_type{};

  template<typename, class = void>
  struct is_std_string : std::false_type {};

  template<typename T>
  struct is_std_string<
    T,
    std::enable_if_t<
      std::is_same<T, std::string>::value ||
      std::is_same<T, std::string_view>::value
      >
    >: std::true_type {};
}

#endif /* end of include guard: GARLIC_META_H */
