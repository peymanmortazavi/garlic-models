#ifndef MODELS_H
#define MODELS_H

#include <iostream>
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

    struct ValidationResult {
      std::vector<ConstraintResult> failures;
      bool is_valid() { return !failures.size(); }
    };

    struct Properties {
      bool required;
      std::string name;
      std::map<std::string, std::string> meta;
      std::vector<ConstraintPtr> constraints;
    };
    
    Field(Properties&& properties) : properties_(std::move(properties)) {}
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

    ValidationResult test(const LayerType& value) const {
      ValidationResult result;
      for(auto& constraint : properties_.constraints) {
        auto test = constraint->test(value);
        if(!test.valid) {
          //result.failures.push_back(ConstraintFailure(constraint.get(), std::move(test)));
          result.failures.push_back(std::move(test));
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
    using FieldType = Field<LayerType>;
    using FieldPtr = std::shared_ptr<FieldType>;

    struct Properties {
      std::string name;
      bool strict = false;
      std::map<std::string, std::string> meta;
      std::map<std::string, FieldPtr> field_map;
    };

    Model(Properties properties) : properties_(properties) {}
    Model(std::string&& name) {
      properties_.name = std::move(name);
    }
    Model(const std::string& name) {
      properties_.name = name;
    }

    void add_field(std::string&& name, FieldPtr field) {
      if (field->is_required()) required_fields_.push_back(name);
      properties_.field_map.emplace(std::move(name), std::move(field));
    }

    const Properties& get_properties() const { return properties_; }
    const std::vector<std::string> get_required_fields() const { return required_fields_; }

  protected:
    Properties properties_;
    std::vector<std::string> required_fields_;
  };


  // Model Constraint
  template<ReadableLayer LayerType>
  class ModelConstraint : public Constraint<LayerType> {
  public:
    using ModelType = Model<LayerType>;

    ModelConstraint(ModelType&& model) : model_(std::move(model)) {
      name_ = model.get_properties().name;
    }

    ModelConstraint(
        ModelType&& model,
        std::string&& name
    ) : model_(std::move(model)), name_(std::move(name)) {}

    ConstraintResult test(const LayerType& value) const override {
      ConstraintResult result;
      const auto& properties = model_.get_properties();
      auto required_fields = model_.get_required_fields();
      if (value.is_object()) {
        for (const auto& member : value.get_object()) {
          auto it = properties.field_map.find(member.key.get_cstr());
          if (it != properties.field_map.end()) {
            auto test = it->second->test(member.value);
            if (!test.is_valid()) {
              result.valid = false;
              result.details.push_back({false, member.key.get_cstr(), "find a summary of why this field value sucks.", std::move(test.failures), true});
              //result.details.push_back({false, });
            }
          }
        }
      }
      if (!result.valid) {
        result.name = model_.get_properties().name;
        result.reason = "This model is invalid";
      }
      return result;
    }

    const std::string& get_name() const noexcept override { return name_; }
    bool skip_constraints() const noexcept override { return true; }

  private:
    ModelType model_;
    std::string name_;
  };

}


#endif /* end of include guard: MODELS_H */
