#ifndef MODELS_H
#define MODELS_H

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "layer.h"
#include "constraints.h"


namespace garlic {

  template<garlic::ReadableLayer LayerType>
  class Field {
  public:
    using Layer = LayerType;
    using ConstraintType = Constraint<LayerType>;
    using ConstraintPtr = std::unique_ptr<ConstraintType>;

    class ConstraintFailure {
    public:
      ConstraintFailure(const ConstraintType* ptr, ConstraintResult&& result) : ptr_(ptr), result_(std::move(result)) {}
      const ConstraintType& get_constraint() const { return *ptr_; }
      const ConstraintResult& get_result() const { return result_; }

    private:
      const ConstraintType* ptr_;
      ConstraintResult result_;
    };

    struct ValidationResult {
      std::vector<ConstraintFailure> failures;
      bool is_valid() { return !failures.size(); }
    };

    struct Properties {
      bool required;
      std::string name;
      std::string description;
      std::vector<ConstraintPtr> constraints;
    };
    
    Field(Properties properties) : properties_(properties) {}
    Field(std::string&& name, bool required=true) {
      properties_.required = required;
      properties_.name = std::move(name);
    }
    Field(const std::string& name, bool required=true) {
      properties_.name = name;
      properties_.required = required;
    }

    template<template <typename> typename T, typename... Args>
    void add_constraint(Args&&... args) {
      this->add_constraint(std::make_unique<T<LayerType>>(std::forward<Args>(args)...));
    }

    void add_constraint(ConstraintPtr&& constraint) {
      properties_.constraints.push_back(std::move(constraint));
    }

    ValidationResult validate(const LayerType& value) {
      ValidationResult result;
      for(auto& constraint : properties_.constraints) {
        auto test = constraint->test(value);
        if(!test.valid) {
          result.failures.push_back(ConstraintFailure(constraint.get(), std::move(test)));
          if (constraint->skip_constraints()) break;
        }
      }
      return result;
    }

  const std::string& get_name() const noexcept { return properties_.name; }
  const std::string& get_description() const noexcept { return properties_.description; }
  bool is_required() const noexcept { return properties_.required; }

  protected:
    Properties properties_;
  };


  template<garlic::ReadableLayer LayerType>
  class Model {
  public:
    using Layer = LayerType;
    using ConstraintType = Constraint<LayerType>;
    using ConstraintPtr = std::unique_ptr<ConstraintType>;
    using FieldType = Field<LayerType>;

    struct Properties {
      bool required;
      std::string name;
      std::string description;
      std::vector<ConstraintPtr> constraints;
    };

  };

}


#endif /* end of include guard: MODELS_H */
