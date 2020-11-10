#ifndef GARLIC_MODELS_H
#define GARLIC_MODELS_H

#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include "layer.h"
#include "constraints.h"
#include "utility.h"


namespace garlic {

  template<garlic::ReadableLayer LayerType>
  class Field {
  public:
    template<garlic::ReadableLayer> friend class Module;

    using ConstraintType = Constraint<LayerType>;
    using ConstraintPtr = std::shared_ptr<ConstraintType>;

    struct ValidationResult {
      std::vector<ConstraintResult> failures;
      bool is_valid() { return !failures.size(); }
    };

    struct Properties {
      std::string name;
      std::map<std::string, std::string> meta;
      std::vector<ConstraintPtr> constraints;
    };
    
    Field(Properties&& properties) : properties_(std::move(properties)) {}
    Field(std::string&& name) { properties_.name = std::move(name); }
    Field(const std::string& name) { properties_.name = name; }

    template<template <typename> typename T, typename... Args>
    void add_constraint(Args&&... args) {
      this->add_constraint(std::make_shared<T<LayerType>>(std::forward<Args>(args)...));
    }

    void add_constraint(ConstraintPtr&& constraint) {
      properties_.constraints.push_back(std::move(constraint));
    }

    ValidationResult test(const LayerType& value) const {
      ValidationResult result;
      for(auto& constraint : properties_.constraints) {
        auto test = constraint->test(value);
        if(!test.valid) {
          result.failures.push_back(std::move(test));
          if (constraint->skip_constraints()) break;
        }
      }
      return result;
    }

  const std::string& get_name() const noexcept { return properties_.name; }
  const Properties& get_properties() const noexcept { return properties_; }

  protected:
    Properties properties_;
  };


  template<ReadableLayer LayerType>
  class Model {
  public:
    template<garlic::ReadableLayer> friend class Module;

    using Layer = LayerType;
    using FieldType = Field<LayerType>;
    using FieldPtr = std::shared_ptr<FieldType>;

    struct Properties {
      std::string name;
      bool strict = false;
      std::map<std::string, std::string> meta;
      std::map<std::string, FieldPtr> field_map;
    };

    Model() {}
    Model(Properties properties) : properties_(properties) {}
    Model(std::string&& name) {
      properties_.name = std::move(name);
    }
    Model(const std::string& name) {
      properties_.name = name;
    }

    void add_field(std::string&& name, FieldPtr field) {
      //if (field->is_required()) required_fields_.push_back(name);
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

    ModelConstraint(
        std::shared_ptr<ModelType> model
    ) : model_(std::move(model)), Constraint<LayerType>({true, model->get_properties().name}) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      ConstraintResult result;
      const auto& properties = model_->get_properties();
      auto required_fields = model_->get_required_fields();
      if (value.is_object()) {
        for (const auto& member : value.get_object()) {
          auto it = properties.field_map.find(member.key.get_cstr());
          if (it != properties.field_map.end()) {
            auto test = it->second->test(member.value);
            if (!test.is_valid()) {
              result.valid = false;
              result.details.push_back({
                  false,
                  member.key.get_cstr(),
                  "find a summary of why this field value sucks.",
                  std::move(test.failures),
                  true
              });
            }
          }
        }
      } else {
        result.valid = false; 
        result.details.push_back({
            false,
            "type",
            "Expected object."
        });
      }
      if (!result.valid) {
        result.name = model_->get_properties().name;
        result.reason = "This model is invalid";
      }
      return result;
    }

  private:
    std::shared_ptr<ModelType> model_;
  };

  // Model Parsing From ReadableLayer
  template<typename T> using ModelPropertiesOf = typename Model<T>::Properties;
  template<typename T> using FieldPropertiesOf = typename Field<T>::Properties;
  
  template<ReadableLayer LayerType, typename...Args>
  std::shared_ptr<Field<LayerType>> make_field(Args&&... args) {
    return std::make_shared<Field<LayerType>>(std::forward<Args>(args)...);
  }

  template<ReadableLayer LayerType, typename...Args>
  std::shared_ptr<Model<LayerType>> make_model(Args&&...args) {
    return std::make_shared<Model<LayerType>>(std::forward<Args>(args)...);
  }


  template<ReadableLayer LayerType>
  class FieldConstraint : public Constraint<LayerType> {
  public:
    using FieldPtr = std::shared_ptr<Field<LayerType>>;
    using FieldReference = std::shared_ptr<FieldPtr>;

    FieldConstraint(
        FieldReference field,
        ConstraintProperties&& props
    ) : field_(std::move(field)), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      auto result = (*field_)->test(value);
      if (result.is_valid()) return {true};
      return {
        false, this->props_.name, this->props_.message,
        std::move(result.failures), false
      };
    }

    void set_field(FieldPtr field) { field_->swap(field); }

    template<ReadableLayer Source, typename Parser>
    static std::shared_ptr<Constraint<LayerType>> parse(const Source& value, Parser parser) noexcept {
      ConstraintProperties props {true, "field_constraint"};
      set_constraint_properties(value, props);
      std::shared_ptr<FieldConstraint<LayerType>> result;
      get_member(value, "field", [&parser, &result, &props](const auto& field) {
          auto ptr = parser.resolve_field_reference(field.get_cstr());
          result = std::make_shared<FieldConstraint<LayerType>>(
              std::make_shared<FieldPtr>(ptr), std::move(props)
          );
          if (!ptr) {
            parser.add_field_dependency(field.get_cstr(), result);
          }
      });
      return result;
    }

  protected:
    FieldReference field_;
  };

}


#endif /* end of include guard: GARLIC_MODELS_H */
