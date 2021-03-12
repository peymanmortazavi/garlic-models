#ifndef GARLIC_PARSING_FLAT_CONSTRAINTS
#define GARLIC_PARSING_FLAT_CONSTRAINTS

#include <memory>
#include "../constraints.h"
#include "../layer.h"

namespace garlic::parsing {

  template<bool Fatal, typename Tag, GARLIC_VIEW Layer, typename... Args>
  static inline FlatConstraint
  build_constraint(
      Layer&& layer,
      text&& name = text::no_text(),
      Args&&... args) noexcept {
    return make_constraint<Tag>(
        std::forward<Args>(args)...,
        get_text(layer, "name", std::move(name)),
        get_text(layer, "message", text::no_text()),
        get(layer, "fatal", Fatal));
  }

  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_any(const Input& layer, Parser parser) noexcept {
    sequence<FlatConstraint> constraints;
    read_constraints(layer, parser, "of", std::back_inserter(constraints));
    return build_constraint<false, any_tag>(layer, "any_constraint", std::move(constraints));
  }


  template<GARLIC_VIEW Input, typename Parser>
  static inline FlatConstraint
  parse_all(const Input& layer, Parser parser) noexcept {
    sequence<FlatConstraint> constraints;
    read_constraints(layer, parser, "of", std::back_inserter(constraints));
    return build_constraint<false, all_tag>(
        layer, "all_constraint",
        std::move(constraints), get(layer, "hide", true), get(layer, "ignore_details", false));
  }


  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_list(const Input& layer, Parser parser) noexcept {
    FlatConstraint constraint = FlatConstraint::empty();
    read_constraint(layer, parser, "of", constraint);
    return build_constraint<true, list_tag>(
        layer, "list_constraint",
        std::move(constraint), get(layer, "ignore_details", false));
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_tuple(const Input& layer, Parser parser) noexcept {
    sequence<FlatConstraint> constraints;
    read_constraints(layer, parser, "items", std::back_inserter(constraints));
    return build_constraint<false, tuple_tag>(
        layer, "tuple_constraint",
        std::move(constraints), get(layer, "strict", true), get(layer, "ignore_details", false));
  }


  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_range(const Input& layer, Parser parser) noexcept {
    using SizeType = typename range_tag::size_type;
    SizeType min;
    SizeType max;
    get_member(layer, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
        });
    get_member(layer, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
        });
    return build_constraint<false, range_tag>(layer, "range_constraint", std::move(min), std::move(max));
  }


  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_regex(const Input& layer, Parser parser) noexcept {
    return build_constraint<false, regex_tag>(layer, "regex_constraint", get<text>(layer, "pattern"));
  }


  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_field(const Input& layer, Parser parser) noexcept {
    using field_pointer = std::shared_ptr<FlatField>;
    FlatConstraint result = FlatConstraint::empty();

    get_member(layer, "field", [&result, &layer, &parser](const auto& field) {
        auto ptr = parser.find_field(field.get_string_view());
        result = build_constraint<true, field_tag>(
            layer, text::no_text(),
            std::make_shared<field_pointer>(ptr),
            get(layer, "hide", false),
            get(layer, "ignore_details", false));
        if (!ptr)
          parser.add_field_dependency(field.get_string_view(), result);
        });

    return result;
  }


  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_map(const Input& layer, Parser parser) noexcept {
    FlatConstraint key_constraint = FlatConstraint::empty();
    FlatConstraint value_constraint = FlatConstraint::empty();
    read_constraint(layer, parser, "key", key_constraint);
    read_constraint(layer, parser, "value", value_constraint);
    return build_constraint<false, map_tag>(layer, "map_constraint",
        std::move(key_constraint), std::move(value_constraint), get(layer, "ignore_details", false));
  }

  template<GARLIC_VIEW Input, typename Parser>
  static FlatConstraint
  parse_literal(const Input& layer, Parser parser) noexcept {
    FlatConstraint result = FlatConstraint::empty();
    get_member(layer, "value", [&layer, &result](const auto& item) {
        if (item.is_int()) {
          result = build_constraint<false, int_literal_tag>(layer, "literal_constraint", item.get_int());
          return;
        }
        else if (item.is_double()) {
          result = build_constraint<false, double_literal_tag>(layer, "literal_constraint", item.get_double());
          return;
        }
        else if (item.is_bool()) {
          result = build_constraint<false, bool_literal_tag>(layer, "literal_constraint", item.get_bool());
          return;
        }
        else if (item.is_string()) {
          result = build_constraint<false, string_literal_tag>(layer, "literal_constraint", item.get_string());
          return;
        }
        else if (item.is_null())
          result = build_constraint<false, null_literal_tag>(layer, "literal_constraint");
        });
    if (!result)
      return build_constraint<false, null_literal_tag>(layer, "literal_constraint");
    return result;
  }



  template<GARLIC_VIEW Input, typename ParserType>
  static void
  read_constraint(
      const Input& layer, ParserType& parser,
      const char* name, FlatConstraint& ptr) {
    get_member(layer, name, [&parser, &ptr](const auto& key) {
        parser.parse_constraint(key, [&ptr](auto&& constraint) {
            ptr = std::move(constraint);
            });
        });
  }

  
  template<GARLIC_VIEW Input, typename ParserType, typename BackInserterIterator>
  static void
  read_constraints(
      const Input& layer,
      ParserType& parser,
      const char* name,
      BackInserterIterator it) {
    get_member(layer, name, [&parser, &it](const auto& items) {
        for (const auto& item : items.get_list()) {
          parser.parse_constraint(item, [&it](auto&& constraint) {
              it = std::move(constraint);
              });
        }
        });
  }

}

#endif /* end of include guard: GARLIC_PARSING_FLAT_CONSTRAINTS */
