#ifndef GARLIC_CONSTRAINTS
#define GARLIC_CONSTRAINTS

#include "layer.h"
#include "utility.h"
#include <cstring>
#include <regex>
#include <string>


namespace garlic {

  struct ConstraintResult {
    bool valid = true;
    std::string name;
    std::string reason;
    std::vector<ConstraintResult> details;
    bool field = false;

    bool is_scalar() const noexcept { return details.size() == 0; }
  };

  struct ConstraintProperties {
    bool fatal = false;  // should stop looking at other constraints.
    std::string name;  // constraint name.
    std::string message;  // custom rejection reason.
  };

  void set_constraint_properties(const ReadableLayer auto& value, ConstraintProperties& props) noexcept {
    get_member(value, "fatal", [&props](const auto& item) { props.fatal = item.get_bool(); });
    get_member(value, "message", [&props](const auto& item) { props.message = item.get_string(); });
    get_member(value, "name", [&props](const auto& item) { props.name = item.get_string(); });
  }

  template<garlic::ReadableLayer LayerType>
  class Constraint {
  public:
    explicit Constraint(ConstraintProperties&& props) : props_(std::move(props)) {}

    virtual ConstraintResult test(const LayerType& value) const noexcept = 0;
    std::string_view get_name() const noexcept { return props_.name; };
    bool skip_constraints() const noexcept { return props_.fatal; };

    auto fail() const noexcept -> ConstraintResult {
      return {false, this->props_.name, this->props_.message};
    }

    auto fail(const char* message) const noexcept -> ConstraintResult {
      if (!this->props_.message.empty()) {
        return ConstraintResult{false, this->props_.name, props_.message};
      } else {
        return ConstraintResult{false, this->props_.name, std::string{message}};
      }
    }

  protected:
    ConstraintProperties props_;
  };


  template<ReadableLayer LayerType>
  class TypeConstraint : public Constraint<LayerType> {
  public:

