#ifndef GARLIC_MODULE_H
#define GARLIC_MODULE_H

#include <iterator>
#include <unordered_map>

#include "lib/expected.h"

#include "layer.h"
#include "constraints.h"
#include "error.h"
#include "utility.h"


namespace garlic {

  class Module {
  public:
    template <typename Key, typename Value> using table = std::unordered_map<Key, Value>;
    using model_type               = Model;
    using model_pointer            = std::shared_ptr<model_type>;
    using field_type               = Field;
    using field_pointer            = std::shared_ptr<field_type>;
    using model_table              = table<text, model_pointer>;
    using field_table              = table<text, field_pointer>;
    using constraint_type          = Constraint;
    using const_model_iterator     = typename model_table::const_iterator;
    using const_field_iterator     = typename field_table::const_iterator;

    constexpr static auto kDefaultFieldTableSize = 16;

    Module() : fields_(kDefaultFieldTableSize) {
      static table<text, field_pointer> static_map = {
        {"string", this->make_field<type_tag>("StringField", TypeFlag::String)},
        {"integer", this->make_field<type_tag>("IntegerField", TypeFlag::Integer)},
        {"double", this->make_field<type_tag>("DoubleField", TypeFlag::Double)},
        {"list", this->make_field<type_tag>("ListField", TypeFlag::List)},
        {"object", this->make_field<type_tag>("ObjectField", TypeFlag::Object)},
        {"bool", this->make_field<type_tag>("BooleanField", TypeFlag::Boolean)},
      };
      fields_ = static_map;
    }

    tl::expected<void, std::error_code> add_model(model_pointer model) noexcept {
      if (auto it = models_.find(model->name()); it != models_.end())
        return tl::make_unexpected(GarlicError::Redefinition);
      models_.emplace(model->name().view(), std::move(model));
      return tl::expected<void, std::error_code>();
    }

    tl::expected<void, std::error_code> add_field(text&& alias, field_pointer field) noexcept {
      if (auto it = fields_.find(alias); it != fields_.end())
        return tl::make_unexpected(GarlicError::Redefinition);
      fields_.emplace(std::move(alias), std::move(field));
      return tl::expected<void, std::error_code>();
    }

    inline tl::expected<void, std::error_code> add_field(field_pointer field) noexcept {
      return add_field(field->name().view(), field);
    }

    model_pointer get_model(const text& name) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_model(const text& name, Callable&& cb) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) cb(it->second);
    }

    inline const_model_iterator begin_model() const { return models_.begin(); }
    inline const_model_iterator end_model() const { return models_.end(); }
    inline const_model_iterator find_model(const text& name) const { return models_.find(name); }

    field_pointer get_field(const text& name) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) return it->second;
      return nullptr;
    }

    template<typename Callable>
    void get_field(const text& name, Callable&& cb) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) cb(it->second);
    }

    inline const_field_iterator begin_field() const { return fields_.begin(); }
    inline const_field_iterator end_field() const { return fields_.end(); }
    inline const_field_iterator find_field(const text& name) const { return fields_.find(name); }

  private:
    template<typename ConstraintTag, typename... Args>
    static inline field_pointer make_field(text&& name, Args&&... args) noexcept {
      sequence<Constraint> constraints(1);
      constraints.push_back(make_constraint<ConstraintTag>(std::forward<Args>(args)...));
      return std::make_shared<field_type>(Field::Properties {
          .meta = {},
          .constraints = std::move(constraints),
          .name = std::move(name),
          .ignore_details = false
          });
    }

    model_table models_;
    field_table fields_;
  };
}


#endif /* end of include guard: GARLIC_MODULE_H */
