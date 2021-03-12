#ifndef GARLIC_PARSING_MODULE
#define GARLIC_PARSING_MODULE

#include <memory>
#include "flat_constraints.h"
#include "../flat_module.h"
#include "garlic/flat_constraints.h"
#include "garlic/lib/expected.h"
#include "garlic/utility.h"

namespace garlic::parsing {

  class ModuleParser {
  private:
    template<typename Key, typename Value>
    using table         = std::unordered_map<Key, Value>;

    using field_pointer = std::shared_ptr<FlatField>;
    using model_pointer = std::shared_ptr<FlatModel>;

    struct model_field {
      text key;
      model_pointer model;
      bool required;
    };

    struct field_dependent_record {
      sequence<field_pointer> fields;
      sequence<model_field> models;
      sequence<text> aliases;
      sequence<FlatConstraint> constraints;  // field constraints.
    };

    struct deferred_field_record {
      text field_name;
      bool required;
    };

    /*
     * This object allows for external parsers to use some of the internal
     * parsing functions.
     */
    struct parser_ambassador {
      ModuleParser& parser;

      template<GARLIC_VIEW Layer, typename Callable>
      inline void parse_constraint(const Layer& value, Callable&& cb) {
        parser.parse_constraint(value, cb);
      }

      field_pointer find_field(const text& name) {
        field_pointer result = nullptr;
        parser.find_field(name, [&result](const auto& ptr) {
            result = ptr;
        });
        return result;
      }

      void add_field_dependency(text&& name, FlatConstraint constraint) {
        parser.field_dependents_[name].constraints.push_back(constraint);
      }
    };

    table<text, field_dependent_record> field_dependents_;  // field to its dependents.
    table<model_pointer, table<text, deferred_field_record>> model_deferred_fields_;
    FlatModule& module_;

    template<GARLIC_VIEW Input>
    void process_field_meta(FlatField& field, Input&& layer) {
      get_member(layer, "meta", [&field](const auto& item) {
        for (const auto& member : item.get_object()) {
          field.meta().emplace(
              decode<text>(member.key).clone(),
              decode<text>(member.value).clone());
        }
      });

      auto add_meta_field = [&layer, &field](text&& name) {
        get_member(layer, name.data(), [&field, &name](const auto& item) {
          // since name is from stack, no need to copy its content.
          field.meta().emplace(std::move(name), decode<text>(item).clone());
        });
      };

      add_meta_field("label");
      add_meta_field("description");
      add_meta_field("message");
    }

    template<GARLIC_VIEW Layer>
    void process_model_meta(FlatModel& model, Layer&& layer) {
      get_member(layer, "description", [&model](const auto& item) {
        model.meta().emplace("description", decode<text>(item).clone());
      });

      get_member(layer, "meta", [&model](const auto& item) {
        std::for_each(item.begin_member(), item.end_member(), [&model](const auto& meta_member) {
          model.meta().emplace(
              decode<text>(meta_member.key).clone(),
              decode<text>(meta_member.value).clone());
        });
      });
    }

    template<GARLIC_VIEW Layer, typename SuccessCallable, typename FailCallable>
    void parse_field(const text& name, Layer&& layer, SuccessCallable&& ok, FailCallable&& fail) noexcept {
      if (layer.is_string()) {  // it is an alias, just process the referenced field.
        parse_field_reference(name, layer, ok, fail);
        return;
      }

      auto ptr = std::make_shared<FlatField>(name.view());
      auto complete = true;

      get_member(layer, "type", [this, &ptr, &complete](const auto& value) {
        auto reference_name = decode<text>(value);
        complete = this->find_field(reference_name, [&ptr](const auto& field) {
            ptr->inherit_constraints_from(*field);
        });
        if (!complete)
          field_dependents_[reference_name].fields.push_back(ptr);
      });

      this->process_field_meta(*ptr, layer);

      get_member(layer, "constraints", [this, &ptr](const auto& value) {
        for (const auto& constraint_info : value.get_list()) {
          this->parse_constraint(constraint_info, [&ptr](auto&& constraint) {
              ptr->add_constraint(std::move(constraint));
          });
        }
      });

      get_member(layer, "ignore_details", [&ptr](const auto& value){
          ptr->set_ignore_details(value.get_bool());
          });

      auto optional = get(layer, "optional", false);

      ok(std::move(ptr), complete, optional);
    }

