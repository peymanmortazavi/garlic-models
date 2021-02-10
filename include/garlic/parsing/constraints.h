#ifndef GARLIC_PARSING_CONSTRAINTS
#define GARLIC_PARSING_CONSTRAINTS

#include <memory>
#include "../constraints.h"
#include "../models.h"
#include "../layer.h"

namespace garlic::parsing {

  template<typename T> using ConstraintPtrOf = std::shared_ptr<Constraint<T>>;

  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_any(const ViewLayer auto& layer, Parser parser) noexcept {
    std::vector<ConstraintPtrOf<Destination>> constraints;
    read_constraints<Destination>(layer, parser, "of", constraints);
    return std::make_shared<AnyConstraint<Destination>>(
        std::move(constraints),
        build_constraint_properties(layer)
    );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_all(const ViewLayer auto& layer, Parser parser) noexcept {
    std::vector<ConstraintPtrOf<Destination>> constraints;
    read_constraints<Destination>(layer, parser, "of", constraints);
    return std::make_shared<AllConstraint<Destination>>(
        std::move(constraints),
        build_constraint_properties(layer),
        get(layer, "hide", true),
        get(layer, "ignore_details", false)
        );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_list(const ViewLayer auto& layer, Parser parser) noexcept {
    ConstraintPtrOf<Destination> constraint;
    read_constraint<Destination>(layer, parser, "of", constraint);
    return std::make_shared<ListConstraint<Destination>>(
        std::move(constraint),
        build_constraint_properties(layer, "list_constraint", "", true),
        get(layer, "ignore_details", false)
        );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_tuple(const ViewLayer auto& layer, Parser parser) noexcept {
    std::vector<ConstraintPtrOf<Destination>> constraints;
    read_constraints<Destination>(layer, parser, "items", constraints);
    return std::make_shared<TupleConstraint<Destination>>(
        std::move(constraints),
        get(layer, "strict", true),
        build_constraint_properties(layer, "tuple_constraint"),
        get(layer, "ignore_details", false)
        );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_range(const ViewLayer auto& layer, Parser parser) noexcept {
    typename RangeConstraint<Destination>::SizeType min;
    typename RangeConstraint<Destination>::SizeType max;
    get_member(layer, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
        });
    get_member(layer, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
        });
    return std::make_shared<RangeConstraint<Destination>>(
        min, max,
        build_constraint_properties(layer, "range_constraint")
        );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_regex(const ViewLayer auto& layer, Parser parser) noexcept {
    return std::make_shared<RegexConstraint<Destination>>(
        get<std::string>(layer, "pattern"),
        build_constraint_properties(layer, "regex_constraint")
        );
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_field(const ViewLayer auto& layer, Parser parser) noexcept {
    using FieldPtr = std::shared_ptr<Field<Destination>>;
    std::shared_ptr<FieldConstraint<Destination>> result;
    get_member(layer, "field", [&result, &layer, &parser](const auto& field) {
        auto ptr = parser.resolve_field_reference(field.get_cstr());
        result = std::make_shared<FieldConstraint<Destination>>(
            std::make_shared<FieldPtr>(ptr),
            build_constraint_properties(layer, "", "", true),
            get(layer, "hide", false),
            get(layer, "ignore_details", false)
            );
        if (!ptr)
          parser.add_field_dependency(field.get_cstr(), result);
        });
    return result;
  }


  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_map(const ViewLayer auto& layer, Parser parser) noexcept {
    ConstraintPtrOf<Destination> key_constraint;
    ConstraintPtrOf<Destination> value_constraint;
    read_constraint<Destination>(layer, parser, "key", key_constraint);
    read_constraint<Destination>(layer, parser, "value", value_constraint);
    return std::make_shared<MapConstraint<Destination>>(
        std::move(key_constraint), std::move(value_constraint),
        build_constraint_properties(layer, "map_constraint"),
        get(layer, "ignore_details", false)
        );
  }

  template<ViewLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_literal(const ViewLayer auto& layer, Parser parser) noexcept {
    auto props = build_constraint_properties(layer, "literal_constraint");
    ConstraintPtrOf<Destination> result;
    get_member(layer, "value", [&props, &result](const auto& item) {
        if (item.is_int()) {
          result = std::make_shared<LiteralConstraint<Destination, int>>(
              std::move(props), item.get_int());
          return;
        }
        else if (item.is_double()) {
          result = std::make_shared<LiteralConstraint<Destination, double>>(
              std::move(props), item.get_double());
          return;
        }
        else if (item.is_bool()) {
          result = std::make_shared<LiteralConstraint<Destination, bool>>(
              std::move(props), item.get_bool());
          return;
        }
        else if (item.is_string()) {
          result = std::make_shared<LiteralConstraint<Destination, std::string>>(
              std::move(props), item.get_string());
          return;
        }
        else if (item.is_null())
          result = std::make_shared<LiteralConstraint<Destination>>(std::move(props));
        });
    if (!result)
      result = std::make_shared<LiteralConstraint<Destination>>(std::move(props));
    return result;
  }



  template<ViewLayer Destination, typename ParserType>
  static void
  read_constraint(
      const ViewLayer auto& layer,
      ParserType& parser,
      const char* name,
      ConstraintPtrOf<Destination>& ptr) {
    get_member(layer, name, [&parser, &ptr](const auto& key) {
        parser.parse_constraint(key, [&ptr](auto&& constraint) {
            ptr = std::move(constraint);
            });
        });
  }

  
  template<ViewLayer Destination, typename ParserType>
  static void
  read_constraints(
      const ViewLayer auto& layer,
      ParserType& parser,
      const char* name,
      std::vector<ConstraintPtrOf<Destination>>& container) {
    get_member(layer, name, [&parser, &container](const auto& items) {
        for (const auto& item : items.get_list()) {
          parser.parse_constraint(item, [&container](auto&& constraint) {
              container.emplace_back(std::move(constraint));
              });
          }
          });
  }

}

#endif /* end of include guard: GARLIC_PARSING_CONSTRAINTS */
