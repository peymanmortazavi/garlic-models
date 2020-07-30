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

    ModelConstraint(std::shared_ptr<ModelType> model) : model_(std::move(model)) {}

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
              result.details.push_back({false, member.key.get_cstr(), "find a summary of why this field value sucks.", std::move(test.failures), true});
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


  // ConstraintParsers
  //template<ReadableLayer LayerType>
  //class ConstraintParser {
  //public:
  //  using ConstraintType = Constraint<LayerType>;
  //  using ConstraintPtr = std::shared_ptr<ConstraintType>;

  //  virtual ConstraintPtr parse(const LayerType& value) const noexcept = 0;
  //  virtual const ConstraintType& get_input_constraint() const noexcept = 0;
  //};

  //template<ReadableLayer LayerType>
  //class RegexConstraintParser : public ConstraintParser<LayerType> {
  //public:
  //  using Super = ConstraintParser<LayerType>;
  //  typename Super::ConstraintPtr parse(const LayerType& value) const noexcept override {
  //    return std::shared_ptr<RegexConstraint<LayerType>>((*value.find_member("pattern")).get_cstr());
  //  }
  //  const typename Super::ConstraintType& get_input_constraint() const noexcept override {
  //    static Model<LayerType> model;
  //  }
  //};


  // Model Parsing From ReadableLayer
  template<typename T> using ModelPropertiesOf = typename Model<T>::Properties;
  template<typename T> using FieldPropertiesOf = typename Field<T>::Properties;

  template<ReadableLayer Destination>
  class ModelContainer {
  public:
    template <typename T> using MapOf = std::map<std::string, T>;
    using ModelType = Model<Destination>;
    using ModelPtr = std::shared_ptr<ModelType>;
    using FieldType = Field<Destination>;
    using FieldPtr = std::shared_ptr<FieldType>;
    using ModelMap = MapOf<ModelPtr>;
    using FieldMap = MapOf<FieldPtr>;
    using ConstraintType = Constraint<Destination>;
    using ConstraintPtr = std::shared_ptr<ConstraintType>;

    struct ParsingResult {
      bool valid;
    };

    ModelContainer() {
      static const std::map<std::string, FieldPtr> static_map = {
        {"string", this->make_field<TypeConstraint>("StringField", TypeFlag::String)},
        {"integer", this->make_field<TypeConstraint>("IntegerField", TypeFlag::Integer)},
        {"double", this->make_field<TypeConstraint>("DoubleField", TypeFlag::Double)},
        {"list", this->make_field<TypeConstraint>("ListField", TypeFlag::List)},
        {"object", this->make_field<TypeConstraint>("ObjectField", TypeFlag::Object)},
        {"bool", this->make_field<TypeConstraint>("BooleanField", TypeFlag::Boolean)},
      };
      fields_ = static_map;
    }

    ParsingResult parse(const ReadableLayer auto& value) noexcept {
      if (!value.is_object()) return {false};
      get_member(value, "fields", [this](const auto& fields) {
        std::for_each(fields.begin_member(), fields.end_member(), [this](const auto& field) {
          this->parse_field(field.value, [this,&field](auto ptr) {
            fields_.emplace(field.key.get_cstr(), std::move(ptr));
          });
        });
      });
      get_member(value, "models", [this](const auto& models) {
        std::for_each(models.begin_member(), models.end_member(), [this](const auto& member) {
          // parse properties
          auto properties = this->create_model_properties(member.key.get_cstr(), member.value);
          models_.emplace(member.key.get_cstr(), std::make_shared<ModelType>(std::move(properties)));
        });
      });
      return {true};
    }

    ModelPtr get_model(const std::string& name) noexcept { return models_[name]; }

  private:
    auto create_model_properties(std::string&& name, const ReadableLayer auto& value) -> ModelPropertiesOf<Destination> {
      ModelPropertiesOf<Destination> props;
      props.name = std::move(name);

      get_member(value, "description", [&props](const auto& item) {
        props.meta.emplace("description", item.get_cstr());
      });

      get_member(value, "meta", [&props](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&](const auto& meta_member) {
          props.meta.emplace(meta_member.key.get_cstr(), meta_member.value.get_cstr());
        });
      });

      get_member(value, "fields", [this,&props](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this,&props](const auto& field_value) {
          this->parse_field(field_value.value, [&props, &field_value](auto ptr) {
            props.field_map.emplace(field_value.key.get_cstr(), std::move(ptr));
          });
        });
      });

      return props;
    }

    template<typename Callable>
    void parse_field(const ReadableLayer auto& value, const Callable& cb) noexcept {
      // parse the field reference.
      if (value.is_string()) {
        if (auto it = this->fields_.find(value.get_cstr()); it != this->fields_.end()) {
          cb(it->second);
        } else {
          // now search the models for this field name.
          if (!this->try_create_model_field(value.get_cstr(), cb)) {
            // raise a parsing error.
          }
        }
      } else if (value.is_object()) {
        FieldPropertiesOf<Destination> props;
        get_member(value, "type", [this, &props, &value](const auto& item) {
          if (auto it = this->fields_.find(item.get_cstr()); it != this->fields_.end()) {
            props.constraints = it->second->get_properties().constraints;
          } else {
            auto found = this->try_create_model_field(item.get_cstr(), [&props](const auto& field) {
              props.constraints = field->get_properties().constraints;
            });
            if (!found) {
              // parsing error.
            }
          }
        });
        get_member(value, "meta", [this,&props](const auto& item) {
          for (const auto& member : item.get_object()) {
            props.meta.emplace(member.key.get_cstr(), member.value.get_cstr());
          }
        });
        get_member(value, "label", [this,&props](const auto& item) {
          props.meta.emplace("label", item.get_cstr());
        });
        get_member(value, "description", [this,&props](const auto& item) {
          props.meta.emplace("description", item.get_cstr());
        });
        get_member(value, "constraints", [this,&props](const auto& item) {
          for (const auto& constraint_info : item.get_list()) {
            this->parse_constraint(constraint_info, [&props](auto&& ptr) {
              props.constraints.emplace_back(std::move(ptr));
            });
          }
        });
        // if the order matters, people can use a field constraint that basically forwards field constraint result.
        cb(std::make_shared<FieldType>(std::move(props)));
      }
    }

    template<ReadableLayer Source, typename Callable>
    void parse_constraint(const Source& value, const Callable& cb) const noexcept {
      typedef ConstraintPtr (*ConstraintInitializer)(const Source&);
      static const std::map<std::string, ConstraintInitializer> ctors = {
        {"regex", &RegexConstraint<Destination>::parse},
        {"range", &RangeConstraint<Destination>::parse},
      };

      get_member(value, "type", [&value,&cb](const auto& item) {
        if (auto it = ctors.find(item.get_cstr()); it != ctors.end()) {
          cb(it->second(value));
        } else {
          // report parsing error.
        }
      });
    }

    /**
     * Search the existing models, if found, create and save a field matching that model.
     */
    template<typename Callable>
    bool try_create_model_field(const char* name, const Callable& cb) noexcept {
      if (auto it = this->models_.find(name); it != this->models_.end()) {
        auto model_field = this->make_field<ModelConstraint>(name, it->second);
        this->fields_.emplace(name, model_field);
        cb(std::move(model_field));
        return true;
      }
      return false;
    }

    template<template<typename> typename ConstraintType, typename... Args>
    FieldPtr make_field(std::string&& name, Args&&... args) {
      FieldPropertiesOf<Destination> props {
        std::move(name),
        {},
        {std::make_shared<ConstraintType<Destination>>(std::forward<Args>(args)...)}
      };
      return std::make_shared<FieldType>(std::move(props));
    }

    ModelMap models_;
    FieldMap fields_;
  };

}


#endif /* end of include guard: MODELS_H */
