#ifndef GARLIC_MODELS_H
#define GARLIC_MODELS_H

#include <iostream>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include "layer.h"
#include "constraints.h"
#include "utility.h"


namespace garlic {

  template<ViewLayer LayerType>
  class Field {
  public:
    template<ViewLayer> friend class Module;

    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    struct ValidationResult {
      sequence<ConstraintResult> failures;

      inline bool is_valid() const noexcept { return failures.empty(); }
    };

    struct Properties {
      text name;
      std::unordered_map<text, text> meta;
      sequence<constraint_pointer> constraints;
      bool ignore_details = false;
    };
    
    Field(Properties&& properties) : properties_(std::move(properties)) {}
    Field(text&& name) { properties_.name = std::move(name); }

    template<template <typename> typename T, typename... Args>
    void add_constraint(Args&&... args) {
      this->add_constraint(std::make_shared<T<LayerType>>(std::forward<Args>(args)...));
    }

    void add_constraint(constraint_pointer&& constraint) {
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
        return it->second.data();
      }
      return nullptr;
    }
    text get_name() const noexcept { return properties_.name.copy(); }
    const Properties& get_properties() const noexcept { return properties_; }

  protected:
    Properties properties_;
  };


  template<ViewLayer LayerType>
  class Model {
  public:
    template<ViewLayer> friend class Module;

    using field_type = Field<LayerType>;
    using field_pointer = std::shared_ptr<field_type>;

    struct field_descriptor {
      field_pointer field;
      bool required;
    };

    struct Properties {
      text name;
      bool strict = false;
      std::unordered_map<text, text> meta;
      std::unordered_map<text, field_descriptor> field_map;
    };

    Model() {}
    Model(Properties&& properties) : properties_(std::move(properties)) {}
    Model(text&& name) {
      properties_.name = std::move(name);
    }

    void add_field(text&& name, field_pointer field, bool required = true) {
      properties_.field_map.emplace(
          std::move(name),
          { .field = std::move(field), .required = required }
          );
    }

    template<typename KeyType>
    field_pointer get_field(KeyType&& name) const {
      auto it = properties_.field_map.find(name);
      if (it != properties_.field_map.end()) {
        return it->second.field;
      }
      return nullptr;
    }

    bool quick_test(const LayerType& value) const {
      if (!value.is_object()) return false;
      std::unordered_set<text> requirements;
      for (const auto& member : value.get_object()) {
        auto it = properties_.field_map.find(member.key.get_cstr());
        if (it == properties_.field_map.end()) continue;
        if (!it->second.field->quick_test(member.value)) {
          return false;
        }
        requirements.emplace(it->first.copy());
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
        std::unordered_set<text> requirements;
        for (const auto& member : value.get_object()) {
          auto it = properties_.field_map.find(member.key.get_cstr());
          if (it != properties_.field_map.end()) {
            this->test_field(details, member.key, member.value, it->second.field);
            requirements.emplace(it->first.copy());
          }
        }
        for (const auto& item : properties_.field_map) {
          if (auto it = requirements.find(item.first); it != requirements.end()) continue;
          if (!item.second.required) continue;
          details.push_back(
              ConstraintResult::leaf_field_failure(
                item.first.copy(),
                "missing required field!"));
        }
      } else {
        details.push_back(ConstraintResult::leaf_failure("type", "Expected object."));
      }
      if (!details.empty()) {
        return ConstraintResult {
          .details = std::move(details),
          .name = properties_.name.deep_copy(),
          .reason = text("This model is invalid!"),
          .flag = ConstraintResult::flags::none
        };
      }

      return ConstraintResult::ok();
    }

    text get_name() const noexcept { return properties_.name.copy(); }
    const Properties& get_properties() const { return properties_; }

  protected:
    Properties properties_;

  private:
    inline void
    test_field(
        sequence<ConstraintResult>& details,
        const LayerType& key,
        const LayerType& value,
        const field_pointer& field) const {
      if (field->get_properties().ignore_details) {
        if (!field->quick_test(value)) {
          const char* reason = field->get_message();
          auto name = key.get_string_view();
          details.push_back(ConstraintResult {
              .details = sequence<ConstraintResult>::no_sequence(),
              .name = text(name.data(), name.size(), text_type::copy),
              .reason = (reason == nullptr ? text::no_text() : text(reason, text_type::copy)),
              .flag = ConstraintResult::flags::field
              });
        }
      } else {
        auto test = field->validate(value);
        if (!test.is_valid()) {
          const char* reason = field->get_message();
          auto name = key.get_string_view();
          details.push_back(ConstraintResult {
              .details = std::move(test.failures),
              .name = text(name.data(), name.size(), text_type::copy),
              .reason = (reason == nullptr ? text::no_text() : text(reason, text_type::copy)),
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
    using model_type = Model<LayerType>;
    using model_pointer = std::shared_ptr<model_type>;

    ModelConstraint(
        model_pointer model
    ) : Constraint<LayerType>(ConstraintProperties {
      .flag = ConstraintProperties::flags::fatal,
      .name = model->get_properties().name.copy(),
      .message = text::no_text()
      }), model_(model) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      return model_->validate(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return model_->quick_test(value);
    }

  private:
    model_pointer model_;
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
    using field_type = Field<LayerType>;
    using field_pointer = std::shared_ptr<field_type>;
    using field_pointer_reference = std::shared_ptr<field_pointer>;  // double pointer.

    FieldConstraint(
        field_pointer_reference field,
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

    void set_field(field_pointer field) {
      field_->swap(field);
      this->update_name();
    }

  protected:
    field_pointer_reference field_;
    bool hide_;
    bool ignore_details_;

    void update_name() {
      if (*field_ && this->props_.name.empty()) {
        this->props_.name = (*field_)->get_name();
      }
    }

    template<typename... Args>
    inline ConstraintResult custom_message_fail(const field_pointer& field, Args&&... args) const noexcept {
      if (auto message = field->get_message(); message != nullptr)
        return this->fail(message, std::forward<Args>(args)...);
      return this->fail("", std::forward<Args>(args)...);
    }
  };

}


#endif /* end of include guard: GARLIC_MODELS_H */