    template<GARLIC_VIEW Layer, typename SuccessCallable, typename FailCallable>
    void parse_field_reference(const text& name, Layer&& layer, SuccessCallable&& ok, FailCallable&& fail) noexcept {
      auto field_name = decode<text>(layer);
      auto optional = false;
      if (field_name.back() == '?') {
        field_name.pop_back();
        optional = true;
      }
      auto ready = this->find_field(
          field_name, [&ok, &optional](const auto& ptr) {
            ok(ptr, true, optional);
            });
      if (!ready) {
        if (!name.empty())
          field_dependents_[field_name].aliases.push_back(name);

        fail(field_name, optional);
      }
    }

    template<typename Callback>
    bool find_field(const text& name, Callback&& cb) noexcept {
      if (auto it = module_.find_field(name); it != module_.end_field()) {
        if (field_dependents_.find(name) == field_dependents_.end()) {
          cb(it->second);
          return true;
        }
      }
      return false;
    }

    void add_deferred_model_field(model_pointer model, text&& key, text&& field, bool required) {
      model_deferred_fields_[model].emplace(key, deferred_field_record { .field_name = field, .required = required });
      field_dependents_[field].models.push_back(
          model_field {
          .key = key,
          .model = std::move(model),
          .required = required
          });
    }

    template<GARLIC_VIEW Layer>
    void process_model_inheritance(model_pointer model, Layer&& layer) noexcept {
      enum class field_status : uint8_t { deferred, ready, excluded };
      struct field_info {
        FlatModel::field_descriptor ready_field;
        deferred_field_record deferred_field;
        field_status status;
      };
      table<text, field_info> fields;
      auto apply_inheritance = [this, &fields, &model](text&& model_name) {
          auto it = module_.find_model(model_name);
          if (it == module_.end_model()) {}  // report parsing error here.
          std::for_each(
              it->second->begin_field(), it->second->end_field(),
              [&fields](const auto& item) {
                auto& info = fields[item.first];
                if (info.status == field_status::excluded)
                  return;
                info.status = field_status::ready;
                info.ready_field = item.second;
              });
          find(model_deferred_fields_, it->second, [&fields](const auto& deferred_fields) {
              for (const auto& item : deferred_fields.second) {
                auto& info = fields[item.first];
                if (info.status == field_status::excluded)
                  continue;
                info.status = field_status::deferred;
                info.deferred_field = item.second;
              }
          });
      };
      get_member(layer, "exclude_fields", [&fields](const auto& excludes) {
          for (const auto& field : excludes.get_list()) {
            fields[decode<text>(field)].status = field_status::excluded;
          }
      });
      get_member(layer, "inherit", [&apply_inheritance](const auto& inherit) {
          if (inherit.is_string()) {
            apply_inheritance(decode<text>(inherit));
            return;
          }
          for (const auto& model_name : inherit.get_list()) {
            apply_inheritance(decode<text>(model_name));
          }
      });
      for (const auto& item : fields) {
        switch (item.second.status) {
          case field_status::deferred:
            add_deferred_model_field(
                model,
                item.first.view(),
                item.second.deferred_field.field_name.view(),
                item.second.deferred_field.required);
            break;
          case field_status::ready:
            model->add_field(
                item.first.clone(),
                item.second.ready_field.field,
                item.second.ready_field.required);
            break;
          default:
            continue;
        }
      }
    }

    template<GARLIC_VIEW Layer, typename Callable>
    void parse_model(text&& name, Layer&& layer, Callable&& cb) {
      auto model_ptr = std::make_shared<FlatModel>(name.clone());

      this->process_model_meta(*model_ptr, layer);
      this->process_model_inheritance(model_ptr, layer);

      get_member(layer, "fields", [this, &model_ptr](const auto& value) {
        std::for_each(value.begin_member(), value.end_member(), [this, &model_ptr](const auto& item) {
          this->parse_field(text::no_text(), item.value,
              [&model_ptr, &item](auto ptr, auto complete, auto optional) {
                model_ptr->add_field(decode<text>(item.key).clone(), ptr, !optional);
          }, [this, &model_ptr, &item](const text& name, auto optional) {
            add_deferred_model_field(model_ptr, decode<text>(item.key), name.view(), !optional);
          });
        });
      });

      auto model_field = std::make_shared<FlatField>(model_ptr->get_name().view());
      model_field->add_constraint(make_constraint<model_tag>(model_ptr));
      this->add_field(model_ptr->get_name().view(), model_field, true);
      cb(std::move(model_ptr));
    }

