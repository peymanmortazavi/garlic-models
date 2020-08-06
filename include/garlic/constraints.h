#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "layer.h"
#include "utility.h"
#include "basic.h"
#include <cstring>
#include <regex>


namespace garlic {

  struct ConstraintResult {
    bool valid;
    std::string_view name;
    std::string reason;
    std::vector<ConstraintResult> details;
    bool field = false;

    bool is_scalar() const noexcept { return details.size() == 0; }
  };

  struct ConstraintProperties {
    bool fatal = false;  // should stop looking at other constraints.
    std::string message;  // custom rejection reason.
    std::string name;  // constraint name.
  };

  void set_constraint_properties(const ReadableLayer auto& value, ConstraintProperties& props) noexcept {
    get_member(value, "fatal", [&props](const auto& item) { props.fatal = item.get_bool(); });
    get_member(value, "message", [&props](const auto& item) { props.message = item.get_string(); });
    get_member(value, "name", [&props](const auto& item) { props.name = item.get_string(); });
  }

  template<garlic::ReadableLayer LayerType>
  class Constraint {
  public:
    virtual ConstraintResult test(const LayerType& value) const noexcept = 0;
    virtual const std::string& get_name() const noexcept = 0;
    virtual bool skip_constraints() const noexcept = 0;
  };


  template<ReadableLayer LayerType>
  class TypeConstraint : public Constraint<LayerType> {
  public:

    TypeConstraint(TypeFlag required_type, std::string&& name="type_constraint") : flag_(required_type), name_(std::move(name)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: {
          if (value.is_null()) { return {true}; }
          else return {false, this->get_name(), "Expected null."};
        }
        case TypeFlag::Boolean: {
          if (value.is_bool()) { return {true}; }
          else return {false, this->get_name(), "Expected boolean type."};
        }
        case TypeFlag::Double: {
          if (value.is_double()) { return {true}; }
          else return {false, this->get_name(), "Expected double type."};
        }
        case TypeFlag::Integer: {
          if (value.is_int()) { return {true}; }
          else return {false, this->get_name(), "Expected integer type."};
        }
        case TypeFlag::String: {
          if (value.is_string()) { return {true}; }
          else return {false, this->get_name(), "Expected string type."};
        }
        case TypeFlag::List: {
          if (value.is_list()) { return {true}; }
          else return {false, this->get_name(), "Expected a list."};
        }
        case TypeFlag::Object: {
          if (value.is_object()) { return {true}; }
          else return {false, this->get_name(), "Expected an object."};
        }
        default: return {true};
      }
    }

    const std::string& get_name() const noexcept override { return name_; }
    bool skip_constraints() const noexcept override { return true; }

    private:
      TypeFlag flag_;
      std::string name_;
  };


  template<ReadableLayer LayerType>
  class RangeConstraint : public Constraint<LayerType> {
  public:
    using SizeType = size_t;

    RangeConstraint(
        SizeType min,
        SizeType max,
        std::string&& name="range_constraint"
    ) : min_(min), max_(max), name_(std::move(name)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return {false, this->get_name(), "invalid string length."};
        return {true};
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return {false, this->get_name(), "invalid double length."};
        return {true};
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(ivalue > max_ || ivalue < min_) return {false, this->get_name(), "invalid int length."};
        return {true};
      } else return {true};
    }

    const std::string& get_name() const noexcept override { return name_; }
    bool skip_constraints() const noexcept override { return true; }

    template<ReadableLayer T>
    static std::shared_ptr<Constraint<LayerType>> parse(const T& value) noexcept {
      SizeType min;
      SizeType max;
      get_member(value, "min", [&min](const auto& v) {
        if (v.is_double()) min = v.get_double(); else min = v.get_int();
      });
      get_member(value, "max", [&max](const auto& v) {
        if (v.is_double()) max = v.get_double(); else max = v.get_int();
      });
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
    ) : pattern_(std::move(pattern)), props_{false, "invalid value.", std::move(name)} {}

    RegexConstraint(
        std::string pattern,
        ConstraintProperties props
    ) : pattern_(std::move(pattern)), props_(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return {true};
      if (std::regex_match(value.get_cstr(), pattern_)) { return {true}; }
      else { return {false, props_.name, props_.message}; }
    }

    const std::string& get_name() const noexcept override { return props_.name; }
    bool skip_constraints() const noexcept override { return props_.fatal; }

    template<ReadableLayer T>
    static std::shared_ptr<Constraint<LayerType>> parse(const T& value) noexcept {
      std::string pattern;
      ConstraintProperties props;
      set_constraint_properties(value, props);
      get_member(value, "pattern", [&pattern](const auto& v) { pattern = v.get_string(); });
      return std::make_shared<RegexConstraint<LayerType>>(std::move(pattern), std::move(props));
    }

  private:
    ConstraintProperties props_;
    std::regex pattern_;
  };

}

#endif /* end of include guard: CONSTRAINTS_H */
