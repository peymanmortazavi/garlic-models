#ifndef GARLIC_MODULE_H
#define GARLIC_MODULE_H

#include "models.h"
#include "layer.h"


namespace garlic {

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

      get_member(value, "fields", [this, &context](const auto &fields) {
        for (const auto &field : fields.get_object()) {
          // parse field definition.
          this->parse_field(
              field.key.get_string(), field.value, context,
              [this, &context, &field](auto ptr, auto complete) {
                this->add_field(field.key.get_string(), context,
                                std::move(ptr), complete);
              });
        }
      });

      get_member(value, "models", [this, &context](const auto& models) {
        for(const auto& model : models.get_object()) {
          // parse properties.
          this->parse_model(
              model.key.get_string(), model.value, context,
              [this, &model](auto&& ptr) {
                models_.emplace(model.key.get_string(), std::move(ptr));
              });
        }
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

    struct parser {
      parse_context& context;
      ModelContainer& module;

      parser(
          parse_context& context,
          ModelContainer& module
      ) : context(context), module(module) {}

      template<ReadableLayer Source, typename Callable>
      void parse_constraint(const Source& value, const Callable& cb) {
        module.parse_constraint(value, context, cb);
      }

      FieldPtr resolve_field_reference(const char* name) {
        FieldPtr result = nullptr;
        module.parse_reference(name, context, [&result](const auto& ptr) {
            result = ptr;
        });
        return result;
      }

      void add_field_dependency(const char* name, FieldConstraintPtr constraint) {
        context.fields[name].constraints.emplace_back(constraint);
      }
    };

    void process_model_meta(ModelPropertiesOf<Destination>& props, const ReadableLayer auto& value) {
      get_member(value, "description", [&props](const auto& item) {
        props.meta.emplace("description", item.get_string());
      });

      get_member(value, "meta", [&props](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&props](const auto& meta_member) {
          props.meta.emplace(meta_member.key.get_string(), meta_member.value.get_string());
        });
      });
    }

    template<typename Callable>
    void parse_model(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) {
      auto ptr = std::make_shared<ModelType>(std::move(name));
      auto& props = ptr->properties_;

      this->process_field_meta(props, value);

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

    void process_field_meta(FieldPropertiesOf<Destination>& props, const ReadableLayer auto& value) {
      get_member(value, "meta", [&props](const auto& item) {
        for (const auto& member : item.get_object()) {
          props.meta.emplace(member.key.get_string(), member.value.get_string());
        }
      });

      auto add_meta_field = [&value, &props](const char* name) {
        get_member(value, name, [&props, name](const auto& item) {
          props.meta.emplace(name, item.get_string());
        });
      };

      add_meta_field("label");
      add_meta_field("description");
    }

    template<typename Callable>
    void parse_field(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) noexcept {
      if (value.is_string()) {  // parse the field reference.
        auto ready = this->parse_reference(value.get_cstr(), context, [&cb](const auto& ptr){ cb(ptr, true); });
        if (!ready && !name.empty()) {
          context.fields[value.get_cstr()].fields.emplace_back(std::move(name));
        }
        return;
      }
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

      this->process_field_meta(props, value);

      get_member(value, "constraints", [this, &props, &context](const auto& item) {
        for (const auto& constraint_info : item.get_list()) {
          this->parse_constraint(constraint_info, context, [&props](auto&& ptr) {
            props.constraints.emplace_back(std::move(ptr));
          });
        }
      });

      cb(std::move(ptr), complete);
    }

    void resolve_field(const std::string& key, parse_context& context, const FieldPtr& ptr) {
      if (auto it = context.fields.find(key); it != context.fields.end()) {
        // apply the constraints to the fields that inherited from this field.
        const auto& src_constraints = ptr->properties_.constraints;
        for (auto& field : it->second.dependencies) {
          auto& dst_constraints = field->properties_.constraints;
          dst_constraints.insert(
              dst_constraints.begin(),
              src_constraints.begin(), src_constraints.end()
              );
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
      typedef ConstraintPtr (*ConstraintInitializer)(const Source&, parser);
      static const std::map<std::string, ConstraintInitializer> ctors = {
        {"regex", &RegexConstraint<Destination>::parse},
        {"range", &RangeConstraint<Destination>::parse},
        {"field", &FieldConstraint<Destination>::parse},
        {"any", &AnyConstraint<Destination>::parse},
      };

      if (value.is_string()) {
        auto ready = this->parse_reference(value.get_cstr(), context,
            [this, &cb](const auto& ptr) {
              cb(this->make_field_constraint(ptr));
            });
        if (!ready) {
          auto constraint = this->make_field_constraint(nullptr);
          context.fields[value.get_cstr()].constraints.emplace_back(constraint);
          cb(std::move(constraint));
        }
      } else {
        get_member(value, "type",
            [this, &value, &context, &cb](const auto& item) {
              if (auto it = ctors.find(item.get_cstr()); it != ctors.end()) {
                cb(it->second(value, parser(context, *this)));
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


#endif /* end of include guard: GARLIC_MODULE_H */