    template<GARLIC_VIEW Layer, typename Callable>
    void parse_constraint(Layer&& layer, Callable&& cb) noexcept {
      typedef FlatConstraint (*ConstraintInitializer)(const Layer&, parser_ambassador);
      static const table<text, ConstraintInitializer> ctors = {
        {"regex", &parsing::parse_regex<Layer>},
        {"range", &parsing::parse_range<Layer>},
        {"field", &parsing::parse_field<Layer>},
        {"any", &parsing::parse_any<Layer>},
        {"all", &parsing::parse_all<Layer>},
        {"list", &parsing::parse_list<Layer>},
        {"tuple", &parsing::parse_tuple<Layer>},
        {"map", &parsing::parse_map<Layer>},
        {"literal", &parsing::parse_literal<Layer>},
      };

      if (layer.is_string()) {
        parse_field_constraint(layer, cb);
        return;
      }

      get_member(layer, "type",
          [this, &layer, &cb](const auto& item) {
            if (auto it = ctors.find(item.get_cstr()); it != ctors.end()) {
              cb(it->second(layer, parser_ambassador{*this}));
            } else {
              // report parsing error.
            }
          });
    }

    template<GARLIC_VIEW Layer, typename Callable>
    void parse_field_constraint(Layer&& layer, Callable&& cb) {
      auto reference_key = decode<text>(layer);
      auto ready = this->find_field(reference_key,
          [this, &cb](const auto& ptr) {
            cb(make_constraint<field_tag>(std::make_shared<field_pointer>(ptr)));
          });
      if (!ready) {
        auto constraint = make_constraint<field_tag>(std::make_shared<field_pointer>(nullptr));
        field_dependents_[std::move(reference_key)].constraints.push_back(constraint);
        cb(std::move(constraint));
      }
    }

    void resolve_field(const text& key, const field_pointer& ptr) {
      if (auto it = field_dependents_.find(key); it != field_dependents_.end()) {
        // apply the constraints to the fields that inherited from this field.
        //const auto& src_constraints = ptr->properties_.constraints;
        for (auto& field : it->second.fields) {
          field->inherit_constraints_from(*ptr);
          // if the field is a named one, it can be resolved as it is complete now.
          // anonymous fields can be skipped.
          if (!field->get_name().empty()) {
            this->resolve_field(field->get_name(), field);
          }
        }

        // add the field to the depending models.
        for (auto& member : it->second.models) {
          member.model->add_field(member.key.clone(), ptr, member.required);
        }

        // register all the field aliases.
        for (auto& alias : it->second.aliases) {
          this->add_field(alias.clone(), ptr, true);
        }

        for(auto& constraint : it->second.constraints) {
          auto context = reinterpret_cast<typename field_tag::context_type*>(&constraint.context());
          context->set_field(ptr);
        }

        // remove it from the context.
        field_dependents_.erase(it);
      }
    }

    void add_field(text&& key, field_pointer ptr, bool complete) noexcept {
      if (complete) {
        this->resolve_field(key, ptr);  // resolve all the dependencies.
      } else {
        field_dependents_[key];  // create a record so it would be deemed as incomplete.
      }
      module_.add_field(std::move(key), std::move(ptr));  // register the field.
    }

  public:
    ModuleParser(FlatModule& module) : module_(module) {}

    template<GARLIC_VIEW Layer>
    std::error_code parse(Layer&& layer) {
      // Load the fields first.
      get_member(layer, "fields", [this](const auto& value) {
          for (const auto& item : value.get_object()) {
            this->parse_field(
                decode<text>(item.key), item.value,
                [this, &item](auto ptr, auto complete, auto) {
                  this->add_field(decode<text>(item.key).clone(), std::move(ptr), complete);
                },
                [](...){});
          }
          });

      // Load the models.
      get_member(layer, "models", [this](const auto& value) {
        for(const auto& item : value.get_object()) {
          this->parse_model(
              decode<text>(item.key), item.value,
              [this](auto&& ptr) { module_.add_model(std::move(ptr)); });
        }
      });

      return (field_dependents_.size() ? GarlicError::Redefinition : std::error_code());
    }
  };

  template<GARLIC_VIEW Layer>
  static tl::expected<FlatModule, std::error_code>
  load_module(Layer&& layer) noexcept {
    if (!layer.is_object())
      return tl::make_unexpected(GarlicError::Redefinition);

    FlatModule module;
    auto parser = ModuleParser(module);
    if (auto error = parser.parse(layer); error)
      return tl::make_unexpected(error);

    return module;
  }

}

#endif /* end of include guard: GARLIC_PARSING_MODULE */
