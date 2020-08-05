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
    template<garlic::ReadableLayer> friend class ModelContainer;

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
    template<garlic::ReadableLayer> friend class ModelContainer;

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
      
      parse_context context;

      get_member(value, "fields", [this, &context](const auto& fields) {
        std::for_each(fields.begin_member(), fields.end_member(), [this,&context](const auto& field) {
          this->parse_field(field.value, context, [this,&context,&field](auto ptr) {
            this->add_field(field.key.get_cstr(), context, std::move(ptr));
          });
        });
      });

      get_member(value, "models", [this,&context](const auto& models) {
        std::for_each(models.begin_member(), models.end_member(), [this,&context](const auto& member) {
          // parse properties
          this->parse_model(member.key.get_string(), member.value, context, [this,&member](auto&& ptr) {
            models_.emplace(member.key.get_string(), std::move(ptr));
          });
        });
      });

      return {true};
    }

    ModelPtr get_model(const std::string& name) noexcept { return models_[name]; }

  private:
    struct forward_model_field {
      std::string key;
      ModelPtr model;
    };

    struct forward_decleration {
      std::vector<FieldPtr> dependencies;
      std::vector<forward_model_field> model_fields;
    };

    struct parse_context {
      MapOf<forward_decleration> fields;
    };

    template<typename Callable>
    auto parse_model(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) {
      auto ptr = std::make_shared<ModelType>(ModelPropertiesOf<Destination>{std::move(name)});
      auto& props = ptr->properties_;

      get_member(value, "description", [&props](const auto& item) {
        props.meta.emplace("description", item.get_cstr());
      });

      get_member(value, "meta", [&props](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&](const auto& meta_member) {
          props.meta.emplace(meta_member.key.get_cstr(), meta_member.value.get_cstr());
        });
      });

      get_member(value, "fields", [this,&props,&context,&ptr](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this,&props,&context,&ptr](const auto& field_value) {
          auto found = false;
          this->parse_field(field_value.value, context, [&props, &field_value, &found](auto ptr) {
           props.field_map.emplace(field_value.key.get_cstr(), std::move(ptr));
           found = true;
          });
          if (!found) {
            context.fields[field_value.value.get_cstr()].model_fields.emplace_back(forward_model_field{field_value.key.get_string(), ptr});
          }
        });
      });

      auto model_field = this->make_field<ModelConstraint>(std::string{ptr->get_properties().name}, ptr);
      this->add_field(ptr->get_properties().name.data(), context, std::move(model_field));
      cb(std::move(ptr));
    }

    template<typename Callable>
    void parse_reference(const char* name, const Callable& cb) {
      if (auto it = this->fields_.find(name); it != this->fields_.end()) {
        cb(it->second);
      } 
    }

    template<typename Callable>
    void parse_field(const ReadableLayer auto& value, parse_context& context, const Callable& cb) noexcept {
      // this maybe shouldn't add fields to permanent when it depends on some forward declared type.
      // maybe we should create a dependency graph to resolve the fields.
      // a depends on c, b depends on a and c depends on nothing.
      // when a field doesnt have forward declared values then back track its dependencies.
      // parse the field reference.
      if (value.is_string()) {
        this->parse_reference(value.get_cstr(), cb);
      } else if (value.is_object()) {
        auto ptr = std::make_shared<FieldType>(FieldPropertiesOf<Destination>{});
        auto& props = ptr->properties_;
        get_member(value, "type", [this, &props, &context, &ptr](const auto& item) {
          auto is_forward = true;
          this->parse_reference(item.get_cstr(), [&props, &is_forward](const auto& field) {
            is_forward = false;
            props.constraints = field->get_properties().constraints;
          });
          if (is_forward) context.fields[item.get_cstr()].dependencies.emplace_back(ptr);
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
        cb(std::move(ptr));
      }
    }

    void add_field(const char* key, parse_context& context, FieldPtr&& ptr) {
      if (auto it = context.fields.find(key); it != context.fields.end()) {
        auto src_constraints = ptr->properties_.constraints;
        for (auto& d : it->second.dependencies) {
          auto& dst_constraints = d->properties_.constraints;
          dst_constraints.insert(dst_constraints.begin(), src_constraints.begin(), src_constraints.end());
        }
        for (auto& m : it->second.model_fields) {
          printf("updating the model field by its key : %s\n", m.key.c_str());
          m.model->properties_.field_map.emplace(std::move(m.key), ptr);
        }
        context.fields.erase(it);
        // install it on all the dependencies.
      }
      fields_.emplace(key, std::move(ptr));
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
