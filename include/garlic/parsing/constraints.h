#ifndef GARLIC_PARSING_CONSTRAINTS
#define GARLIC_PARSING_CONSTRAINTS

#include <memory>
#include "../constraints.h"
#include "../models.h"
#include "../layer.h"

namespace garlic::parsing {

  template<typename T> using ConstraintPtrOf = std::shared_ptr<Constraint<T>>;

  template<ReadableLayer Destination, typename Parser>
  static std::shared_ptr<Constraint<Destination>>
  parse_any(const ReadableLayer auto& value, Parser parser) noexcept {
    ConstraintProperties props {false};
    set_constraint_properties(value, props);
    std::vector<ConstraintPtrOf<Destination>> constraints;
    read_constraints<Destination>(value, parser, "of", constraints);
    return std::make_shared<AnyConstraint<Destination>>(
        std::move(constraints), std::move(props)
    );
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_all(const ReadableLayer auto& value, Parser parser) noexcept {
    ConstraintProperties props {false, ""};
    set_constraint_properties(value, props);
    std::vector<ConstraintPtrOf<Destination>> constraints;
    bool hide = true;
    read_constraints<Destination>(value, parser, "of", constraints);
    get_member(value, "hide", [&hide](const auto& result) {
        hide = result.get_bool();
        });
    return std::make_shared<AllConstraint<Destination>>(
        std::move(constraints), std::move(props), hide
        );
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_list(const ReadableLayer auto& value, Parser parser) noexcept {
    ConstraintProperties props {true, "list_constraint"};
    set_constraint_properties(value, props);
    ConstraintPtrOf<Destination> constraint;
    read_constraint<Destination>(value, parser, "of", constraint);
    return std::make_shared<ListConstraint<Destination>>(
        std::move(constraint), std::move(props)
    );
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_tuple(const ReadableLayer auto& value, Parser parser) noexcept {
    ConstraintProperties props {false, "tuple_constraint"};
    set_constraint_properties(value, props);
    std::vector<ConstraintPtrOf<Destination>> constraints;
    read_constraints<Destination>(value, parser, "items", constraints);
    bool strict = true;
    get_member(value, "strict", [&strict](const auto& item) {
        strict = item.get_bool();
    });
    return std::make_shared<TupleConstraint<Destination>>(
        std::move(constraints), strict, std::move(props)
    );
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_range(const ReadableLayer auto& value, Parser parser) noexcept {
    typename RangeConstraint<Destination>::SizeType min;
    typename RangeConstraint<Destination>::SizeType max;
    get_member(value, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
        });
    get_member(value, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
        });
    ConstraintProperties props {false, "range_constraint"};
    set_constraint_properties(value, props);
    return std::make_shared<RangeConstraint<Destination>>(min, max, std::move(props));
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_regex(const ReadableLayer auto& value, Parser parser) noexcept {
    std::string pattern;
    ConstraintProperties props {false, "regex_constraint"};
    set_constraint_properties(value, props);
    get_member(value, "pattern", [&pattern](const auto& v) {
        pattern = v.get_cstr();
        });
    return std::make_shared<RegexConstraint<Destination>>(
        std::move(pattern), std::move(props)
        );
  }


  template<ReadableLayer Destination, typename Parser>
  static ConstraintPtrOf<Destination>
  parse_field(const ReadableLayer auto& value, Parser parser) noexcept {
    using FieldPtr = std::shared_ptr<Field<Destination>>;
    ConstraintProperties props {true};
    set_constraint_properties(value, props);
    std::shared_ptr<FieldConstraint<Destination>> result;
    get_member(value, "field", [&parser, &result, &props](const auto& field) {
        auto ptr = parser.resolve_field_reference(field.get_cstr());
        result = std::make_shared<FieldConstraint<Destination>>(
            std::make_shared<FieldPtr>(ptr), std::move(props)
        );
        if (!ptr) {
          parser.add_field_dependency(field.get_cstr(), result);
        }
    });
    return result;
  }


  template<ReadableLayer Destination, typename Parser>
  static std::shared_ptr<Constraint<Destination>>
  parse_map(const ReadableLayer auto& value, Parser parser) noexcept {
    ConstraintProperties props {false, "map_constraint"};
    set_constraint_properties(value, props);
    ConstraintPtrOf<Destination> key_constraint;
    ConstraintPtrOf<Destination> value_constraint;
    read_constraint<Destination>(value, parser, "key", key_constraint);
    read_constraint<Destination>(value, parser, "value", value_constraint);
    return std::make_shared<MapConstraint<Destination>>(
        std::move(key_constraint), std::move(value_constraint), std::move(props)
        );
  }


  template<ReadableLayer Destination, typename ParserType>
  static void
  read_constraint(
      const ReadableLayer auto& value,
      ParserType& parser,
      const char* name,
      ConstraintPtrOf<Destination>& ptr) {
    get_member(value, name, [&parser, &ptr](const auto& key) {
        parser.parse_constraint(key, [&ptr](auto&& constraint) {
            ptr = std::move(constraint);
            });
       });
  }

  
  template<ReadableLayer Destination, typename ParserType>
  static void
  read_constraints(
      const ReadableLayer auto& value,
      ParserType& parser,
      const char* name,
      std::vector<ConstraintPtrOf<Destination>>& container) {
    get_member(value, name, [&parser, &container](const auto& items) {
        for (const auto& item : items.get_list()) {
          parser.parse_constraint(item, [&container](auto&& constraint) {
              container.emplace_back(std::move(constraint));
              });
          }
        });
  }

}

#endif /* end of include guard: GARLIC_PARSING_CONSTRAINTS */
