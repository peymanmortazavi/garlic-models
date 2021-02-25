#ifndef GARLIC_MODULE_H
#define GARLIC_MODULE_H

#include <iterator>
#include <unordered_map>
#include "constraints.h"
#include "models.h"
#include "layer.h"
#include "parsing/constraints.h"
#include "utility.h"


namespace garlic {

  template<ViewLayer Destination>
  class Module {
  public:
    template <typename Key, typename Value> using table = std::unordered_map<Key, Value>;
    using model_type               = Model<Destination>;
    using model_pointer            = std::shared_ptr<model_type>;
    using field_type               = Field<Destination>;
    using field_pointer            = std::shared_ptr<field_type>;
    using model_table              = table<text, model_pointer>;
    using field_table              = table<text, field_pointer>;
    using constraint_type          = Constraint<Destination>;
    using field_constraint_type    = FieldConstraint<Destination>;
    using constraint_pointer       = std::shared_ptr<constraint_type>;
    using field_constraint_pointer = std::shared_ptr<field_constraint_type>;

    constexpr static auto kDefaultFieldTableSize = 16;

    struct ParsingResult {
      bool valid;
    };

    Module() : fields_(kDefaultFieldTableSize) {
      static table<text, field_pointer> static_map = {
        {"string", this->make_field<TypeConstraint>("StringField", TypeFlag::String)},
        {"integer", this->make_field<TypeConstraint>("IntegerField", TypeFlag::Integer)},
        {"double", this->make_field<TypeConstraint>("DoubleField", TypeFlag::Double)},
        {"list", this->make_field<TypeConstraint>("ListField", TypeFlag::List)},
        {"object", this->make_field<TypeConstraint>("ObjectField", TypeFlag::Object)},
        {"bool", this->make_field<TypeConstraint>("BooleanField", TypeFlag::Boolean)},
      };
      fields_ = static_map;
    }

    template<ViewLayer Layer>
    ParsingResult parse(Layer&& layer) noexcept {
      if (!layer.is_object()) return {false};
      
      parse_context context;

      get_member(layer, "fields", [this, &context](const auto& value) {
        for (const auto& field : value.get_object()) {
          // parse field definition.
          this->parse_field(
              decode<text>(field.key), field.value, context,
              [this, &context, &field](auto ptr, auto complete, auto) {
                this->add_field(decode<text>(field.key).deep_copy(), context, std::move(ptr), complete);
              },
              [](const auto&, auto) {});
        }
      });

      get_member(layer, "models", [this, &context](const auto& value) {
        for(const auto& model : value.get_object()) {
          // parse properties.
          this->parse_model(
              decode<text>(model.key), model.value, context,
              [this, &model](auto&& ptr) {
                models_.emplace(ptr->get_name(), std::move(ptr));
              });
        }
      });

      return {context.fields.size() == 0};
    }