    TypeConstraint(
        TypeFlag required_type,
        std::string&& name="type_constraint"
    ) : flag_(required_type), Constraint<LayerType>({true, std::move(name)}) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: {
          if (value.is_null()) { return {true}; }
          else return this->fail("Expected null.");
        }
        case TypeFlag::Boolean: {
          if (value.is_bool()) { return {true}; }
          else return this->fail("Expected boolean type.");
        }
        case TypeFlag::Double: {
          if (value.is_double()) { return {true}; }
          else return this->fail("Expected double type.");
        }
        case TypeFlag::Integer: {
          if (value.is_int()) { return {true}; }
          else return this->fail("Expected integer type.");
        }
        case TypeFlag::String: {
          if (value.is_string()) { return {true}; }
          else return this->fail("Expected string type.");
        }
        case TypeFlag::List: {
          if (value.is_list()) { return {true}; }
          else return this->fail("Expected a list.");
        }
        case TypeFlag::Object: {
          if (value.is_object()) { return {true}; }
          else return this->fail("Expected an object.");
        }
        default: return {true};
      }
    }

    private:
      TypeFlag flag_;
  };


  template<ReadableLayer LayerType>
  class RangeConstraint : public Constraint<LayerType> {
  public:
    using SizeType = size_t;

    RangeConstraint(
        SizeType min,
        SizeType max,
        std::string&& name="range_constraint"
    ) : min_(min), max_(max), Constraint<LayerType>({false, std::move(name)}) {}

    RangeConstraint(
        SizeType min,
        SizeType max,
        ConstraintProperties&& props
    ) : min_(min), max_(max), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return this->fail("invalid string length.");
        return {true};
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return this->fail("out of range value.");
        return {true};
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(ivalue > max_ || ivalue < min_) return this->fail("out of range value.");
        return {true};
      } else return {true};
    }

    template<ReadableLayer T, typename Parser>
    static std::shared_ptr<Constraint<LayerType>> parse(const T& value, Parser parser) noexcept {
      SizeType min;
      SizeType max;
      get_member(value, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
      });
      get_member(value, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
      });
      ConstraintProperties props {false, "range_constraint"};
      set_constraint_properties(value, props);
      return std::make_shared<RangeConstraint<LayerType>>(min, max);
    }

  private:
    SizeType min_;
    SizeType max_;
    std::string name_;
  };


  template<ReadableLayer LayerType>
  class RegexConstraint : public Constraint<LayerType> {
  public:

    RegexConstraint(
        std::string pattern,
        std::string name="regex_constraint"
    ) : pattern_(std::move(pattern)), Constraint<LayerType>({false, std::move(name)}) {}

    RegexConstraint(
        std::string pattern,
        ConstraintProperties props
    ) : pattern_(std::move(pattern)), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return {true};
      if (std::regex_match(value.get_cstr(), pattern_)) { return {true}; }
      else { return this->fail("invalid value."); }
    }

    template<ReadableLayer T, typename Parser>
    static std::shared_ptr<Constraint<LayerType>> parse(const T& value, Parser parser) noexcept {
      std::string pattern;
      ConstraintProperties props {false, "regex_constraint"};
      set_constraint_properties(value, props);
      get_member(value, "pattern", [&pattern](const auto& v) { pattern = v.get_string(); });
      return std::make_shared<RegexConstraint<LayerType>>(std::move(pattern), std::move(props));
    }

  private:
    std::regex pattern_;
  };


  template<ReadableLayer LayerType>
  class AnyConstraint : public Constraint<LayerType> {
  public:

  AnyConstraint(
      std::vector<std::shared_ptr<Constraint<LayerType>>> constraints,
      ConstraintProperties props
  ) : constraints_(std::move(constraints)), Constraint<LayerType>(std::move(props)) {}

  ConstraintResult test(const LayerType& value) const noexcept override {
    for(const auto& constraint : constraints_) {
      auto result = constraint->test(value);
      if (result.valid) return {true};
    }
    return this->fail("None of the constraints read this value.");
  }

  template<ReadableLayer Source, typename Parser>
  static std::shared_ptr<Constraint<LayerType>>
  parse(const Source& value, Parser parser) noexcept {
    ConstraintProperties props {false};
    set_constraint_properties(value, props);
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints;
    get_member(
        value, "items", [&value, &constraints, &parser](const auto& items) {
          for(const auto& item : items.get_list()) {
            parser.parse_constraint(item, [&constraints](auto&& constraint) {
                constraints.emplace_back(std::move(constraint));
            });
          }
        }
    );
    return std::make_shared<AnyConstraint>(
        std::move(constraints), std::move(props)
    );
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;
  };


  template<ReadableLayer LayerType>
  class ListConstraint : public Constraint<LayerType> {
  public:

  ListConstraint() : Constraint<LayerType>({false, "list_constraint"}) {}

  ListConstraint(
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints,
    ConstraintProperties props
  ) : constraints_(std::move(constraints)), Constraint<LayerType>(std::move(props)) {}

  ConstraintResult test(const LayerType& value) const noexcept override {
    if (!value.is_list()) return this->fail("Expected a list.");
    int index = 0;
    for (const auto& item : value.get_list()) {
      for(const auto& constraint : constraints_) {
        auto result = constraint->test(item);
        if (!result.valid) {
          auto final_result = this->fail("Invalid value found in the list.");
          final_result.details.push_back({
              false,
              std::to_string(index),
              "invalid value.",
              {std::move(result)},
              true
              });
          return final_result;
        }
      }
      index++;
    }
    return {true};
  }

  template<ReadableLayer Source, typename Parser>
  static std::shared_ptr<Constraint<LayerType>>
  parse(const Source& value, Parser parser) noexcept {
    ConstraintProperties props {false, "list_constraint"};
    set_constraint_properties(value, props);
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints;
    get_member(
        value, "items", [&value, &constraints, &parser](const auto& items) {
          for(const auto& item : items.get_list()) {
            parser.parse_constraint(item, [&constraints](auto&& constraint) {
                constraints.emplace_back(std::move(constraint));
            });
          }
        }
    );
    return std::make_shared<ListConstraint>(
        std::move(constraints), std::move(props)
    );
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;
  };

}

#endif /* end of include guard: GARLIC_CONSTRAINTS */
