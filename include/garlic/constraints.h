#ifndef GARLIC_CONSTRAINTS
#define GARLIC_CONSTRAINTS

#include "layer.h"
#include "utility.h"
#include <cstring>
#include <memory>
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
    bool leaf = false;  // should avoid storing details.
    bool fatal = false;  // should stop looking at other constraints.
    std::string name;  // constraint name.
    std::string message;  // custom rejection reason.
  };


  template<typename ConstraintPtrType>
  void test_constraints(
      const ViewLayer auto& value,
      const std::vector<ConstraintPtrType>& constraints,
      std::vector<ConstraintResult>& results) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.valid) {
        results.push_back(std::move(result));
        if (constraint->skip_constraints()) break;
      }
    }
  }


  template<typename ConstraintPtrType>
  bool test_constraints_concise(
      const ViewLayer auto& value,
      const std::vector<ConstraintPtrType>& constraints) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value, false); !result.valid) {
        return false;
      }
    }
    return true;
  }


  template<typename ConstraintPtrType, typename Callback>
  void test_constraints_first_failure(
      const ViewLayer auto& value,
      std::vector<ConstraintPtrType> constraints,
      const Callback& cb
      ) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.valid) {
        cb(std::move(result));
        return;
      }
    }
  }


  void set_constraint_properties(const ViewLayer auto& value, ConstraintProperties& props) noexcept {
    get_member(value, "leaf", [&props](const auto& item) { props.leaf = item.get_bool(); });
    get_member(value, "fatal", [&props](const auto& item) { props.fatal = item.get_bool(); });
    get_member(value, "message", [&props](const auto& item) { props.message = item.get_cstr(); });
    get_member(value, "name", [&props](const auto& item) { props.name = item.get_cstr(); });
  }


  template<garlic::ViewLayer LayerType>
  class Constraint {
  public:
    explicit Constraint(ConstraintProperties&& props) : props_(std::move(props)) {}

    virtual ConstraintResult test(const LayerType& value, bool concise = false) const noexcept = 0;
    std::string_view get_name() const noexcept { return props_.name; };
    bool skip_constraints() const noexcept { return props_.fatal; };

    auto fail(bool concise = false, bool field = false) const noexcept -> ConstraintResult {
      if (concise) return ConstraintResult { .valid = false };
      return ConstraintResult{
        .valid = false,
        .name = this->props_.name,
        .reason = this->props_.message,
        .field = field
      };
    }

    auto fail(const char* message, bool concise = false, bool field=false) const noexcept -> ConstraintResult {
      if (concise) return ConstraintResult { .valid = false };
      if (!this->props_.message.empty()) {
        return ConstraintResult{
          .valid = false,
          .name = this->props_.name,
          .reason = props_.message,
          .field = field
        };
      } else {
        return ConstraintResult{
          .valid = false,
          .name = this->props_.name,
          .reason = message,
          .field = field
        };
      }
    }

    auto fail(const char* message, std::vector<ConstraintResult>&& details, bool concise = false, bool field=false) const noexcept {
      // if stop/concise is needed, then only set the valid state and return immediately.
      // what's needed is an easy interface so that the future constraints can easily hand
      // this concise logic to these fail methods instead of implementing a custom way every
      // single time.
      // using concise should automatically enable fatality as well, because we won't be
      // showing any other constraints.
      if (concise) return ConstraintResult { .valid = false };
      if (!this->props_.message.empty()) {
        return ConstraintResult {
          .valid = false,
          .name = this->props_.name,
          .reason = props_.message,
          .details = (props_.leaf ? std::vector<ConstraintResult>{} : std::move(details)),
          .field = field,
        };
      } else {
        return ConstraintResult {
          .valid = false,
          .name = this->props_.name,
          .reason = message,
          .details = (props_.leaf ? std::vector<ConstraintResult>{} : std::move(details)),
          .field = field
        };
      }
    }

    auto ok() const noexcept -> ConstraintResult {
      return ConstraintResult{ .valid = true };
    }

  protected:
    ConstraintProperties props_;
  };


  template<ViewLayer LayerType>
  class TypeConstraint : public Constraint<LayerType> {
  public:

    TypeConstraint(
        TypeFlag required_type,
        std::string&& name="type_constraint"
    ) : flag_(required_type), Constraint<LayerType>({
      .leaf = false, .fatal = true, .name = std::move(name)
      }) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: {
          if (value.is_null()) { return this->ok(); }
          else return this->fail("Expected null.", concise);
        }
        case TypeFlag::Boolean: {
          if (value.is_bool()) { return this->ok(); }
          else return this->fail("Expected boolean type.", concise);
        }
        case TypeFlag::Double: {
          if (value.is_double()) { return this->ok(); }
          else return this->fail("Expected double type.", concise);
        }
        case TypeFlag::Integer: {
          if (value.is_int()) { return this->ok(); }
          else return this->fail("Expected integer type.", concise);
        }
        case TypeFlag::String: {
          if (value.is_string()) { return this->ok(); }
          else return this->fail("Expected string type.", concise);
        }
        case TypeFlag::List: {
          if (value.is_list()) { return this->ok(); }
          else return this->fail("Expected a list.", concise);
        }
        case TypeFlag::Object: {
          if (value.is_object()) { return this->ok(); }
          else return this->fail("Expected an object.", concise);
        }
        default: return this->ok();
      }
    }

    private:
      TypeFlag flag_;
  };


  template<ViewLayer LayerType>
  class RangeConstraint : public Constraint<LayerType> {
  public:
    using SizeType = size_t;

    RangeConstraint(
        SizeType min,
        SizeType max,
        std::string&& name="range_constraint"
    ) : min_(min), max_(max), Constraint<LayerType>({
      .leaf = false, .fatal = false, .name = std::move(name)
      }) {}

    RangeConstraint(
        SizeType min,
        SizeType max,
        ConstraintProperties&& props
    ) : min_(min), max_(max), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return this->fail("invalid string length.", concise);
        return this->ok();
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return this->fail("out of range value.", concise);
        return this->ok();
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(ivalue > max_ || ivalue < min_) return this->fail("out of range value.", concise);
        return this->ok();
      } else if (value.is_list()) {
        int count = 0;
        for (const auto& item : value.get_list()) {
          count++;
          if (count > max_) return this->fail("too many items in the list.", concise);
        }
        if (count < min_) return this->fail("too few items in the list.", concise);
        return this->ok();
      } else return this->ok();
    }

  private:
    SizeType min_;
    SizeType max_;
    std::string name_;
  };


  template<ViewLayer LayerType>
  class RegexConstraint : public Constraint<LayerType> {
  public:

    RegexConstraint(
        std::string pattern,
        std::string name="regex_constraint"
    ) : pattern_(std::move(pattern)), Constraint<LayerType>({
      .leaf = false, .fatal = false, .name = std::move(name)
      }) {}

    RegexConstraint(
        std::string pattern,
        ConstraintProperties&& props
    ) : pattern_(std::move(pattern)), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      if (!value.is_string()) return this->ok();
      if (std::regex_match(value.get_cstr(), pattern_)) { return this->ok(); }
      else { return this->fail("invalid value.", concise); }
    }

  private:
    std::regex pattern_;
  };


  template<ViewLayer LayerType>
  class AnyConstraint : public Constraint<LayerType> {
  public:

  AnyConstraint(
      std::vector<std::shared_ptr<Constraint<LayerType>>>&& constraints,
      ConstraintProperties&& props
  ) : constraints_(std::move(constraints)), Constraint<LayerType>(std::move(props)) {}

  AnyConstraint(
      const std::vector<std::shared_ptr<Constraint<LayerType>>>& constraints,
      ConstraintProperties&& props
  ) : constraints_(constraints), Constraint<LayerType>(std::move(props)) {}

  ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
    for(const auto& constraint : constraints_) {
      auto result = constraint->test(value, true);
      if (result.valid) return this->ok();
    }
    return this->fail("None of the constraints read this value.", concise);
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;
  };


  template<ViewLayer LayerType>
  class ListConstraint : public Constraint<LayerType> {
  public:

    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    ListConstraint() : Constraint<LayerType>({false, "list_constraint"}) {}

    ListConstraint(
      ConstraintPtr&& constraint,
      ConstraintProperties&& props
    ) : constraint_(std::move(constraint)), Constraint<LayerType>(std::move(props)) {}

    ListConstraint(
      const ConstraintPtr& constraint,
      ConstraintProperties&& props
    ) : constraint_(constraint), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      if (!value.is_list()) return this->fail("Expected a list.", concise);
      int index = 0;
      for (const auto& item : value.get_list()) {
        auto result = constraint_->test(item, this->props_.leaf || concise);
        if (!result.valid) {
          auto final_result = this->fail("Invalid value found in the list.", concise);
          if (!concise) {
            final_result.details.push_back(ConstraintResult{
                .valid = false,
                .name = std::to_string(index),
                .reason = "invalid value.",
                .details = (this->props_.leaf ? std::vector<ConstraintResult>{} : std::vector<ConstraintResult>{std::move(result)}),
                .field = true
                });
          }
          return final_result;
        }
        index++;
      }
      return this->ok();
    }

  private:
    ConstraintPtr constraint_;
  };


  template<ViewLayer LayerType>
  class TupleConstraint : public Constraint<LayerType> {
  public:

  TupleConstraint() : Constraint<LayerType>({
      .leaf = false, .fatal = false, .name = "tuple_constraint"
      }) {}

  TupleConstraint(
    std::vector<std::shared_ptr<Constraint<LayerType>>>&& constraints,
    bool strict,
    ConstraintProperties&& props
  ) : constraints_(std::move(constraints)), strict_(strict), Constraint<LayerType>(std::move(props)) {}

  TupleConstraint(
    const std::vector<std::shared_ptr<Constraint<LayerType>>>& constraints,
    bool strict,
    ConstraintProperties&& props
  ) : constraints_(constraints), strict_(strict), Constraint<LayerType>(std::move(props)) {}

  ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
    if (!value.is_list()) return this->fail("Expected a list (tuple).", concise);
    int index = 0;
    auto tuple_it = value.begin_list();
    auto constraint_it = constraints_.begin();
    while (constraint_it != constraints_.end() && tuple_it != value.end_list()) {
      auto result = (*constraint_it)->test(*tuple_it, this->props_.leaf || concise);
      if (!result.valid) {
        auto final_result = this->fail("Invalid value found in the tuple.", concise);
        if (!concise) {
          final_result.details.push_back(ConstraintResult{
              .valid = false,
              .name = std::to_string(index),
              .reason = "invalid value.",
              .details = (this->props_.leaf ? std::vector<ConstraintResult>{} : std::vector<ConstraintResult>{std::move(result)}),
              .field = true
              });
        }
        return final_result;
      }
      std::advance(tuple_it, 1);
      std::advance(constraint_it, 1);
      index++;
    }
    if (strict_ && tuple_it != value.end_list()) {
      return this->fail("Too many values in the tuple.", concise);
    }
    if (constraint_it != constraints_.end()) {
      return this->fail("Too few values in the tuple.", concise);
    }
    return this->ok();
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;
    bool strict_;
  };


  template<ViewLayer LayerType>
  class MapConstraint : public Constraint<LayerType> {
  public:
    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    MapConstraint() : Constraint<LayerType>({
        .leaf = false, .fatal = false, .name = "map_constraint"
        }) {}

    MapConstraint(
      ConstraintPtr key_constraint,
      ConstraintPtr value_constraint,
      ConstraintProperties&& props
    ) : key_constraint_(std::move(key_constraint)),
        value_constraint_(std::move(value_constraint)),
        Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      if (!value.is_object()) return this->fail("Expected an object.", concise);
      for (const auto& item : value.get_object()) {
        if (key_constraint_) {
          if (auto result = key_constraint_->test(item.key); !result.valid) {
            auto final_result = this->fail("Object contains invalid key.", concise);
            if (!(this->props_.leaf || concise)) {
              final_result.details.push_back(std::move(result));
            }
            return final_result;
          }
        }
        if (value_constraint_) {
          if (auto result = value_constraint_->test(item.value); !result.valid) {
            auto final_result = this->fail("Object contains invalid value.", concise);
            if (!(this->props_.leaf || concise)) {
              final_result.details.push_back(std::move(result));
            }
            return final_result;
          }
        }
      }
      return this->ok();
    }

  private:
    ConstraintPtr key_constraint_;
    ConstraintPtr value_constraint_;
  };


  template<ViewLayer LayerType>
  class AllConstraint : public Constraint<LayerType> {
  public:

    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    AllConstraint() : Constraint<LayerType>({
        .leaf = false, .fatal = true
        }) {}

    AllConstraint(
        std::vector<ConstraintPtr>&& constraints,
        ConstraintProperties&& props,
        bool hide = true
    ) : constraints_(std::move(constraints)),
        Constraint<LayerType>(std::move(props)),
        hide_(hide) {}

    ConstraintResult test(const LayerType& value, bool concise = false) const noexcept override {
      if (hide_) {
        for (const auto& constraint : constraints_) {
          if (auto result = constraint->test(value, concise); !result.valid) return result;
        }
        return this->ok();
      }
      if (this->props_.leaf || concise) {
        if (test_constraints_concise(value, constraints_)) return this->ok();
        else return this->fail("Some of the constraints fail on this value.", concise);
      }
      std::vector<ConstraintResult> results;
      test_constraints(value, constraints_, results);
      if (results.empty()) return this->ok();
      return this->fail("Some of the constraints fail on this value.", std::move(results));
    }

  private:
    std::vector<ConstraintPtr> constraints_;
    bool hide_;
  };

}

#endif /* end of include guard: GARLIC_CONSTRAINTS */
