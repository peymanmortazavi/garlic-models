#ifndef GARLIC_MODULE_H
#define GARLIC_MODULE_H

#include "models.h"
#include "layer.h"
#include "parsing/constraints.h"


namespace garlic {

  template<ReadableLayer Destination>
  class Module {
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

    Module() {
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
              [this, &context, &field](auto ptr, auto complete, auto) {
                this->add_field(field.key.get_string(), context,
                                std::move(ptr), complete);
              }, [](auto&&, auto){});
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
      bool required;
    };

    struct field_dependency {
      std::vector<FieldPtr> dependencies;  // field inheritence.
      std::vector<lazy_pair> models;  // model field memberships.
      std::vector<std::string> fields;  // field aliases.
      std::vector<std::shared_ptr<FieldConstraintType>> constraints;  // field constraints.
    };

    struct lazy_model_field {
      std::string name;
      bool required;
    };

    struct parse_context {
      MapOf<field_dependency> fields;
      std::map<ModelPtr, MapOf<lazy_model_field>> lazy_model_fields;

      void add_lazy_model_field(const std::string& field, const std::string& key, ModelPtr ptr, bool required) {
        lazy_model_fields[ptr].emplace(key, lazy_model_field{.name = field, .required = required});
        fields[field].models.emplace_back(
            lazy_pair{
              .key = key,
              .target = std::move(ptr),
              .required = required
            });
      }
    };

    struct parser {
      parse_context& context;
      Module& module;

      parser(
          parse_context& context,
          Module& module
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

    void process_model_inheritance(const ModelPtr& model, parse_context& context, const ReadableLayer auto& value) {
      enum FieldStatus : uint8_t { lazy = 1, available = 2, exclude = 3 };
      struct field_info {
        FieldStatus status;
        lazy_model_field lazy_field;
        typename ModelType::FieldDescriptor descriptor;
      };
      MapOf<field_info> field_table;
      auto& props = model->properties_;
      auto apply_inheritance = [this, &context, &field_table, &model](const auto& model_name) {
          auto it = this->models_.find(model_name);
          if (it == this->models_.end()) {}  // report parsing error here.
          auto& field_map = it->second->properties_.field_map;
          for (const auto& item : field_map) {
            auto& info = field_table[item.first];
            if (info.status == FieldStatus::exclude) continue;
            info.status = FieldStatus::available;
            info.descriptor = item.second;
          }
          get(context.lazy_model_fields, it->second, [&field_table](const auto& lazy_field_map) {
              for (const auto& item : lazy_field_map.second) {
                auto& info = field_table[item.first];
                if (info.status == FieldStatus::exclude) continue;
                info.status = FieldStatus::lazy;
                info.lazy_field = item.second;
              }
          });
      };
      get_member(value, "exclude_fields", [&field_table](const auto& excludes) {
          for (const auto& field : excludes.get_list()) {
            field_table[field.get_string()].status = FieldStatus::exclude;
          }
      });
      get_member(value, "inherit", [&apply_inheritance](const auto& inherit) {
          if (inherit.is_string()) {
            apply_inheritance(inherit.get_string());
            return;
          }
          for (const auto& model_name : inherit.get_list()) {
            apply_inheritance(model_name.get_string());
          }
      });
      for (const auto& item : field_table) {
        switch (item.second.status) {
          case FieldStatus::lazy:
            context.add_lazy_model_field(
                item.second.lazy_field.name,
                item.first,
                model,
                item.second.lazy_field.required);
            break;
          case FieldStatus::available:
            props.field_map.emplace(item.first, item.second.descriptor);
            break;
          default:
            continue;
        }
      }
    }

    template<typename Callable>
    void parse_model(std::string&& name, const ReadableLayer auto& value, parse_context& context, const Callable& cb) {
      auto ptr = std::make_shared<ModelType>(std::move(name));
      auto& props = ptr->properties_;

      this->process_model_meta(props, value);
      this->process_model_inheritance(ptr, context, value);

      get_member(value, "fields", [this, &props, &context, &ptr](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this, &props, &context, &ptr](const auto& field) {
          this->parse_field("", field.value, context,
              [&props, &field](auto ptr, auto complete, auto optional) {
            props.field_map[field.key.get_cstr()] = {.field = std::move(ptr), .required = !optional};
          }, [&field, &context, &ptr](auto&& name, auto optional) {
            context.add_lazy_model_field(std::move(name), field.key.get_string(), ptr, !optional);
          });
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
      add_meta_field("message");
    }

    template<typename SuccessCallable, typename FailCallable>
    void parse_field(
        std::string&& name, const ReadableLayer auto& value,
        parse_context& context,const SuccessCallable& cb,
        const FailCallable& fcb = [](auto, auto){}
        ) noexcept {
      auto optional = false;

      if (value.is_string()) {  // parse the field reference.
        auto field_name = value.get_string();
        if (field_name.back() == '?') {
          field_name.pop_back();
          optional = true;
        }
        auto ready = this->parse_reference(
            field_name.c_str(), context, [&cb, &optional](const auto& ptr) {
              cb(ptr, true, optional);
              });
        if (!ready) {
          if (!name.empty()) {
            context.fields[field_name].fields.emplace_back(std::move(name));
          }
          fcb(std::move(field_name), optional);
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

      get_member(value, "optional", [&optional](const auto& item) {
          optional = item.get_bool();
          });

      cb(std::move(ptr), complete, optional);
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
          member.target->properties_.field_map.emplace(
              std::move(member.key),
              typename ModelType::FieldDescriptor{.field = ptr, .required = member.required}
              );
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
      fields_[std::move(key)] = std::move(ptr);  // register the field.
    }

    template<ReadableLayer Source, typename Callable>
    void parse_constraint(const Source& value, parse_context& context, const Callable& cb) noexcept {
      typedef ConstraintPtr (*ConstraintInitializer)(const Source&, parser);
      static const std::map<std::string, ConstraintInitializer> ctors = {
        {"regex", &parsing::parse_regex<Destination>},
        {"range", &parsing::parse_range<Destination>},
        {"field", &parsing::parse_field<Destination>},
        {"any", &parsing::parse_any<Destination>},
        {"all", &parsing::parse_all<Destination>},
        {"list", &parsing::parse_list<Destination>},
        {"tuple", &parsing::parse_tuple<Destination>},
        {"map", &parsing::parse_map<Destination>},
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
            ConstraintProperties{},
            true
        );
      }
      return std::make_shared<FieldConstraintType>(
          std::make_shared<FieldPtr>(nullptr),
          ConstraintProperties{},
          true
      );
    }

    ModelMap models_;
    FieldMap fields_;
  };
}


#endif /* end of include guard: GARLIC_MODULE_H */
