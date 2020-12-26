#ifndef GARLIC_CONSTRAINTS
#define GARLIC_CONSTRAINTS

#include "layer.h"
#include "utility.h"
#include <algorithm>
#include <cstdlib>
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
    bool fatal = false;  // should stop looking at other constraints.
    std::string name;  // constraint name.
    std::string message;  // custom rejection reason.
  };


  template<typename ConstraintPtrType>
  inline void test_constraints(
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
  inline bool test_constraints_quick(
      const ViewLayer auto& value,
      const std::vector<ConstraintPtrType>& constraints) {
    return std::all_of(
        constraints.begin(), constraints.end(),
        [&value](const auto& constraint) { return constraint->quick_test(value); }
        );
  }


  template<typename ConstraintPtrType>
  inline ConstraintResult test_constraints_first_failure(
      const ViewLayer auto& value,
      std::vector<ConstraintPtrType> constraints
      ) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.valid) {
        return std::move(result);
      }
    }
    return { .valid = true };
  }


  void set_constraint_properties(const ViewLayer auto& value, ConstraintProperties& props) noexcept {
    get_member(value, "fatal", [&props](const auto& item) { props.fatal = item.get_bool(); });
    get_member(value, "message", [&props](const auto& item) { props.message = item.get_cstr(); });
    get_member(value, "name", [&props](const auto& item) { props.name = item.get_cstr(); });
  }


  template<garlic::ViewLayer LayerType>
  class Constraint {
  public:
    explicit Constraint(ConstraintProperties&& props) : props_(std::move(props)) {}

    virtual ConstraintResult test(const LayerType& value) const noexcept = 0;
    virtual bool quick_test(const LayerType& value) const noexcept = 0;
    std::string_view get_name() const noexcept { return props_.name; };
    bool skip_constraints() const noexcept { return props_.fatal; };

    auto fail(bool field = false) const noexcept -> ConstraintResult {
      return ConstraintResult{
        .valid = false,
        .name = this->props_.name,
        .reason = this->props_.message,
        .field = field
      };
    }

    auto fail(const char* message, bool field=false) const noexcept -> ConstraintResult {
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

    auto fail(const char* message, std::vector<ConstraintResult>&& details, bool field=false) const noexcept {
      if (!this->props_.message.empty()) {
        return ConstraintResult {
          .valid = false,
          .name = this->props_.name,
          .reason = props_.message,
          .details = std::move(details),
          .field = field,
        };
      } else {
        return ConstraintResult {
          .valid = false,
          .name = this->props_.name,
          .reason = message,
          .details = std::move(details),
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
      .fatal = true, .name = std::move(name)
      }) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: {
          if (value.is_null()) { return this->ok(); }
          else return this->fail("Expected null.");
        }
        case TypeFlag::Boolean: {
          if (value.is_bool()) { return this->ok(); }
          else return this->fail("Expected boolean type.");
        }
        case TypeFlag::Double: {
          if (value.is_double()) { return this->ok(); }
          else return this->fail("Expected double type.");
        }
        case TypeFlag::Integer: {
          if (value.is_int()) { return this->ok(); }
          else return this->fail("Expected integer type.");
        }
        case TypeFlag::String: {
          if (value.is_string()) { return this->ok(); }
          else return this->fail("Expected string type.");
        }
        case TypeFlag::List: {
          if (value.is_list()) { return this->ok(); }
          else return this->fail("Expected a list.");
        }
        case TypeFlag::Object: {
          if (value.is_object()) { return this->ok(); }
          else return this->fail("Expected an object.");
        }
        default: return this->fail();
      }
    }

    bool quick_test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: return value.is_null();
        case TypeFlag::Boolean: return value.is_bool();
        case TypeFlag::Double: return value.is_double();
        case TypeFlag::Integer: return value.is_int();
        case TypeFlag::String: return value.is_string();
        case TypeFlag::List: return value.is_list();
        case TypeFlag::Object: return value.is_object();
        default: return false;
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
      .fatal = false, .name = std::move(name)
      }) {}

    RangeConstraint(
        SizeType min,
        SizeType max,
        ConstraintProperties&& props
    ) : min_(min), max_(max), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return this->fail("invalid string length.");
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return this->fail("out of range value.");
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(ivalue > max_ || ivalue < min_) return this->fail("out of range value.");
      } else if (value.is_list()) {
        int count = 0;
        for (const auto& item : value.get_list()) {
          count++;
          if (count > max_) return this->fail("too many items in the list.");
        }
        if (count < min_) return this->fail("too few items in the list.");
      }
      return this->ok();
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return false;
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return false;
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(ivalue > max_ || ivalue < min_) return false;
      } else if (value.is_list()) {
        int count = 0;
        for (const auto& item : value.get_list()) {
          count++;
          if (count > max_) return false;
        }
        if (count < min_) return false;
      }
      return true;
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
      .fatal = false, .name = std::move(name)
      }) {}

    RegexConstraint(
        std::string pattern,
        ConstraintProperties&& props
    ) : pattern_(std::move(pattern)), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return this->ok();
      if (std::regex_match(value.get_cstr(), pattern_)) { return this->ok(); }
      else { return this->fail("invalid value."); }
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return true;
      if (std::regex_match(value.get_cstr(), pattern_)) return true;
      else return false;
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

  ConstraintResult test(const LayerType& value) const noexcept override {
    if (this->validate(value))
      return this->ok();
    return this->fail("None of the constraints read this value.");
  }

  bool quick_test(const LayerType& value) const noexcept override {
    return this->validate(value);
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;

    inline bool validate(const LayerType& value) const noexcept {
      return std::any_of(
          constraints_.begin(),
          constraints_.end(),
          [&value](const auto& item) { return item->quick_test(value); });
    }
  };


  template<ViewLayer LayerType>
  class ListConstraint : public Constraint<LayerType> {
  public:

    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    ListConstraint() : Constraint<LayerType>({false, "list_constraint"}) {}

    ListConstraint(
      ConstraintPtr&& constraint,
      ConstraintProperties&& props,
      bool ignore_details = false
    ) : constraint_(std::move(constraint)),
        Constraint<LayerType>(std::move(props)),
        ignore_details_(ignore_details) {}

    ListConstraint(
      const ConstraintPtr& constraint,
      ConstraintProperties&& props,
      bool ignore_details = false
    ) : constraint_(constraint),
        Constraint<LayerType>(std::move(props)),
        ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_list()) return this->fail("Expected a list.");
      if (ignore_details_) return this->test<true>(value);
      return this->test<false>(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_list()) return false;
      return std::all_of(
          value.begin_list(), value.end_list(),
          [this](const auto& item) { return constraint_->quick_test(item); }
          );
    }

  private:
    ConstraintPtr constraint_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      int index = 0;
      for (const auto& item : value.get_list()) {
        if (IgnoreDetails) {
          if (!constraint_->quick_test(item)) {
            return this->fail("Invalid value found in the list.", {{
                    .valid = false,
                    .name = std::to_string(index),
                    .reason = "invalid value.",
                    .field = true
                  }});
          }
        } else {
          auto result = constraint_->test(item);
          if (!result.valid) {
            return this->fail("Invalid value found in the list.", {{
                    .valid = false,
                    .name = std::to_string(index),
                    .reason = "invalid value.",
                    .details = {std::move(result)},
                    .field = true
                  }});
          }
        }
        index++;
      }
      return this->ok();
    }
  };


  template<ViewLayer LayerType>
  class TupleConstraint : public Constraint<LayerType> {
  public:

  TupleConstraint() : Constraint<LayerType>({
      .fatal = false, .name = "tuple_constraint"
      }) {}

  TupleConstraint(
    std::vector<std::shared_ptr<Constraint<LayerType>>>&& constraints,
    bool strict,
    ConstraintProperties&& props,
    bool ignore_details = false
  ) : constraints_(std::move(constraints)),
      strict_(strict),
      Constraint<LayerType>(std::move(props)),
      ignore_details_(ignore_details) {}

  TupleConstraint(
    const std::vector<std::shared_ptr<Constraint<LayerType>>>& constraints,
    bool strict,
    ConstraintProperties&& props,
    bool ignore_details = false
  ) : constraints_(constraints),
      strict_(strict),
      Constraint<LayerType>(std::move(props)),
      ignore_details_(ignore_details) {}

  ConstraintResult test(const LayerType& value) const noexcept override {
    if (ignore_details_) return this->test<true>(value);
    return this->test<false>(value);
  }

  bool quick_test(const LayerType& value) const noexcept override {
    if (!value.is_list()) return false;
    auto tuple_it = value.begin_list();
    auto constraint_it = constraints_.begin();
    while (constraint_it != constraints_.end() && tuple_it != value.end_list()) {
      if (!(*constraint_it)->quick_test(*tuple_it)) return false;
      std::advance(tuple_it, 1);
      std::advance(constraint_it, 1);
    }
    if (strict_ && tuple_it != value.end_list()) return false;
    if (constraint_it != constraints_.end()) return false;
    return true;
  }

  private:
    std::vector<std::shared_ptr<Constraint<LayerType>>> constraints_;
    bool strict_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      if (!value.is_list()) return this->fail("Expected a list (tuple).");
      int index = 0;
      auto tuple_it = value.begin_list();
      auto constraint_it = constraints_.begin();
      while (constraint_it != constraints_.end() && tuple_it != value.end_list()) {
        if (IgnoreDetails) {
          auto result = (*constraint_it)->quick_test(*tuple_it);
          if (!result) {
            return this->fail("Invalid value found in the tuple.", {{
                    .valid = false,
                    .name = std::to_string(index),
                    .reason = "invalid value.",
                    .field = true
                  }});
          }
        } else {
          auto result = (*constraint_it)->test(*tuple_it);
          if (!result.valid) {
            return this->fail("Invalid value found in the tuple.", {{
                    .valid = false,
                    .name = std::to_string(index),
                    .reason = "invalid value.",
                    .details = {std::move(result)},
                    .field = true
                  }});
          }
        }
        std::advance(tuple_it, 1);
        std::advance(constraint_it, 1);
        index++;
      }
      if (strict_ && tuple_it != value.end_list()) {
        return this->fail("Too many values in the tuple.");
      }
      if (constraint_it != constraints_.end()) {
        return this->fail("Too few values in the tuple.");
      }
      return this->ok();
    }
  };


  template<ViewLayer LayerType>
  class MapConstraint : public Constraint<LayerType> {
  public:
    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    MapConstraint() : Constraint<LayerType>({
        .fatal = false, .name = "map_constraint"
        }) {}

    MapConstraint(
      ConstraintPtr&& key_constraint,
      ConstraintPtr&& value_constraint,
      ConstraintProperties&& props,
      bool ignore_details = false
    ) : key_constraint_(std::move(key_constraint)),
        value_constraint_(std::move(value_constraint)),
        Constraint<LayerType>(std::move(props)),
        ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_object()) return this->fail("Expected an object.");
      if (ignore_details_) return this->test<true>(value);
      return this->test<false>(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_object()) return false;
      if (key_constraint_) {
        if (value_constraint_) return quick_test<true, true>(value);
        return quick_test<true, false>(value);
      } else if (value_constraint_) return quick_test<false, true>(value);
      return true;  // meaning there is no key or value constraint.
    }

  private:
    ConstraintPtr key_constraint_;
    ConstraintPtr value_constraint_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      if (key_constraint_) {
        if (value_constraint_) return test<IgnoreDetails, true, true>(value);
        return test<IgnoreDetails, true, false>(value);
      } else if (value_constraint_) return test<IgnoreDetails, false, true>(value);
      return this->ok();  // meaning there is no key or value constraint.
    }

    template<bool IgnoreDetails, bool HasKeyConstraint, bool HasValueConstraint>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      for (const auto& item : value.get_object()) {
        if (HasKeyConstraint) {
          if (IgnoreDetails && !key_constraint_->quick_test(item.key)) {
            return this->fail("Object contains invalid key.");
          } else {
            if (auto result = key_constraint_->test(item.key); !result.valid) {
              return this->fail("Object contains invalid key.", {std::move(result)});
            }
          }
        }
        if (HasValueConstraint) {
          if (IgnoreDetails && !value_constraint_->quick_test(item.value)) {
            return this->fail("Object contains invalid value.");
          } else {
            if (auto result = value_constraint_->test(item.value); !result.valid) {
              return this->fail("Object contains invalid value.", {std::move(result)});
            }
          }
        }
      }
      return this->ok();
    }

    template<bool HasKeyConstraint, bool HasValueConstraint>
    inline bool quick_test(const LayerType& value) const noexcept {
      return std::all_of(
          value.begin_member(), value.end_member(),
          [this](const auto& item) {
            if (HasKeyConstraint && !key_constraint_->quick_test(item.key)) return false;
            if (HasValueConstraint && !value_constraint_->quick_test(item.value)) return false;
            return true;
          }
          );
    }
  };


  template<ViewLayer LayerType>
  class AllConstraint : public Constraint<LayerType> {
  public:

    using ConstraintPtr = std::shared_ptr<Constraint<LayerType>>;

    AllConstraint() : Constraint<LayerType>({ .fatal = true }) {}

    AllConstraint(
        std::vector<ConstraintPtr>&& constraints,
        ConstraintProperties&& props,
        bool hide = true,
        bool ignore_details = false
    ) : constraints_(std::move(constraints)),
        Constraint<LayerType>(std::move(props)),
        hide_(hide), ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (hide_) return this->test_hide(value);
      if (ignore_details_) {
        if (test_constraints_quick(value, constraints_)) return this->ok();
        else return this->fail("Some of the constraints fail on this value.");
      }
      std::vector<ConstraintResult> results;
      test_constraints(value, constraints_, results);
      if (results.empty()) return this->ok();
      return this->fail("Some of the constraints fail on this value.", std::move(results));
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return test_constraints_quick(value, constraints_);
    }

  private:
    std::vector<ConstraintPtr> constraints_;
    bool hide_;
    bool ignore_details_;

    inline ConstraintResult test_hide(const LayerType& value) const noexcept {
      return test_constraints_first_failure(value, constraints_);
    }
  };


  template<ViewLayer LayerType>
  class LiteralConstraint : public Constraint<LayerType> {
  public:

    LiteralConstraint() : Constraint<LayerType>({
        .fatal = false, .name = "literal_constraint"
        }), type_(TypeFlag::Null) {}

    LiteralConstraint(
      ConstraintProperties&& props
    ) : Constraint<LayerType>(std::move(props)), type_(TypeFlag::Null) {
          printf("Making a null\n");
    }

    LiteralConstraint(bool value, ConstraintProperties&& props)
      : Constraint<LayerType>(std::move(props)),
        type_(TypeFlag::Boolean) {
          data_.integer = (int)value;
          printf("Making a bool: %d\n", data_.integer);
        }

    LiteralConstraint(int value, ConstraintProperties&& props)
      : Constraint<LayerType>(std::move(props)),
        type_(TypeFlag::Integer) {
          data_.integer = value;
          printf("Making an int: %d\n", data_.integer);
        }

    LiteralConstraint(double value, ConstraintProperties&& props)
      : Constraint<LayerType>(std::move(props)),
        type_(TypeFlag::Double) {
          data_.floating_number = value;
          printf("Making a double: %f\n", data_.floating_number);
        }

    LiteralConstraint(std::string&& value, ConstraintProperties&& props)
      : Constraint<LayerType>(std::move(props)),
        type_(TypeFlag::String) {
          data_.text = static_cast<char*>(std::malloc(sizeof(char) * value.size()));
          strcpy(data_.text, value.data());
          printf("Making a string: %s\n", data_.text);
        }

    ~LiteralConstraint() { if (type_ == TypeFlag::String) free(data_.text); }

    ConstraintResult test(const LayerType& value) const noexcept override {
      return (this->validate(value) ? this->ok() : this->fail("invalid value."));
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return this->validate(value);
    }

  private:
    union LiteralData {
      char* text;
      long integer;
      double floating_number;
    };

    TypeFlag type_;
    LiteralData data_;

    inline bool validate(const LayerType& value) const noexcept {
      switch (type_) {
        case TypeFlag::Null: return value.is_null();
        case TypeFlag::String:
          return (value.is_string() && strcmp(data_.text, value.get_cstr()) == 0);
        case TypeFlag::Double:
          return (value.is_double() && data_.floating_number == value.get_double());
        case TypeFlag::Integer:
          return (value.is_int() && (int)data_.integer == value.get_int());
        case TypeFlag::Boolean:
          return (value.is_bool() && (bool)data_.integer == value.get_bool());
      }
      return false;
    }
  };

}

#endif /* end of include guard: GARLIC_CONSTRAINTS */