    model_pointer get_model(const text& name) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_model(const text& name, Callable&& cb) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) cb(it->second);
    }

    field_pointer get_field(const text& name) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_field(const text& name, Callable&& cb) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) cb(it->second);
    }

  private:
    struct lazy_pair {
      text key;
      model_pointer target;
      bool required;
    };

    struct field_dependency {
      sequence<field_pointer> dependencies;  // field inheritence.
      sequence<lazy_pair> models;  // model field memberships.
      sequence<text> fields;  // field aliases.
      sequence<field_constraint_pointer> constraints;  // field constraints.
    };

    struct lazy_model_field {
      text name;
      bool required;
    };

    struct parse_context {
      table<text, field_dependency> fields;
      table<model_pointer, table<text, lazy_model_field>> lazy_model_fields;

      void add_lazy_model_field(const text& field, const text& key, model_pointer ptr, bool required) {
        lazy_model_fields[ptr].emplace(key.copy(), lazy_model_field {.name = field.copy(), .required = required});
        fields[field].models.push_back(
            lazy_pair {
              .key = key.copy(),
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

      template<ViewLayer Source, typename Callable>
      void parse_constraint(const Source& value, const Callable& cb) {
        module.parse_constraint(value, context, cb);
      }

      field_pointer resolve_field_reference(const char* name) {
        field_pointer result = nullptr;
        module.parse_reference(name, context, [&result](const auto& ptr) {
            result = ptr;
        });
        return result;
      }

      void add_field_dependency(const char* name, field_constraint_pointer constraint) {
        context.fields[name].constraints.push_back(constraint);
      }
    };

    void process_model_meta(ModelPropertiesOf<Destination>& props, const ViewLayer auto& value) {
      get_member(value, "description", [&props](const auto& item) {
        props.meta.emplace("description", decode<text>(item).deep_copy());
      });

      get_member(value, "meta", [&props](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&props](const auto& meta_member) {
          props.meta.emplace(
              decode<text>(meta_member.key).deep_copy(),
              decode<text>(meta_member.value).deep_copy());
        });
      });
    }

    void process_model_inheritance(const model_pointer& model, parse_context& context, const ViewLayer auto& value) {
      enum field_status : uint8_t { lazy = 1, available = 2, exclude = 3 };
      struct field_info {
        field_status status;
        lazy_model_field lazy_field;
        typename model_type::field_descriptor descriptor;
      };
      table<text, field_info> field_table;
      auto& props = model->properties_;
      auto apply_inheritance = [this, &context, &field_table, &model](text&& model_name) {
          auto it = this->models_.find(model_name);
          if (it == this->models_.end()) {}  // report parsing error here.
          auto& model_field_map = it->second->properties_.field_map;
          for (const auto& item : model_field_map) {
            auto& info = field_table[item.first];
            if (info.status == field_status::exclude)
              continue;
            info.status = field_status::available;
            info.descriptor = item.second;
          }
          find(context.lazy_model_fields, it->second, [&field_table](const auto& lazy_field_map) {
              for (const auto& item : lazy_field_map.second) {
                auto& info = field_table[item.first];
                if (info.status == field_status::exclude)
                  continue;
                info.status = field_status::lazy;
                info.lazy_field = item.second;
              }
          });
      };
      get_member(value, "exclude_fields", [&field_table](const auto& excludes) {
          for (const auto& field : excludes.get_list()) {
            field_table[decode<text>(field)].status = field_status::exclude;
          }
      });
      get_member(value, "inherit", [&apply_inheritance](const auto& inherit) {
          if (inherit.is_string()) {
            apply_inheritance(decode<text>(inherit));
            return;
          }
          for (const auto& model_name : inherit.get_list()) {
            apply_inheritance(decode<text>(model_name));
          }
      });
      for (const auto& item : field_table) {
        switch (item.second.status) {
          case field_status::lazy:
            context.add_lazy_model_field(
                item.second.lazy_field.name,
                item.first,
                model,
                item.second.lazy_field.required);
            break;
          case field_status::available:
            props.field_map.emplace(item.first.deep_copy(), item.second.descriptor);
            break;
          default:
            continue;
        }
      }
    }

    template<typename Callable>
    void parse_model(const text& name, const ViewLayer auto& layer, parse_context& context, const Callable& cb) {
      auto ptr = std::make_shared<model_type>(name.deep_copy());
      auto& props = ptr->properties_;

      this->process_model_meta(props, layer);
      this->process_model_inheritance(ptr, context, layer);

      get_member(layer, "fields", [this, &props, &context, &ptr](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this, &props, &context, &ptr](const auto& field) {
          this->parse_field(text::no_text(), field.value, context,
              [&props, &field](auto ptr, auto complete, auto optional) {
            props.field_map[decode<text>(field.key).deep_copy()] = {.field = std::move(ptr), .required = !optional};
          }, [&field, &context, &ptr](const text& name, auto optional) {
            context.add_lazy_model_field(name, decode<text>(field.key), ptr, !optional);
          });
        });
      });

      auto model_field = this->make_field<ModelConstraint>(ptr->get_name(), ptr);
      this->add_field(ptr->get_name(), context, std::move(model_field), true);
      cb(std::move(ptr));
    }

    template<typename Callable>
    bool parse_reference(const text& name, const parse_context& context, Callable&& cb) {
      if (auto it = this->fields_.find(name); it != this->fields_.end()) {
        if (context.fields.find(name) == context.fields.end()) {
          cb(it->second);
          return true;
        }
      }
      return false;
    }

    void process_field_meta(FieldPropertiesOf<Destination>& props, const ViewLayer auto& layer) {
      get_member(layer, "meta", [&props](const auto& item) {
        for (const auto& member : item.get_object()) {
          props.meta.emplace(
              decode<text>(member.key).deep_copy(),
              decode<text>(member.value).deep_copy());
        }
      });

      auto add_meta_field = [&layer, &props](text&& name) {
        get_member(layer, name.data(), [&props, &name](const auto& item) {
          // since name is from stack, no need to copy its content.
          props.meta.emplace(name.copy(), decode<text>(item).deep_copy());
        });
      };

      add_meta_field("label");
      add_meta_field("description");
      add_meta_field("message");
    }

    template<typename SuccessCallable, typename FailCallable>
    void parse_field(
        text&& name, const ViewLayer auto& layer,
        parse_context& context, SuccessCallable&& cb,
        FailCallable&& fcb
        ) noexcept {
      auto optional = false;

      if (layer.is_string()) {  // parse the field reference.
        auto field_name = decode<text>(layer);
        if (field_name.back() == '?') {
          field_name.pop_back();
          optional = true;
        }
        auto ready = this->parse_reference(
            field_name, context, [&cb, &optional](const auto& ptr) {
              cb(ptr, true, optional);
              });
        if (!ready) {
          if (!name.empty()) {
            context.fields[field_name].fields.push_back(name.copy());
          }
          fcb(field_name.copy(), optional);
        }
        return;
      }
      auto ptr = std::make_shared<field_type>(name.deep_copy());
      auto& props = ptr->properties_;
      auto complete = true;

      get_member(layer, "type", [this, &props, &context, &ptr, &complete](const auto& value) {
        complete = false;
        auto reference_name = decode<text>(value);
        this->parse_reference(reference_name, context, [&props, &complete](const auto& field) {
          complete = true;
          const auto& base_props = field->get_properties().constraints;
          std::copy(std::begin(base_props), std::end(base_props), std::back_inserter(props.constraints));
        });
        if (!complete)
          context.fields[reference_name.copy()].dependencies.push_back(ptr);
      });

      this->process_field_meta(props, layer);

      get_member(layer, "constraints", [this, &props, &context](const auto& value) {
        for (const auto& constraint_info : value.get_list()) {
          this->parse_constraint(constraint_info, context, [&props](auto&& ptr) {
            props.constraints.push_back(std::move(ptr));
          });
        }
      });

      get_member(layer, "ignore_details", [&props](const auto& value){
          props.ignore_details = value.get_bool();
          });

      get_member(layer, "optional", [&optional](const auto& value) {
          optional = value.get_bool();
          });

      cb(std::move(ptr), complete, optional);
    }

    void resolve_field(const text& key, parse_context& context, const field_pointer& ptr) {
      if (auto it = context.fields.find(key); it != context.fields.end()) {
        // apply the constraints to the fields that inherited from this field.
        const auto& src_constraints = ptr->properties_.constraints;
        for (auto& field : it->second.dependencies) {
          auto& dst_constraints = field->properties_.constraints;
          dst_constraints.push_front(src_constraints.begin(), src_constraints.end());
          // if the field is a named one, it can be resolved as it is complete now.
          // anonymous fields can be skipped.
          if (!field->get_name().empty()) {
            this->resolve_field(field->get_name(), context, field);
          }
        }

        // add the field to the depending models.
        for (auto& member : it->second.models) {
          member.target->properties_.field_map.emplace(
              member.key.deep_copy(),
              typename model_type::field_descriptor{.field = ptr, .required = member.required}
              );
        }

        // register all the field aliases.
        for (auto& alias : it->second.fields) {
          this->add_field(alias.deep_copy(), context, ptr, true);
        }

        for(auto& constraint : it->second.constraints) {
          constraint->set_field(ptr);
        }

        // remove it from the context.
        context.fields.erase(it);
      }
    }

    void add_field(text&& key, parse_context& context, field_pointer ptr, bool complete) noexcept {
      if (complete) {
        this->resolve_field(key, context, ptr);  // resolve all the dependencies.
      } else {
        context.fields[key];  // create a record so it would be deemed as incomplete.
      }
      fields_.emplace(std::move(key), std::move(ptr));  // register the field.
    }

    template<ViewLayer Source, typename Callable>
    void parse_constraint(const Source& value, parse_context& context, const Callable& cb) noexcept {
      typedef constraint_pointer (*ConstraintInitializer)(const Source&, parser);
      static const table<text, ConstraintInitializer> ctors = {
        {"regex", &parsing::parse_regex<Destination, Source>},
        {"range", &parsing::parse_range<Destination, Source>},
        {"field", &parsing::parse_field<Destination, Source>},
        {"any", &parsing::parse_any<Destination, Source>},
        {"all", &parsing::parse_all<Destination, Source>},
        {"list", &parsing::parse_list<Destination, Source>},
        {"tuple", &parsing::parse_tuple<Destination, Source>},
        {"map", &parsing::parse_map<Destination, Source>},
        {"literal", &parsing::parse_literal<Destination, Source>},
      };

      if (value.is_string()) {
        auto reference_key = decode<text>(value);
        auto ready = this->parse_reference(reference_key, context,
            [this, &cb](const auto& ptr) {
              cb(this->make_field_constraint(ptr));
            });
        if (!ready) {
          auto constraint = this->make_field_constraint(nullptr);
          context.fields[std::move(reference_key)].constraints.push_back(constraint);
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
    field_pointer make_field(text&& name, Args&&... args) const noexcept {
      sequence<constraint_pointer> constraints;
      constraints.push_back(std::make_shared<ConstraintType<Destination>>(std::forward<Args>(args)...));
      return std::make_shared<field_type>(FieldPropertiesOf<Destination> {
          std::move(name),
          {},
          std::move(constraints)
          });
    }

    field_constraint_pointer make_field_constraint(const field_pointer& ptr) {
      if (ptr) {
        return std::make_shared<field_constraint_type>(
            std::make_shared<field_pointer>(ptr),
            ConstraintProperties::create_default(),
            true
        );
      }
      return std::make_shared<field_constraint_type>(
          std::make_shared<field_pointer>(nullptr),
          ConstraintProperties::create_default(),
          true
      );
    }

    model_table models_;
    field_table fields_;
  };
}


#endif /* end of include guard: GARLIC_MODULE_H */
