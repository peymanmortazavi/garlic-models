#ifndef GARLIC_MODULE_H
#define GARLIC_MODULE_H

/*!
 * \file module.h
 * \brief A repository of models, fields and constraints that can be serialized/deserialized.
 */

#include <unordered_map>

#include "constraints.h"
#include "error.h"
#include "utility.h"


namespace garlic {

  /*! \brief A Module is a repository of models, fields and constraints.
   *
   *  This allows to package lots of relevant fields and models in one object.
   *  This can be parsed and dumped which makes it easy to save in a file or send it over the wire.
   *
   *  In a module, no two records can exist with the same entity and they cannot be overriden.
   *  So if a Field has already been added by the name "Field", this cannot be exchanged for any
   *  other Field. This is mostly because other Model or Constraint instances might already depend on it.
   *
   *  \code{.cpp}
   *  garlic::Module module;
   *  module.add_field(garlic::make_field("ID", { garlic::make_constraint<garlic::regex_tag>("\\d{1,3}") }));
   *  module.add_field("UID", module.get_field("ID"));  // an alias for field ID.
   *  auto user_model = garlic::make_model("User");
   *  user_model->add_field("id", module.get_field("UID"));
   *  module.add_model(user_model);
   *  \endcode
   *
   *  \note Try not to add models or fields that depend on resources outside of this module.
   *        That would make it impossible to deserialize should that become necessary.
   */
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

    /*! Creates an empty module with the following fields already available:
     *  **string**, **bool**, **list**, **object**, **integer**, **double**
     */
    Module() : fields_(kDefaultFieldTableSize) {
      static table<text, field_pointer> static_map = {
        {"string", this->make_field<type_tag>("StringField", TypeFlag::String)},
        {"integer", this->make_field<type_tag>("IntegerField", TypeFlag::Integer)},
        {"double", this->make_field<type_tag>("DoubleField", TypeFlag::Double)},
        {"list", this->make_field<type_tag>("ListField", TypeFlag::List)},
        {"object", this->make_field<type_tag>("ObjectField", TypeFlag::Object)},
        {"bool", this->make_field<type_tag>("BooleanField", TypeFlag::Boolean)},
        {"null", this->make_field<type_tag>("BooleanField", TypeFlag::Null)},
      };
      fields_ = static_map;
    }

    /*! Tries to add a new model to the module.
     *
     * \param model A shared pointer to the Model.
     * \return nothing if successful, std::error_code if a model by the same name already exists.
     */
    tl::expected<void, std::error_code>
    add_model(model_pointer model) noexcept {
      if (auto it = models_.find(model->name()); it != models_.end())
        return tl::make_unexpected(GarlicError::Redefinition);
      models_.emplace(model->name().view(), std::move(model));
      return tl::expected<void, std::error_code>();
    }

    /*! Tries to add a new field to the module.
     *
     * \param alias A custom name or an alias to use for the field.
     * \param field A shared pointer to the Field.
     * \return nothing if successful, std::error_code if a field record by the same alias already exists.
     */
    tl::expected<void, std::error_code>
    add_field(text&& alias, field_pointer field) noexcept {
      if (auto it = fields_.find(alias); it != fields_.end())
        return tl::make_unexpected(GarlicError::Redefinition);
      fields_.emplace(std::move(alias), std::move(field));
      return tl::expected<void, std::error_code>();
    }

    /*! Tries to add a new field to the module, using its name for the field record.
     *
     * \param field A shared pointer to the Field.
     * \return nothing if successful, std::error_code if a field record by the same alias already exists.
     */
    inline tl::expected<void, std::error_code>
    add_field(field_pointer field) noexcept {
      return add_field(field->name().view(), field);
    }

    /*! Get a model by its name.
     *
     * \return a shared pointer to the model if found, otherwise nullptr.
     */
    model_pointer get_model(const text& name) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) return it->second;
      return nullptr;
    }

    /*! Get a model by its name.
     *  Calls the callback function with a shared pointer to the Model if found.
     */
    template<typename Callable>
    void get_model(const text& name, Callable&& cb) const noexcept {
      if (auto it = models_.find(name); it != models_.end()) cb(it->second);
    }

    //! \return a read-only (const) iterator that points to the first model in the module.
    inline const_model_iterator begin_models() const { return models_.begin(); }

    //! \return a read-only (const) iterator that points to one past the last model in the module.
    inline const_model_iterator end_models() const { return models_.end(); }

    //! \return a read-only (const) iterator that points to found element or **end_models()** if not found.
    inline const_model_iterator find_model(const text& name) const { return models_.find(name); }

    //! \return a shared pointer to the found field or nullptr if not found.
    field_pointer get_field(const text& name) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) return it->second;
      return nullptr;
    }

    //! Calls the callable function with a shared pointer to the field, if found.
    template<typename Callable>
    void get_field(const text& name, Callable&& cb) const noexcept {
      if (auto it = fields_.find(name); it != fields_.end()) cb(it->second);
    }

    //! \return a read-only (const) iterator that points to the first field in the module.
    inline const_field_iterator begin_fields() const { return fields_.begin(); }

    //! \return a read-only (const) iterator that points to one past the last field in the module.
    inline const_field_iterator end_fields() const { return fields_.end(); }

    //! \return a read-only (const) iterator that points to found element or **end_fields()** if not found.
    inline const_field_iterator find_field(const text& name) const { return fields_.find(name); }

  private:
    template<typename ConstraintTag, typename... Args>
    static inline field_pointer make_field(text&& name, Args&&... args) noexcept {
      sequence<Constraint> constraints(1);
      constraints.push_back(make_constraint<ConstraintTag>(std::forward<Args>(args)...));
      return std::make_shared<field_type>(Field::Properties {
          .annotations = {},
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
