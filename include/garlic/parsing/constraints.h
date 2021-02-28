#ifndef GARLIC_PARSING_CONSTRAINTS
#define GARLIC_PARSING_CONSTRAINTS

#include <memory>
#include "../constraints.h"
#include "../models.h"
#include "../layer.h"

namespace garlic::parsing {

  template<typename T> using ConstraintPtrOf = std::shared_ptr<Constraint<T>>;

  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_any(const Input& layer, Parser parser) noexcept {
    sequence<ConstraintPtrOf<Output>> constraints;
    read_constraints<Output>(layer, parser, "of", std::back_inserter(constraints));
    return std::make_shared<AnyConstraint<Output>>(
        std::move(constraints),
        build_constraint_properties(layer)
    );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_all(const Input& layer, Parser parser) noexcept {
    sequence<ConstraintPtrOf<Output>> constraints;
    read_constraints<Output>(layer, parser, "of", std::back_inserter(constraints));
    return std::make_shared<AllConstraint<Output>>(
        std::move(constraints),
        build_constraint_properties(layer),
        get(layer, "hide", true),
        get(layer, "ignore_details", false)
        );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_list(const Input& layer, Parser parser) noexcept {
    ConstraintPtrOf<Output> constraint;
    read_constraint<Output>(layer, parser, "of", constraint);
    return std::make_shared<ListConstraint<Output>>(
        std::move(constraint),
        build_constraint_properties<true>(layer, "list_constraint"),
        get(layer, "ignore_details", false)
        );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_tuple(const Input& layer, Parser parser) noexcept {
    sequence<ConstraintPtrOf<Output>> constraints;
    read_constraints<Output>(layer, parser, "items", std::back_inserter(constraints));
    return std::make_shared<TupleConstraint<Output>>(
        std::move(constraints),
        get(layer, "strict", true),
        build_constraint_properties(layer, "tuple_constraint"),
        get(layer, "ignore_details", false)
        );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_range(const Input& layer, Parser parser) noexcept {
    using SizeType = typename RangeConstraint<Output>::SizeType;
    SizeType min;
    SizeType max;
    get_member(layer, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
        });
    get_member(layer, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
        });
    return std::make_shared<RangeConstraint<Output>>(
        min, max,
        build_constraint_properties(layer, "range_constraint")
        );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_regex(const Input& layer, Parser parser) noexcept {
    return std::make_shared<RegexConstraint<Output>>(
        get<text>(layer, "pattern"),
        build_constraint_properties(layer, "regex_constraint")
        );
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_field(const Input& layer, Parser parser) noexcept {
    using field_pointer = std::shared_ptr<Field<Output>>;
    using field_constraint = FieldConstraint<Output>;

    std::shared_ptr<field_constraint> result;
    get_member(layer, "field", [&result, &layer, &parser](const auto& field) {
        auto ptr = parser.resolve_field_reference(field.get_cstr());
        result = std::make_shared<field_constraint>(
            std::make_shared<field_pointer>(ptr),
            build_constraint_properties<true>(layer),
            get(layer, "hide", false),
            get(layer, "ignore_details", false)
            );
        if (!ptr)
          parser.add_field_dependency(field.get_cstr(), result);
        });
    return result;
  }


  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_map(const Input& layer, Parser parser) noexcept {
    ConstraintPtrOf<Output> key_constraint;
    ConstraintPtrOf<Output> value_constraint;
    read_constraint<Output>(layer, parser, "key", key_constraint);
    read_constraint<Output>(layer, parser, "value", value_constraint);
    return std::make_shared<MapConstraint<Output>>(
        std::move(key_constraint), std::move(value_constraint),
        build_constraint_properties(layer, "map_constraint"),
        get(layer, "ignore_details", false)
        );
  }

  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename Parser>
  static ConstraintPtrOf<Output>
  parse_literal(const Input& layer, Parser parser) noexcept {
    auto props = build_constraint_properties(layer, "literal_constraint");
    ConstraintPtrOf<Output> result;
    get_member(layer, "value", [&props, &result](const auto& item) {
        if (item.is_int()) {
          result = std::make_shared<LiteralConstraint<Output, int>>(
              std::move(props), item.get_int());
          return;
        }
        else if (item.is_double()) {
          result = std::make_shared<LiteralConstraint<Output, double>>(
              std::move(props), item.get_double());
          return;
        }
        else if (item.is_bool()) {
          result = std::make_shared<LiteralConstraint<Output, bool>>(
              std::move(props), item.get_bool());
          return;
        }
        else if (item.is_string()) {
          result = std::make_shared<LiteralConstraint<Output, std::string>>(
              std::move(props), item.get_string());
          return;
        }
        else if (item.is_null())
          result = std::make_shared<LiteralConstraint<Output>>(std::move(props));
        });
    if (!result)
      return std::make_shared<LiteralConstraint<Output>>(std::move(props));
    return result;
  }



  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename ParserType>
  static void
  read_constraint(
      const Input& layer,
      ParserType& parser,
      const char* name,
      ConstraintPtrOf<Output>& ptr) {
    get_member(layer, name, [&parser, &ptr](const auto& key) {
        parser.parse_constraint(key, [&ptr](auto&& constraint) {
            ptr = std::move(constraint);
            });
        });
  }

  
  template<GARLIC_VIEW Output, GARLIC_VIEW Input, typename ParserType, typename BackInserterIterator>
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

#endif /* end of include guard: GARLIC_PARSING_CONSTRAINTS */
