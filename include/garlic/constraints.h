#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "layer.h"
#include "basic.h"
#include <cstring>
#include <regex>


namespace garlic {

  struct ConstraintResult {
    bool valid;
    std::string invalid_reason;
  };


  template<garlic::ReadableLayer LayerType>
  class Constraint {
  public:
    virtual ConstraintResult test(const LayerType& value) const = 0;
    virtual const std::string& get_name() const noexcept = 0;
    virtual bool skip_constraints() const noexcept = 0;
  };


  template<ReadableLayer LayerType>
  class TypeConstraint : public Constraint<LayerType> {
  public:

    TypeConstraint(TypeFlag required_type, std::string&& name="type_constraint") : flag_(required_type), name_(std::move(name)) {}

    ConstraintResult test(const LayerType& value) const override {
      switch (flag_) {
        case TypeFlag::Null: return {value.is_null(), "Expected null type."};
        case TypeFlag::Boolean: return {value.is_bool(), "Expected boolean."};
        case TypeFlag::Double: return {value.is_double(), "Expected double."};
        case TypeFlag::Integer: return {value.is_int(), "Expected integer."};
        case TypeFlag::String: return {value.is_string(), "Expected string."};
        case TypeFlag::List: return {value.is_list(), "Expected list."};
        case TypeFlag::Object: return {value.is_object(), "Expected object."};
        default: return {false};
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

    ConstraintResult test(const LayerType& value) const override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return {false, "invalid string length."};
        return {true};
      } else if (value.is_double()) {
        if(!(min_ < value.get_double() < max_)) return {false, "invalid double length."};
        return {true};
      } else if (value.is_int()) {
        if(!(min_ < value.get_int() < max_)) return {false, "invalid int length."};
        return {true};
      } else return {true};
    }

    const std::string& get_name() const noexcept override { return name_; }
    bool skip_constraints() const noexcept override { return true; }

  private:
    SizeType min_;
    SizeType max_;
    std::string name_;
  };


  template<ReadableLayer LayerType>
  class RegexConstraint : public Constraint<LayerType> {
  public:

    RegexConstraint(
        std::string&& pattern,
        std::string&& name="regex_constraint"
    ) : pattern_(std::move(pattern)), name_(std::move(name)) {}

    ConstraintResult test(const LayerType& value) const override {
      if (!value.is_string()) return {true};
      if (std::regex_match(value.get_cstr(), pattern_)) { return {true}; }
      else { return {false, "invalid value."}; }
    }

    const std::string& get_name() const noexcept override { return name_; }
    bool skip_constraints() const noexcept override { return true; }

  private:
    std::regex pattern_;
    std::string name_;
  };

}

#endif /* end of include guard: CONSTRAINTS_H */
