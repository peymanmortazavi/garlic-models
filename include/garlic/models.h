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


  template<ReadableLayer LayerType>
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

    Model() {}
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

    ModelConstraint(std::shared_ptr<ModelType> model) : model_(std::move(model)) {
      //name_ = model->get_properties().name;
    }

    ConstraintResult test(const LayerType& value) const override {
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
              result.details.push_back({false, member.key.get_cstr(), "find a summary of why this field value sucks.", std::move(test.failures), true});
              //result.details.push_back({false, });
            }
          }
        }
      }
      if (!result.valid) {
        result.name = model_->get_properties().name;
        result.reason = "This model is invalid";
      }
      return result;
    }

    const std::string& get_name() const noexcept override { return model_->get_properties().name; }
    bool skip_constraints() const noexcept override { return true; }

  private:
    std::shared_ptr<ModelType> model_;
  };


  /**
   * A container provides a description of all fields and models agnostic to the layer type.
   */
  class Container {
  public:
    using Map = std::map<std::string, std::string>;

    struct FieldDefinition {
      std::string name;
      Map inputs;
      Map meta;
    };

    struct ModelDefinition {
    };
  };


  // Model Parsing From ReadableLayer
  template<typename T> using ModelPropertiesOf = typename Model<T>::Properties;

  template<ReadableLayer Source, ReadableLayer Destination = Source>
  class ModelContainer {
  public:
    template <typename T> using MapOf = std::map<std::string, T>;
    using ModelType = Model<Destination>;
    using ModelPtr = std::shared_ptr<ModelType>;
    using FieldType = Field<Destination>;
    using FieldPtr = std::shared_ptr<FieldType>;
    using ModelMap = MapOf<ModelPtr>;
    using FieldMap = MapOf<FieldPtr>;

    struct ParsingResult {
      bool valid;
    };

    ModelContainer() {
      auto field = std::make_shared<FieldType>("StringField");
      field->template add_constraint<TypeConstraint>(TypeFlag::String);
      fields_.emplace("string", std::move(field));
    }

    ParsingResult parse(const Source& value) {
      if (!value.is_object()) return {false};
      auto it = value.find_member("models");
      if (it != value.end_member()) {
        std::for_each((*it).value.begin_member(), (*it).value.end_member(), [&](const auto& member) {
          // parse properties
          auto properties = this->create_model_properties(member.key.get_cstr(), member.value);
          models_.emplace(member.key.get_cstr(), std::make_shared<ModelType>(std::move(properties)));
        });
      }
      return {true};
    }

    ModelPtr get_model(const std::string& name) { return models_[name]; }

  private:
    ModelPropertiesOf<Destination> create_model_properties(std::string&& name, const Source& value) {
      ModelPropertiesOf<Destination> props;
      props.name = std::move(name);

      this->get_member(value, "description", [&](const auto& item) {
        props.meta.emplace("description", item.get_cstr());
      });

      this->get_member(value, "meta", [&](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&](const auto& meta_member) {
          props.meta.emplace(meta_member.key.get_cstr(), meta_member.value.get_cstr());
        });
      });

      this->get_member(value, "fields", [&](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [&](const auto& field_value) {
          // parse the field reference.
          if (field_value.value.is_string()) {
            auto it = this->fields_.find(field_value.value.get_cstr());
            if (it == this->fields_.end()) {
              // raise a parsing error.
            }
            props.field_map.emplace(field_value.key.get_cstr(), it->second);
          } else if (field_value.value.is_object()) {
          }
        });
      });

      return props;
    }

    template<typename T>
    void get_member(const Source& value, const char* key, T&& callback) {
      auto it = value.find_member(key);
      if (it != value.end_member()) callback((*it).value);
    }

    ModelMap models_;
    FieldMap fields_;
  };

}


#endif /* end of include guard: MODELS_H */
