#ifndef GARLIC_MODELS_H
#define GARLIC_MODELS_H

#include <iostream>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "layer.h"
#include "constraints.h"
#include "utility.h"


namespace garlic {

  template<garlic::ViewLayer LayerType>
  class Field {
  public:
    template<garlic::ViewLayer> friend class Module;

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

    ValidationResult validate(const LayerType& value) const {
      ValidationResult result;
      test_constraints(value, properties_.constraints, result.failures);
      return result;
    }

    const char* get_message() const noexcept {
      const auto& meta = properties_.meta;
      if (auto it = meta.find("message"); it != meta.end()) {
        return it->second.c_str();
      }
      return nullptr;
    }
    const std::string& get_name() const noexcept { return properties_.name; }
    const Properties& get_properties() const noexcept { return properties_; }

  protected:
    Properties properties_;
  };


  template<ViewLayer LayerType>
  class Model {
  public:
    template<garlic::ViewLayer> friend class Module;

    using Layer = LayerType;
    using FieldType = Field<LayerType>;
    using FieldPtr = std::shared_ptr<FieldType>;

    struct FieldDescriptor {
      FieldPtr field;
      bool required;
    };

    struct Properties {
      std::string name;
      bool strict = false;
      std::map<std::string, std::string> meta;
      std::map<std::string, FieldDescriptor> field_map;
    };

    Model() {}
    Model(Properties properties) : properties_(properties) {}
    Model(std::string&& name) {
      properties_.name = std::move(name);
    }
    Model(const std::string& name) {
      properties_.name = name;
    }

    void add_field(std::string&& name, FieldPtr field, bool required = true) {
      properties_.field_map.emplace(
          std::move(name),
          { .field = std::move(field), .required = required }
          );
    }

    FieldPtr get_field(const std::string& name) const {
      auto it = properties_.field_map.find(name);
      if (it != properties_.field_map.end()) {
        return it->second.field;
      }
      return nullptr;
    }

    ConstraintResult validate(const LayerType& value) const {
      ConstraintResult result;
      if (value.is_object()) {
        // todo : if the container allows for atomic table look up, swap the loop.
        std::set<std::string_view> requirements;
        for (const auto& member : value.get_object()) {
          auto it = properties_.field_map.find(member.key.get_cstr());
          if (it != properties_.field_map.end()) {
            auto test = it->second.field->validate(member.value);
            if (!test.is_valid()) {
              const char* reason = it->second.field->get_message();
              result.details.push_back({
                    .valid = false,
                    .name = member.key.get_cstr(),
                    .reason = (reason == nullptr ? "" : reason),
                    .details = std::move(test.failures),
                    .field = true
                  });
            }
            requirements.emplace(it->first);
          }
        }
        for (const auto& item : properties_.field_map) {
          if (auto it = requirements.find(item.first); it != requirements.end()) continue;
          if (!item.second.required) continue;
          result.details.push_back({
                .valid = false,
                .name = item.first,
                .reason = "missing required field!",
                .field = true
              });
        }
      } else {
        result.details.push_back({
            .valid = false,
            .name = "type",
            .reason = "Expected object."
        });
      }
      if (!result.details.empty()) {
        result.valid = false;
        result.name = properties_.name;
        result.reason = "This model is invalid!";
      }
      return result;
    }

    const Properties& get_properties() const { return properties_; }

  protected:
    Properties properties_;
  };


  // Model Constraint
  template<ViewLayer LayerType>
  class ModelConstraint : public Constraint<LayerType> {
  public:
    using ModelType = Model<LayerType>;

    ModelConstraint(
        std::shared_ptr<ModelType> model
    ) : model_(std::move(model)), Constraint<LayerType>({
      .leaf = false, .fatal = true, .name = model->get_properties().name
      }) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      return model_->validate(value);
    }

  private:
    std::shared_ptr<ModelType> model_;
  };

  // Model Parsing From ViewLayer
  template<typename T> using ModelPropertiesOf = typename Model<T>::Properties;
  template<typename T> using FieldPropertiesOf = typename Field<T>::Properties;
  
  template<ViewLayer LayerType, typename...Args>
  std::shared_ptr<Field<LayerType>> make_field(Args&&... args) {
    return std::make_shared<Field<LayerType>>(std::forward<Args>(args)...);
  }

  template<ViewLayer LayerType, typename...Args>
  std::shared_ptr<Model<LayerType>> make_model(Args&&...args) {
    return std::make_shared<Model<LayerType>>(std::forward<Args>(args)...);
  }


  template<ViewLayer LayerType>
  class FieldConstraint : public Constraint<LayerType> {
  public:
    using FieldPtr = std::shared_ptr<Field<LayerType>>;
    using FieldReference = std::shared_ptr<FieldPtr>;

    FieldConstraint(
        FieldReference field,
        ConstraintProperties&& props,
        bool hide = false
    ) : field_(std::move(field)),
        Constraint<LayerType>(std::move(props)),
        hide_(hide) {
      this->update_name();
    }

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (hide_) {
        for (const auto& constraint : (*field_)->get_properties().constraints) {
          if (auto result = constraint->test(value); !result.valid) {
            return std::move(result);
          }
        }
        return this->ok();
      }
      auto result = (*field_)->validate(value);
      if (result.is_valid()) return this->ok();
      auto reason = (*field_)->get_message();
      if (reason == nullptr) return this ->fail("", std::move(result.failures));
      return this->fail(reason, std::move(result.failures));
    }

    void set_field(FieldPtr field) {
      field_->swap(field);
      this->update_name();
    }

  protected:
    FieldReference field_;
    bool hide_;

    void update_name() {
      if (*field_ && this->props_.name.empty()) {
        this->props_.name = (*field_)->get_name();
      }
    }
  };

}


#endif /* end of include guard: GARLIC_MODELS_H */
