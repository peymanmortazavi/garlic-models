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

  protected:
    FieldReference field_;
  };


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
    using FieldConstraintType = FieldConstraint<Destination>;
    using ConstraintPtr = std::shared_ptr<ConstraintType>;
    using FieldConstraintPtr = std::shared_ptr<FieldConstraintType>;

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
        std::for_each(fields.begin_member(), fields.end_member(), [this, &context](const auto& field) {
          // parse field definition.
          this->parse_field(
            field.key.get_string(), field.value, context,
            [this, &context, &field](auto ptr, auto complete) {
              this->add_field(field.key.get_string(), context, std::move(ptr), complete);
            }
          );
        });
      });

      get_member(value, "models", [this, &context](const auto& models) {
        std::for_each(models.begin_member(), models.end_member(), [this, &context](const auto& member) {
          // parse properties.
          this->parse_model(member.key.get_string(), member.value, context, [this, &member](auto&& ptr) {
            models_.emplace(member.key.get_string(), std::move(ptr));
          });
        });
      });

      return {context.fields.size() == 0};
    }

    ModelPtr get_model(const std::string& name) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_model(const std::string& name, const Callable& cb) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) cb(it->second);
    }

    FieldPtr get_field(const std::string& name) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_field(const std::string& name, const Callable& cb) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) cb(it->second);
    }

  private:
    struct lazy_pair {
      std::string key;
      ModelPtr target;
    };

    struct field_dependency {
      std::vector<FieldPtr> dependencies;  // field inheritence.
      std::vector<lazy_pair> models;  // model field memberships.
      std::vector<std::string> fields;  // field aliases.
      std::vector<std::shared_ptr<FieldConstraintType>> constraints;  // field constraints.
    };

    struct parse_context {
      MapOf<field_dependency> fields;
    };

    template<typename Callable>
    auto parse_model(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) {
      auto ptr = std::make_shared<ModelType>(std::move(name));
      auto& props = ptr->properties_;

      get_member(value, "description", [&props](const auto& item) {
        props.meta.emplace("description", item.get_string());
      });

      get_member(value, "meta", [&props](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&props](const auto& meta_member) {
          props.meta.emplace(meta_member.key.get_string(), meta_member.value.get_string());
        });
      });

      get_member(value, "fields", [this, &props, &context, &ptr](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this, &props, &context, &ptr](const auto& field) {
          auto ready = false;
          this->parse_field("", field.value, context, [&props, &field, &ready](auto ptr, auto complete) {
            ready = true;
            props.field_map.emplace(field.key.get_cstr(), std::move(ptr));
          });
          if (!ready) {  // it must be a referenced field. add a dependency.
            context.fields[field.value.get_cstr()].models.emplace_back(lazy_pair{field.key.get_string(), ptr});
          }
        });
      });

      auto model_field = this->make_field<ModelConstraint>(std::string{ptr->get_properties().name}, ptr);
      this->add_field(ptr->get_properties().name.data(), context, std::move(model_field), true);
      cb(std::move(ptr));
    }

    template<typename Callable>
    bool parse_reference(const char* name, const parse_context& context, const Callable& cb) {
      if (auto it = this->fields_.find(name); it != this->fields_.end()) {
        if (context.fields.find(name) == context.fields.end()) {
          cb(it->second);
          return true;
        }
      } 
      return false;
    }

    template<typename Callable>
    void parse_field(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) noexcept {
      if (value.is_string()) {  // parse the field reference.
        auto ready = this->parse_reference(value.get_cstr(), context, [&cb](const auto& ptr){ cb(ptr, true); });
        if (!ready && !name.empty()) {
          context.fields[value.get_cstr()].fields.emplace_back(std::move(name));
        }
      } else if (value.is_object()) {
        auto ptr = std::make_shared<FieldType>(std::move(name));
        auto& props = ptr->properties_;
        auto complete = true;

        get_member(value, "type", [this, &props, &context, &ptr, &complete](const auto& item) {
          complete = false;
          this->parse_reference(item.get_cstr(), context, [&props, &complete](const auto& field) {
            complete = true;
            props.constraints = field->get_properties().constraints;
          });
          if (!complete) context.fields[item.get_cstr()].dependencies.emplace_back(ptr);
        });

        get_member(value, "meta", [this, &props](const auto& item) {
          for (const auto& member : item.get_object()) {
            props.meta.emplace(member.key.get_string(), member.value.get_string());
          }
        });

        get_member(value, "label", [this, &props](const auto& item) {
          props.meta.emplace("label", item.get_string());
        });

        get_member(value, "description", [this, &props](const auto& item) {
          props.meta.emplace("description", item.get_string());
        });

        get_member(value, "constraints", [this, &props, &context](const auto& item) {
          for (const auto& constraint_info : item.get_list()) {
            this->parse_constraint(constraint_info, context, [&props](auto&& ptr) {
              props.constraints.emplace_back(std::move(ptr));
            });
          }
        });

        cb(std::move(ptr), complete);
      }
    }

    void resolve_field(const std::string& key, parse_context& context, const FieldPtr& ptr) {
      if (auto it = context.fields.find(key); it != context.fields.end()) {
        // apply the constraints to the fields that inherited from this field.
        const auto& src_constraints = ptr->properties_.constraints;
        for (auto& field : it->second.dependencies) {
          auto& dst_constraints = field->properties_.constraints;
          dst_constraints.insert(dst_constraints.begin(), src_constraints.begin(), src_constraints.end());
          // if the field is a named one, it can be resolved as it is complete now.
          // anonymous fields can be skipped.
          if (!field->get_properties().name.empty()) {
            this->resolve_field(field->get_properties().name, context, field);
          }
        }

        // add the field to the depending models.
        for (auto& member : it->second.models) {
          member.target->properties_.field_map.emplace(std::move(member.key), ptr);
        }

        // register all the field aliases.
        for (auto& alias : it->second.fields) {
          this->add_field(alias.data(), context, ptr, true);
        }

        for(auto& constraint : it->second.constraints) {
          constraint->set_field(ptr);
        }

        // remove it from the context.
        context.fields.erase(it);
      }
    }

    void add_field(std::string key, parse_context& context, FieldPtr ptr, bool complete) noexcept {
      if (complete) {
        this->resolve_field(key, context, ptr);  // resolve all the dependencies.
      } else {
        context.fields[key];  // create a record so it would be deemed as incomplete.
      }
      fields_.emplace(std::move(key), std::move(ptr));  // register the field.
    }

    template<ReadableLayer Source, typename Callable>
    void parse_constraint(const Source& value, parse_context& context, const Callable& cb) noexcept {
      typedef ConstraintPtr (*ConstraintInitializer)(const Source&);
      static const std::map<std::string, ConstraintInitializer> ctors = {
        {"regex", &RegexConstraint<Destination>::parse},
        {"range", &RangeConstraint<Destination>::parse},
      };

      if (value.is_string()) {
        auto ready = this->parse_reference(
            value.get_cstr(), context,
            [this, &cb](const auto& ptr) {
              cb(this->make_field_constraint(ptr));
            }
        );
        if (!ready) {
          auto constraint = this->make_field_constraint(nullptr);
          context.fields[value.get_cstr()].constraints.emplace_back(constraint);
          cb(std::move(constraint));
        }
      } else {
        get_member(value, "type", [&value, &cb](const auto& item) {
          if (auto it = ctors.find(item.get_cstr()); it != ctors.end()) {
            cb(it->second(value));
          } else {
            // report parsing error.
          }
        });
      }

    }

    template<template<typename> typename ConstraintType, typename... Args>
    FieldPtr make_field(std::string&& name, Args&&... args) const noexcept {
      FieldPropertiesOf<Destination> props {
        std::move(name),
        {},
        {std::make_shared<ConstraintType<Destination>>(std::forward<Args>(args)...)}
      };
      return std::make_shared<FieldType>(std::move(props));
    }

    FieldConstraintPtr make_field_constraint(const FieldPtr& ptr) {
      if (ptr) {
        return std::make_shared<FieldConstraintType>(
            std::make_shared<FieldPtr>(ptr),
            ConstraintProperties{}
        );
      }
      return std::make_shared<FieldConstraintType>(
          std::make_shared<FieldPtr>(nullptr),
          ConstraintProperties{}
      );
    }

    ModelMap models_;
    FieldMap fields_;
  };

}


#endif /* end of include guard: MODELS_H */
