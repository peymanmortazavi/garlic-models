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
      sequence<ConstraintResult> failures;

      inline bool is_valid() const noexcept { return failures.empty(); }
    };

    struct Properties {
      std::string name;
      std::map<std::string, std::string> meta;
      std::vector<ConstraintPtr> constraints;
      bool ignore_details = false;
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
      test_constraints(value, properties_.constraints, std::back_inserter(result.failures));
      return result;
    }

    bool quick_test(const LayerType& value) const noexcept {
      return test_constraints_quick(value, properties_.constraints);
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

    bool quick_test(const LayerType& value) const {
      if (!value.is_object()) return false;
      std::set<std::string_view> requirements;
      for (const auto& member : value.get_object()) {
        auto it = properties_.field_map.find(member.key.get_cstr());
        if (it == properties_.field_map.end()) continue;
        if (!it->second.field->quick_test(member.value)) {
          return false;
        }
        requirements.emplace(it->first);
      }
      for (const auto& item : properties_.field_map) {
        if (auto it = requirements.find(item.first); it != requirements.end()) continue;
        if (!item.second.required) continue;
        return false;
      }
      return true;
    }

    ConstraintResult validate(const LayerType& value) const {
      sequence<ConstraintResult> details;
      if (value.is_object()) {
        // todo : if the container allows for atomic table look up, swap the loop.
        std::set<std::string_view> requirements;
        for (const auto& member : value.get_object()) {
          auto it = properties_.field_map.find(member.key.get_cstr());
          if (it != properties_.field_map.end()) {
            this->test_field(details, member.key, member.value, it->second.field);
            requirements.emplace(it->first);
          }
        }
        for (const auto& item : properties_.field_map) {
          if (auto it = requirements.find(item.first); it != requirements.end()) continue;
          if (!item.second.required) continue;
          details.push_back(ConstraintResult {
                .details = sequence<ConstraintResult>(0),
                .name = garlic::text(item.first, text_type::copy),
                .reason = garlic::text("missing required field!"),
                .flag = ConstraintResult::flags::field
              });
        }
      } else {
        details.push_back(ConstraintResult::leaf_failure("type", "Expected object."));
      }
      if (!details.empty()) {
        return ConstraintResult {
          .details = std::move(details),
          .name = garlic::text(properties_.name, text_type::copy),
          .reason = garlic::text("This model is invalid!"),
          .flag = ConstraintResult::flags::none
        };
      }

      return ConstraintResult::ok();
    }

    const Properties& get_properties() const { return properties_; }

  protected:
    Properties properties_;

  private:
    inline void
    test_field(
        sequence<ConstraintResult>& details,
        const LayerType& key,
        const LayerType& value,
        const FieldPtr& field) const {
      if (field->get_properties().ignore_details) {
        auto test = field->quick_test(value);
        if (!test) {
          const char* reason = field->get_message();
          details.push_back(ConstraintResult {
              .details = sequence<ConstraintResult>(0),
              .name = garlic::text(key.get_cstr(), text_type::copy),
              .reason = (reason == nullptr ? garlic::text::no_text() : garlic::text(reason, text_type::copy)),
              .flag = ConstraintResult::flags::field
              });
        }
      } else {
        auto test = field->validate(value);
        if (!test.is_valid()) {
          const char* reason = field->get_message();
          details.push_back(ConstraintResult {
              .details = std::move(test.failures),
              .name = garlic::text(key.get_cstr(), text_type::copy),
              .reason = (reason == nullptr ? garlic::text::no_text() : garlic::text(reason, text_type::copy)),
              .flag = ConstraintResult::flags::field
              });
        }
      }
    }
  };


  // Model Constraint
  template<ViewLayer LayerType>
  class ModelConstraint : public Constraint<LayerType> {
  public:
    using ModelType = Model<LayerType>;

    ModelConstraint(
        std::shared_ptr<ModelType> model
    ) : Constraint<LayerType>(ConstraintProperties {
      .flag = ConstraintProperties::flags::fatal,
      .name = garlic::text(model->get_properties().name),
      .message = garlic::text::no_text()
      }), model_(model) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      return model_->validate(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return model_->quick_test(value);
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
        bool hide = false,
        bool ignore_details = false
    ) : field_(std::move(field)),
        Constraint<LayerType>(std::move(props)),
        hide_(hide), ignore_details_(ignore_details) {
      this->update_name();
    }

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (hide_) {
        return test_constraints_first_failure(value, (*field_)->get_properties().constraints);
      }
      const auto& field = *field_;
      if (ignore_details_ || field->get_properties().ignore_details) {
        if (field->quick_test(value)) return this->ok();
        return this->custom_message_fail(field);
      }
      auto result = (*field_)->validate(value);
      if (result.is_valid()) return this->ok();
      return this->custom_message_fail(field, std::move(result.failures));
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return (*field_)->quick_test(value);
    }

    void set_field(FieldPtr field) {
      field_->swap(field);
      this->update_name();
    }

  protected:
    FieldReference field_;
    bool hide_;
    bool ignore_details_;

    void update_name() {
      if (*field_ && this->props_.name.empty()) {
        this->props_.name = (*field_)->get_name();
      }
    }

    template<typename... Args>
    inline ConstraintResult custom_message_fail(const FieldPtr& field, Args&&... args) const noexcept {
      if (auto message = field->get_message(); message != nullptr)
        return this->fail(message, std::forward<Args>(args)...);
      return this->fail("", std::forward<Args>(args)...);
    }
  };

}


#endif /* end of include guard: GARLIC_MODELS_H */
