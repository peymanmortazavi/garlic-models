#ifndef GARLIC_CONSTRAINTS_H
#define GARLIC_CONSTRAINTS_H

//! \file constraints.h \brief Contains constraints, built-in constraint tags, fields and models.

#include <algorithm>
#include <unordered_set>
#include <regex>

#include "layer.h"
#include "utility.h"
#include "containers.h"


namespace garlic {

  //! Result of a constriant test.
  struct ConstraintResult {

    using constraint_sequence = sequence<ConstraintResult>;

    enum flags : uint8_t {
      none      = 0x1 << 0,
      valid     = 0x1 << 1,
      field     = 0x1 << 2,
    };

    constraint_sequence details;  //!< extra details/inner constraint list.

    text name;  //!< name of the constraint. empty if successful.

    text reason;  //!< reason for the failure, if any. empty if successful. 

    flags flag;

    //! \return whether or not the constraint is a leaf meaning it does not have any extra details.
    inline bool is_leaf()  const noexcept { return !details.size();  }

    //! \return whether or not the constraint test has passed, meaning no failure.
    inline bool is_valid() const noexcept { return flag & flags::valid; }

    //! \return whether or not the constraint corresponds to a field/key, if so, **name** will be the field name.
    inline bool is_field() const noexcept { return flag & flags::field; }

    inline void set_valid() noexcept { flag = static_cast<flags>(flag & flags::valid); }
    inline void set_field() noexcept { flag = static_cast<flags>(flag & flags::field); }

    //! \copydoc is_valid()
    inline operator bool() const { return is_valid(); }

    template<flags Flag = flags::none>
    static ConstraintResult leaf_failure(
        text&& name, text&& reason=text::no_text()) noexcept {
      return ConstraintResult {
        .details = sequence<ConstraintResult>::no_sequence(),
        .name = std::move(name),
        .reason = std::move(reason),
        .flag = Flag
      };
    }

    static inline ConstraintResult leaf_field_failure(
        text&& name, text&& reason=text::no_text()) noexcept {
      return leaf_failure<flags::field>(std::move(name), std::move(reason));
    }

    static inline ConstraintResult
    field_failure(
        text&& name,
        ConstraintResult&& inner_detail,
        text&& reason = text::no_text()) noexcept {
      sequence<ConstraintResult> details(1);
      details.push_back(std::move(inner_detail));
      return ConstraintResult {
        .details = std::move(details),
        .name = std::move(name),
        .reason = std::move(reason),
        .flag = flags::field
      };
    }

    static ConstraintResult ok() noexcept {
      return ConstraintResult {
        .details = sequence<ConstraintResult>::no_sequence(),
        .name = text::no_text(),
        .reason = text::no_text(),
        .flag = flags::valid
      };
    }

  };

  class constraint_context {
  public:
    enum flags : uint8_t {
      none  = 0x1 << 0,
      fatal = 0x1 << 1,  // should stop looking at other constraints.
    };

    text name;  // constraint name.
    text message;  // custom rejection reason.
    flags flag = flags::none;

    constraint_context(
        text&& name = text::no_text(),
        text&& message = text::no_text(),
        bool fatal = false
        ) : name(std::move(name)), message(std::move(message)), flag(fatal ? flags::fatal : flags::none) {}

    virtual ~constraint_context() = default;

    inline bool is_fatal() const noexcept { return flag & flags::fatal; }

    template<bool Field = false, bool Leaf = true>
    auto fail() const noexcept -> ConstraintResult {
      return ConstraintResult {
        .details = (Leaf ? sequence<ConstraintResult>::no_sequence() : sequence<ConstraintResult>()),
        .name = name,
        .reason = message,
        .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
      };
    }

    template<bool Field = false, bool Leaf = true>
    auto fail(const text& message) const noexcept -> ConstraintResult {
      if (this->message.empty())
        return ConstraintResult {
          .details = (Leaf ? sequence<ConstraintResult>::no_sequence() : sequence<ConstraintResult>()),
          .name = name,
          .reason = message,
          .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
        };
      return this->fail<Field, Leaf>();
    }

    template<bool Field = false>
    auto fail(const text& message, sequence<ConstraintResult>&& details) const noexcept {
      return ConstraintResult {
        .details = std::move(details),
        .name = name,
        .reason = (this->message.empty() ? message : this->message),
        .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
      };
    }

    template<bool Field = false>
    auto fail(const text& message, ConstraintResult&& inner_detail) const noexcept {
      sequence<ConstraintResult> details(1);
      details.push_back(std::move(inner_detail));
      return this->fail<Field>(message, std::move(details));
    }

    inline auto ok() const noexcept -> ConstraintResult {
      return ConstraintResult::ok();
    }
  };

  template<GARLIC_VIEW Layer>
  static inline text 
  get_text(Layer&& layer, const char* key, text default_value) noexcept {
    get_member(layer, key, [&default_value](const auto& result) {
        default_value = text::copy(result.get_string_view());
        });
    return default_value;
  }

  template<typename T>
  using constraint_test_handler = ConstraintResult (*)(const T&, const constraint_context*);

  template<typename T>
  using constraint_quick_test_handler = bool (*)(const T&, const constraint_context*);

  namespace internal {
    template<typename InnerTag>
    struct tag_wrapper {
      template<GARLIC_VIEW Layer>
      static inline ConstraintResult
      test(const Layer& layer, const constraint_context* context) noexcept {
        return InnerTag::test(layer, *reinterpret_cast<const typename InnerTag::context_type*>(context));
      }

      template<GARLIC_VIEW Layer>
      static inline bool
      quick_test(const Layer& layer, const constraint_context* context) noexcept {
        return InnerTag::quick_test(layer, *reinterpret_cast<const typename InnerTag::context_type*>(context));
      }
    };

    template<typename Tag, typename... Rest>
    struct registry {
      using tag = tag_wrapper<Tag>;
      using rest = registry<Rest...>;
      static constexpr unsigned id = sizeof...(Rest);
      static constexpr unsigned not_found = UINT32_MAX;

      template<typename Query>
      static unsigned constexpr position_of() {
        if (std::is_same<Tag, Query>::value)
          return 0;
        constexpr auto result = rest::template position_of<Query>();
        if (result == not_found)
          return result;
        return result + 1;
      }

      template<typename Query, GARLIC_VIEW Layer>
      static constexpr auto test_handler() {
        if (std::is_same<Tag, Query>::value)
          return &tag::template test<Layer>;
        return rest::template test_handler<Query, Layer>();
      }

      template<GARLIC_VIEW Layer>
      static constexpr std::array<constraint_test_handler<Layer>, sizeof...(Rest) + 1>
      test_handlers() {
        return {&tag::template test<Layer>, &tag_wrapper<Rest>::template test<Layer>...};
      }

      template<GARLIC_VIEW Layer>
      static constexpr std::array<constraint_quick_test_handler<Layer>, sizeof...(Rest) + 1>
      quick_test_handlers() {
        return {&tag::template quick_test<Layer>, &tag_wrapper<Rest>::template quick_test<Layer>...};
      }
    };

    template<typename Tag>
    struct registry<Tag> {
      using tag = tag_wrapper<Tag>;
      static constexpr unsigned id = 0;
      static constexpr unsigned not_found = UINT32_MAX;

      template<typename Query>
      static unsigned constexpr position_of() {
        return (std::is_same<Tag, Query>::value ? 0 : not_found);
      }

      template<typename Query, GARLIC_VIEW Layer>
      static constexpr auto test_handler() {
        return (std::is_same<Tag, Query>::value ? &tag::template test<Layer> : nullptr);
      }

      template<typename Query, GARLIC_VIEW Layer>
      static constexpr auto quick_test_handler() {
        return (std::is_same<Tag, Query>::value ? &tag::template quick_test<Layer> : nullptr);
      }
    };

  }

  //! Smallest unit of data validation.
  /*!
   * A Constraint is a generic type that can be used to validate
   * any type that conforms to garlic::ViewLayer.
   *
   * This type cannot be inherited and it does not have public constructors.
   * The proper way to create a Constraint is to use Constraint::make() or its
   * shorthand make_constraint().
   *
   * If needed, one can define an uninitialized Constraint using Constraint::empty()
   * but this Constraint should not be passed to any other object as it is uninitialized
   * and can lead to crashes or at least undefined behaviors.
   *
   * \note You should think of Constraint as a shared pointer to a context data that
   *       can be used to instantiate **test** and **quick_test** methods when needed.
   *       Thus, copying a Constraint does not really deep copy its inside, it creates
   *       a new pointer that holds onto the inner context, much like a shared pointer.
   *       In fact it does use a shared pointer inside.
   *
   * \code
   * auto regex_constriant = garlic::make_constraint<garlic::regex_tag>("\\d");
   * auto copy = regex_constraint;  // uses the same context as regex_constraint.
   * &copy.context() == &regex_constraint.context();  // true
   * \endcode
   */
  class Constraint final {
  private:
    using context_pointer = std::shared_ptr<constraint_context>;
    context_pointer context_;
    unsigned index_;

    Constraint(unsigned index, context_pointer&& context) : context_(std::move(context)), index_(index) {}

  public:
    //! Create a generic Constraint based on a constraint tag.
    /*! \tparam Tag any **built-in** constraint tag that is registered in constraint_registry.
     *  \tparam Args appropriate set of arguments for the constraint tag used.
     *  \note You will get a compile-time error if you use a tag that is not registered.
     */
    template<typename Tag, typename... Args>
    static Constraint make(Args&&... args) noexcept;

    //! Make a Constraint without any context.
    static Constraint empty() noexcept {
      return Constraint(0, nullptr);
    }

    //! \return whether or not the constraint is initialized.
    inline operator bool () const noexcept {
      return context_ != nullptr;
    }

    //! Perform the constraint test on the layer and return the result.
    /*! \tparam Layer any readable layer that conforms to the garlic::ViewLayer concept.
     */
    template<GARLIC_VIEW Layer>
    inline ConstraintResult test(const Layer& value) const noexcept;

    //! Very similar to Constraint::test() but it only returns a boolean.
    /*! \tparam Layer any readable layer that conforms to the garlic::ViewLayer concept.
     */
    template<GARLIC_VIEW Layer>
    inline bool quick_test(const Layer& value) const noexcept;

    inline const constraint_context& context() const noexcept {
      return *context_;
    }

    inline constraint_context& context() noexcept {
      return *context_;
    }

    template<typename Tag>
    inline const auto& context_for() const noexcept {
      return *reinterpret_cast<typename Tag::context_type*>(context_.get());
    }

    template<typename Tag>
    inline auto& context_for() noexcept {
      return *reinterpret_cast<typename Tag::context_type*>(context_.get());
    }

    template<typename, typename... Args>
    friend inline Constraint make_constraint(Args&&...) noexcept;
  };


  //! Run a number of constraints on a layer.
  /*! \tparam Layer any readable layer that conforms to garlic::ViewLayer concept.
   *  \tparam Container any constainer that stores a number of Constraint items and has begin() and end()
   *  \param it a back inserter iterator to store ConstraintResult items.
   */
  template<GARLIC_VIEW Layer, typename Container, typename BackInserterIterator>
  inline void test_constraints(
      Layer&& value,
      Container&& constraints,
      BackInserterIterator it) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint.test(value); !result.is_valid()) {
        it = std::move(result);
        if (constraint.context().is_fatal())
          break;
      }
    }
  }


  /*! \return whether or not a readable layer passes all constraint tests.
   *  \tparam Layer any readable layer that conforms to garlic::ViewLayer concept.
   *  \tparam Container any constainer that stores a number of Constraint items and has begin() and end()
   */
  template<GARLIC_VIEW Layer, typename Container>
  static inline bool
  test_constraints_quick(Layer&& value, Container&& constraints) {
    return std::all_of(
        std::begin(constraints), std::end(constraints),
        [&value](const auto& constraint) { return constraint.quick_test(value); }
        );
  }


  /*! \return the first failed Constraint or return an ok Constraint if all Constraint checks pass.
   *  \tparam Layer any readable layer that conforms to garlic::ViewLayer concept.
   *  \tparam Container any constainer that stores a number of Constraint items and has begin() and end()
   */
  template<GARLIC_VIEW Layer, typename Container>
  static inline ConstraintResult
  test_constraints_first_failure(Layer&& value, Container&& constraints) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint.test(value); !result.is_valid()) {
        return result;
      }
    }
    return ConstraintResult::ok();
  }

  //! Constraint Tag that passes if the layer has matching data type.
  /*! \code{.cpp}
   *  make_constraint<type_tag>(TypeTag);
   *  \endcode
   */
  struct type_tag {
    
    struct Context : public constraint_context {
      template<typename... Args>
      Context(TypeFlag flag, text&& name = "type_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...), flag(flag) {}

      TypeFlag flag;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test (const Layer& layer, const Context& context) noexcept {
      switch (context.flag) {
        case TypeFlag::Null: {
          if (layer.is_null()) { return context.ok(); }
          else return context.fail("Expected null.");
        }
        case TypeFlag::Boolean: {
          if (layer.is_bool()) { return context.ok(); }
          else return context.fail("Expected boolean type.");
        }
        case TypeFlag::Double: {
          if (layer.is_double()) { return context.ok(); }
          else return context.fail("Expected double type.");
        }
        case TypeFlag::Integer: {
          if (layer.is_int()) { return context.ok(); }
          else return context.fail("Expected integer type.");
        }
        case TypeFlag::String: {
          if (layer.is_string()) { return context.ok(); }
          else return context.fail("Expected string type.");
        }
        case TypeFlag::List: {
          if (layer.is_list()) { return context.ok(); }
          else return context.fail("Expected a list.");
        }
        case TypeFlag::Object: {
          if (layer.is_object()) { return context.ok(); }
          else return context.fail("Expected an object.");
        }
        default: return context.fail();
      }
    }

    template<GARLIC_VIEW Layer>
    static bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      switch (context.flag) {
        case TypeFlag::Null: return layer.is_null();
        case TypeFlag::Boolean: return layer.is_bool();
        case TypeFlag::Double: return layer.is_double();
        case TypeFlag::Integer: return layer.is_int();
        case TypeFlag::String: return layer.is_string();
        case TypeFlag::List: return layer.is_list();
        case TypeFlag::Object: return layer.is_object();
        default: return false;
      }

    }
  };


  //! Constraint Tag that passes if the layer is within a certain boundary.
  /*! If layer is a *string*, its length will be checked.
   *
   *  If layer is a *number*, its value will be checked.
   *
   *  If layer is a *list*, its element count will be checked.
   *
   *  It passes the test for all other types including *null*.
   * \code{.cpp}
   *  make_constraint<range_tag>(size_t min, size_t max);
   *  \endcode
   */
  struct range_tag {
    using size_type = size_t;

    struct Context : public constraint_context {
      template<typename... Args>
      Context(size_type min, size_type max, text&& name = "range_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...), min(min), max(max) {}

      size_type min;
      size_type max;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (layer.is_string()) {
        auto length = layer.get_string_view().size();
        if (length > context.max || length < context.min) return context.fail("invalid string length.");
      } else if (layer.is_double()) {
        auto dvalue = layer.get_double();
        if(dvalue > context.max || dvalue < context.min) return context.fail("out of range value.");
      } else if (layer.is_int()) {
        auto ivalue = layer.get_int();
        if(static_cast<size_type>(ivalue) > context.max || static_cast<size_type>(ivalue) < context.min)
          return context.fail("out of range value.");
      } else if (layer.is_list()) {
        size_type count = garlic::list_size(layer);
        if (count > context.max)
          return context.fail("too many items in the list.");
        if (count < context.min)
          return context.fail("too few items in the list.");
      }
      return context.ok();
    }

    template<GARLIC_VIEW Layer>
    static bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      if (layer.is_string()) {
        auto length = layer.get_string_view().size();
        if (length > context.max || length < context.min) return false;
      } else if (layer.is_double()) {
        auto dvalue = layer.get_double();
        if(dvalue > context.max || dvalue < context.min) return false;
      } else if (layer.is_int()) {
        auto ivalue = layer.get_int();
        if(static_cast<size_type>(ivalue) > context.max || static_cast<size_type>(ivalue) < context.min)
          return false;
      } else if (layer.is_list()) {
        size_type count = garlic::list_size(layer);
        if (count > context.max)
          return false;
        if (count < context.min)
          return false;
      }
      return true;

    }
  };


  //! Constraint Tag that passes if a specified regex pattern passes the test.
  /*! \note It passes the test if the layer is not a string type.
   *
   *  \code{.cpp}
   *  make_constraint<regex_tag>(text&& pattern);
   *  \endcode
   */
  struct regex_tag {

    struct Context : public constraint_context {
      template<typename... Args>
      Context(text&& pattern, text&& name = "regex_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              pattern(pattern.data(), pattern.size()) {}

      std::regex pattern;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_string()) return context.ok();
      if (std::regex_match(layer.get_cstr(), context.pattern)) { return context.ok(); }
      else { return context.fail("invalid value."); }
    }

    template<GARLIC_VIEW Layer>
    static inline bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_string()) return true;
      if (std::regex_match(layer.get_cstr(), context.pattern)) return true;
      else return false;
    }
  };


  //! Constraint Tag that passes if any of the inner constraints pass.
   /*! \code{.cpp}
   *   make_constraint<any_tag>(sequence<Constraint>&& constraints);
   *   \endcode
   */
  struct any_tag {
    struct Context : public constraint_context {
      template<typename... Args>
      Context(sequence<Constraint>&& constraints, Args&&... args
          ) : constraint_context(std::forward<Args>(args)...), constraints(std::move(constraints)) {}

      sequence<Constraint> constraints;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (any_tag::quick_test(layer, context))
        return context.ok();
      return context.fail("None of the constraints read this value.");
    }

    template<GARLIC_VIEW Layer>
    static inline bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      return std::any_of(
          context.constraints.begin(),
          context.constraints.end(),
          [&layer](const auto& item) { return item.quick_test(layer); });
    }
  };


  /*! \brief Constraint Tag that passes if the layer is a list and all its
   *         elements pass the inner constraints.
   *  \note If the layer is not a *list*, the test will fail.
   *
   *  \code{.cpp}
   *  make_constraint<list_tag>(Constraint&& constraint);
   *  \endcode
   */
  struct list_tag {
    struct Context : public constraint_context {
      template<typename... Args>
      Context(
          Constraint&& constraint, bool ignore_details = false,
          text&& name = "list_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              constraint(std::move(constraint)), ignore_details(ignore_details) {}

      Constraint constraint;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_list()) return context.fail("Expected a list.");
      if (context.ignore_details)
        return list_tag::test<true>(layer, context);
      return list_tag::test<false>(layer, context);
    }

    template<GARLIC_VIEW Layer>
    static bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_list()) return false;
      return std::all_of(
          layer.begin_list(), layer.end_list(),
          [&context](const auto& item) { return context.constraint.quick_test(item); }
          );
    }

    template<bool IgnoreDetails, GARLIC_VIEW Layer>
    inline static ConstraintResult test(const Layer& layer, const Context& context) noexcept {
      size_t index = 0;
      for (const auto& item : layer.get_list()) {
        if (IgnoreDetails) {  // no runtime penalty since IgnoreDetails is a template parameter.
          if (!context.constraint.quick_test(item)) {
            return context.fail(
                "Invalid value found in the list.",
                ConstraintResult::leaf_field_failure(
                  text::copy(std::to_string(index)),
                  "invalid value."));
          }
        } else {
          auto result = context.constraint.test(item);
          if (!result.is_valid()) {
            return context.fail(
                "Invalid value found in the list.",
                ConstraintResult::field_failure(
                  text::copy(std::to_string(index)),
                  std::move(result),
                  "invalid value."
                  ));
          }
        }
        ++index;
      }
      return context.ok();
    }
  };

  /*! \brief Constraint Tag that passes if the layer is a list and its first n
   *         elements pass the first n constraints.
   *
   *  Basically when constraint[N] passes layer.begin_list()[N]
   *
   *  \code{.cpp}
   *  make_constraint<tuple_tag>(sequence<Constraint>&& constraints,
                                 bool strict = true,
                                 bool ignore_details = false);
   *  \endcode
   *
   *  \param constraints ordered list of constraints to match the elements in the layer.
   *  \param strict if true, layer can only have as many elements as there are constraints.
   *  \param ignore_details if true, failing constraints will not be added to the resulting
   *                        ConstraintResult as details.
   *  \return The resulting ConstraintResult will have one extra detail and it's name is the index of element
   *          in the tuple.
   */
  class tuple_tag {
  public:
    struct Context : public constraint_context {
      template<typename... Args>
      Context(
          sequence<Constraint>&& constraints, bool strict = true, bool ignore_details = false,
          text&& name = "tuple_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              constraints(std::move(constraints)), strict(strict), ignore_details(ignore_details) {}

      sequence<Constraint> constraints;
      bool strict;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static bool
    quick_test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_list()) return false;
      auto tuple_it = layer.begin_list();
      auto constraint_it = context.constraints.begin();
      while (constraint_it != context.constraints.end() && tuple_it != layer.end_list()) {
        if (!constraint_it->quick_test(*tuple_it)) return false;
        std::advance(tuple_it, 1);
        std::advance(constraint_it, 1);
      }
      if (context.strict && tuple_it != layer.end_list()) return false;
      if (constraint_it != context.constraints.end()) return false;
      return true;
    }

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.ignore_details)
        return tuple_tag::test<true>(layer, context);
      return tuple_tag::test<false>(layer, context);
    }

  private:
    template<bool IgnoreDetails, GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_list())
        return context.fail("Expected a list (tuple).");
      size_t index = 0;
      auto tuple_it = layer.begin_list();
      auto constraint_it = context.constraints.begin();
      while (constraint_it != context.constraints.end() && tuple_it != layer.end_list()) {
        if (IgnoreDetails) {
          auto result = constraint_it->quick_test(*tuple_it);
          if (!result) {
            return context.fail(
                "Invalid value found in the tuple.",
                ConstraintResult::leaf_field_failure(
                  text::copy(std::to_string(index)),
                  "invalid value."
                  ));
          }
        } else {
          auto result = constraint_it->test(*tuple_it);
          if (!result.is_valid()) {
            return context.fail(
                "Invalid value found in the tuple.",
                ConstraintResult::field_failure(
                  text::copy(std::to_string(index)),
                  std::move(result),
                  "invalid value."
                  ));
          }
        }
        std::advance(tuple_it, 1);
        std::advance(constraint_it, 1);
        ++index;
      }
      if (context.strict && tuple_it != layer.end_list()) {
        return context.fail("Too many values in the tuple.");
      }
      if (constraint_it != context.constraints.end()) {
        return context.fail("Too few values in the tuple.");
      }
      return context.ok();
    }
  };

  /*! \brief Constraint Tag that passes if the layer's members pass key and value constraints.
   *
   *  \code{.cpp}
   *  make_constraint<map_tag>(Constraint&& key_constraint,
                               Constraint&& value_constraint,
                               bool ignore_details);
   *  \endcode
   *
   *  \note In this tag, it is **valid** to pass empty (uninitialized) constraints, they will be ignored in that case.
   *
   *  \param key_constraint member keys must pass this constraint if constraint is not empty.
   *  \param value_constraint member values must pass this constraint if constraint is not empty.
   *  \param ignore_details if true, failing constraints will not be added to the resulting
   *                        ConstraintResult as details.
   */
  class map_tag {
  public:
    struct Context : public constraint_context {
      template<typename... Args>
      Context(
          Constraint&& key_constraint, Constraint&& value_constraint, bool ignore_details = false,
          text&& name = "map_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              key(std::move(key_constraint)), value(std::move(value_constraint)), ignore_details(ignore_details) {}

      Constraint key;
      Constraint value;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_object())
        return context.fail("Expected an object.");
      if (context.ignore_details)
        return map_tag::test<true>(layer, context);
      return map_tag::test<false>(layer, context);
    }

    template<GARLIC_VIEW Layer>
    static bool quick_test(const Layer& layer, const Context& context) noexcept {
      if (!layer.is_object())
        return false;
      if (context.key) {
        if (context.value)
          return map_tag::quick_test<true, true>(layer, context);
        return map_tag::quick_test<true, false>(layer, context);
      } else if (context.value)
        return map_tag::quick_test<false, true>(layer, context);
      return true;  // meaning there is not key or value constraint.
    }

  private:
    template<bool HasKeyConstraint, bool HasValueConstraint, GARLIC_VIEW Layer>
    static inline bool quick_test(const Layer& layer, const Context& context) noexcept {
      return std::all_of(
          layer.begin_member(), layer.end_member(),
          [&context](const auto& item) {
            if (HasKeyConstraint && !context.key.quick_test(item.key)) return false;
            if (HasValueConstraint && !context.value.quick_test(item.value)) return false;
            return true;
            });
    }

    template<bool IgnoreDetails, GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.key) {
        if (context.value) return test<IgnoreDetails, true, true>(layer, context);
        return test<IgnoreDetails, true, false>(layer, context);
      } else if (context.value) return test<IgnoreDetails, false, true>(layer, context);
      return context.ok();  // meaning there is no key or value constraint.
    }

    template<bool IgnoreDetails, bool HasKeyConstraint, bool HasValueConstraint, GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      for (const auto& item : layer.get_object()) {
        if (HasKeyConstraint) {
          if (IgnoreDetails && !context.key.quick_test(item.key)) {
            return context.fail("Object contains invalid key.");
          } else {
            if (auto result = context.key.test(item.key); !result.is_valid()) {
              return context.fail("Object contains invalid key.", std::move(result));
            }
          }
        }
        if (HasValueConstraint) {
          if (IgnoreDetails && !context.value.quick_test(item.value)) {
            return context.fail("Object contains invalid value.");
          } else {
            if (auto result = context.value.test(item.value); !result.is_valid()) {
              return context.fail("Object contains invalid value.", std::move(result));
            }
          }
        }
      }
      return context.ok();
    }
  };

  /*! \brief Constraint Tag that passes if all inner constraints pass the layer.
   *
   *  \code{.cpp}
   *  make_constraint<all_tag>(sequence<Constraint>&& constraints,
                               bool hide = true,
                               bool ignore_details = false);
   *  \endcode
   *
   *  \param hide if true, ConstraintResult from the first failing constraint will be
   *              returned. This practically hides this constraint and leaves no trace
   *              of it in the resulting ConstraintResult.
   *  \param ignore_details if true, failing constraints will not be added to the resulting
   *                        ConstraintResult as details.
   */
  struct all_tag {
    struct Context : public constraint_context {
      template<typename... Args>
      Context(
          sequence<Constraint>&& constraints, bool hide = true, bool ignore_details = false,
          Args&&... args
          ) : constraint_context(std::forward<Args>(args)...),
              constraints(std::move(constraints)), hide(hide), ignore_details(ignore_details) {}

      sequence<Constraint> constraints;
      bool hide;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.hide)
        return test_constraints_first_failure(layer, context.constraints);
      if (context.ignore_details) {
        if (test_constraints_quick(layer, context.constraints))
          return context.ok();
        else
          return context.fail("Some of the constraints fail on this value.");
      }
      sequence<ConstraintResult> results;
      test_constraints(layer, context.constraints, std::back_inserter(results));
      if (results.empty())
        return context.ok();
      return context.fail("Some of the constraints fail on this value.", std::move(results));
    }

    template<GARLIC_VIEW Layer>
    static bool quick_test(const Layer& layer, const Context& context) noexcept {
      return test_constraints_quick(layer, context.constraints);
    }
  };

  /*! \brief Constraint Tag that passes if layer is equal to a specified value.
   *
   *  \code{.cpp}
   *  make_constraint<string_literal_tag>("text");
   *  make_constraint<int_literal_tag>(12);
   *  make_constraint<double_literal_tag>(10.5);
   *  make_constraint<bool_literal_tag>(false);
   *  make_constraint<null_literal_tag>();
   *  \endcode
   */
  template<typename T>
  struct literal_tag {
    struct Context : public constraint_context {

      template<typename... Args>
      Context(T value, Args&&... args) : constraint_context(std::forward<Args>(args)...), value(value) {}

      T value;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (literal_tag::validate(layer, context.value))
        return context.ok();
      else
        return context.fail("invalid value.");
    }

    template<GARLIC_VIEW Layer>
    static bool quick_test(const Layer& layer, const Context& context) noexcept {
      return literal_tag::validate(layer, context.value);
    }

  private:
    template<typename V>
    using enable_if_int = typename std::enable_if<std::is_integral<V>::value && !std::is_same<V, bool>::value, bool>::type;

    template<typename V>
    using enable_if_bool = typename std::enable_if<std::is_same<V, bool>::value, bool>::type;

    template<typename V>
    using enable_if_double = typename std::enable_if<std::is_floating_point<V>::value, bool>::type;

    template<typename V>
    using enable_if_string = typename std::enable_if<std::is_same<V, std::string>::value, bool>::type;

    template<typename V>
    using enable_if_char_ptr = typename std::enable_if<std::is_same<V, const char*>::value, bool>::type;

    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_int<ValueType>
    validate(const Layer& layer, ValueType expectation) noexcept {
      return layer.is_int() && expectation == layer.get_int();
    }

    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_bool<ValueType>
    validate(const Layer& layer, ValueType expectation) noexcept {
      return layer.is_bool() && expectation == layer.get_bool();
    }

    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_double<ValueType>
    validate(const Layer& layer, ValueType expectation) noexcept {
      return layer.is_double() && expectation == layer.get_double();
    }

    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_string<ValueType>
    validate(const Layer& layer, const ValueType& expectation) noexcept {
      return layer.is_string() && !expectation.compare(layer.get_cstr());
    }
    
    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_char_ptr<ValueType>
    validate(const Layer& layer, ValueType expectation) noexcept {
      return layer.is_string() && !strcmp(expectation, layer.get_cstr());
    }
  };

  template<>
  struct literal_tag<VoidType> {
    using context_type = constraint_context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const context_type& context) noexcept {
      if (layer.is_null())
        return context.ok();
      return context.fail("invalid value.");
    }

    template<GARLIC_VIEW Layer>
    static bool quick_test(const Layer& layer, const context_type& context) noexcept {
      return layer.is_null();
    }
  };

  using string_literal_tag = literal_tag<std::string>;
  using int_literal_tag    = literal_tag<int>;
  using double_literal_tag = literal_tag<double>;
  using bool_literal_tag   = literal_tag<bool>;
  using null_literal_tag   = literal_tag<VoidType>;

  //! A named group of Constraint elements.
  class Field {
  public:

    using const_constraint_iterator = typename sequence<Constraint>::const_iterator;

    //! List of all failed constraints.
    struct ValidationResult {
      using constraint_sequence = sequence<ConstraintResult>;

      constraint_sequence failures; //!< all failed constraints.

      //! \return whether or not the layer is valid.
      inline bool is_valid() const noexcept { return failures.empty(); }

      //! \copydoc is_valid()
      inline operator bool() const { return is_valid(); }
    };

    struct Properties {
      std::unordered_map<text, text> annotations;
      sequence<Constraint> constraints;
      text name;
      bool ignore_details = false;
    };
    
    Field(Properties&& properties) : properties_(std::move(properties)) {}

    //! Create an empty Field by a name.
    Field(text&& name) { properties_.name = std::move(name); }

    //! Create a Field by a name and a defined set of constraints.
    Field(text&& name, sequence<Constraint>&& constraints) {
      properties_.name = std::move(name);
      properties_.constraints = std::move(constraints);
    }

    //! Helper method to create and add a constriant to the Field.
    /*! Uses make_constraint() and adds the resulting Constraint to the Filed.
     */
    template<typename Tag, typename... Args>
    void add_constraint(Args&&... args) noexcept {
      this->add_constraint(make_constraint<Tag>(std::forward<Args>(args)...));
    }

    //! Add a constraint to the Field.
    void add_constraint(Constraint&& constraint) {
      properties_.constraints.push_back(std::move(constraint));
    }

    //! Adds all the constraints from the specified field to the front.
    inline void inherit_constraints_from(const Field& another) {
      properties_.constraints.push_front(
          another.begin_constraints(),
          another.end_constraints());
    }

    //! \return annotations object.
    auto& annotations() noexcept { return properties_.annotations; }

    //! \return annotations object.
    const auto& annotations() const noexcept { return properties_.annotations; }

    /*! \return whether or not the field should ignore details and return
     *          a leaf ConstraintResult when testing/validating.
     */
    bool ignore_details() const { return properties_.ignore_details; }

    //! Set whether or not the field should ignore details.
    void set_ignore_details(bool value) { properties_.ignore_details = value; }

    //! \return the message to display when making failed ConstraintResult instances.
    text message() const noexcept {
      const auto& annotations = properties_.annotations;
      if (auto it = annotations.find("message"); it != annotations.end()) {
        return it->second;
      }
      return text::no_text();
    }
    //! \return name of the field.
    const text& name() const noexcept { return properties_.name; }
    const Properties& properties() const noexcept { return properties_; }

    //! \return an iterator (const) pointing to the first constraint in the field.
    const_constraint_iterator begin_constraints() const noexcept { return properties_.constraints.begin(); }

    //! \return an iterator (const) pointing to one past the last constraint in the field.
    const_constraint_iterator end_constraints() const noexcept { return properties_.constraints.end(); }

    //! test the layer with all the constraints in the field.
    template<GARLIC_VIEW Layer>
    ValidationResult validate(const Layer& layer) const noexcept {
      ValidationResult result;
      test_constraints(layer, properties_.constraints, std::back_inserter(result.failures));
      return result;
    }

    //! perform a quick and efficient test of all constraints in the field.
    template<GARLIC_VIEW Layer>
    bool quick_test(const Layer& layer) const noexcept {
      return test_constraints_quick(layer, properties_.constraints);
    }

  protected:
    Properties properties_;
  };


  //! An object to describe a Model.
  /*! A model will contain a table basic_text -> {field: Field, required: bool}
   */
  class Model {
  public:

    using field_pointer = std::shared_ptr<Field>;

    //! Field Record.
    struct FieldDescriptor {
      field_pointer field;  //!< Field instance.
      bool required;  //!< whether or not this Field is required.
    };

    using const_field_iterator = typename std::unordered_map<text, FieldDescriptor>::const_iterator;

    struct Properties {
      std::unordered_map<text, FieldDescriptor> field_map;
      std::unordered_map<text, text> annotations;
      text name;
      bool strict = false;
    };

    //! Create an empty Model.
    Model() {}

    Model(Properties&& properties) : properties_(std::move(properties)) {}

    //! Create an empty Model by name.
    Model(text&& name) {
      properties_.name = std::move(name);
    }

    //! Add a new field.
    /*! \param name the name/key of the member.
     *  \param field std::shared_ptr<Field>
     *  \param required whether or not this field is required.
     *  \note If a member by such name already exists, this function will not do anything.
     */
    void add_field(text&& name, field_pointer field, bool required = true) {
      properties_.field_map.emplace(
          std::move(name),
          FieldDescriptor { .field = std::move(field), .required = required }
          );
    }

    //! \return a shared pointer to the Field instance or nullptr if such key is not defined yet.
    template<typename KeyType>
    field_pointer get_field(KeyType&& name) const {
      auto it = properties_.field_map.find(name);
      if (it != properties_.field_map.end()) {
        return it->second.field;
      }
      return nullptr;
    }

    //! \return the model's annotations map.
    auto& annotations() noexcept { return properties_.annotations; }

    //! \copydoc annotations()
    const auto& annotations() const noexcept { return properties_.annotations; }

    //! \return name of the Model.
    const text& name() const noexcept { return properties_.name; }
    const Properties& properties() const { return properties_; }

    //! \return an iterator (const) pointing to the first (basic_text, FieldDescriptor) pair.
    const_field_iterator begin_fields() const noexcept { return properties_.field_map.begin(); }

    //! \return an iterator (const) pointing to one past the last (basic_text, FieldDescriptor) pair.
    const_field_iterator end_fields() const noexcept { return properties_.field_map.end(); }

    //! \return an iterator (const) pointing to found (basic_text, FieldDescriptor) pair or end_fields()
    const_field_iterator find_field(const text& name) const noexcept { return properties_.field_map.find(name); }

    //! Run a quick test on a layer.
    template<GARLIC_VIEW Layer>
    bool quick_test(const Layer& layer) const noexcept {
      if (!layer.is_object()) return false;
      std::unordered_set<text> requirements;
      for (const auto& member : layer.get_object()) {
        auto it = properties_.field_map.find(member.key.get_cstr());
        if (it == properties_.field_map.end()) continue;
        if (!it->second.field->quick_test(member.value)) {
          return false;
        }
        requirements.emplace(it->first);
      }
      for (const auto& item : properties_.field_map) {
        if (auto it = requirements.find(item.first); it != requirements.end()) continue;
        if (!item.second.required) continue;
        return false;
      }
      return true;
    }

    //! Validate a layer and return a detailed ConstraintResult.
    template<GARLIC_VIEW Layer>
    ConstraintResult validate(const Layer& layer) const noexcept {
      sequence<ConstraintResult> details;
      if (layer.is_object()) {
        // todo : if the container allows for atomic table look up, swap the loop.
        std::unordered_set<text> requirements;
        for (const auto& member : layer.get_object()) {
          auto it = properties_.field_map.find(member.key.get_cstr());
          if (it != properties_.field_map.end()) {
            this->test_field(details, member.key, member.value, it->second.field);
            requirements.emplace(it->first);
          }
        }
        for (const auto& item : properties_.field_map) {
          if (auto it = requirements.find(item.first); it != requirements.end()) continue;
          if (!item.second.required) continue;
          details.push_back(
              ConstraintResult::leaf_field_failure(
                item.first.view(),
                "missing required field!"));
        }
      } else {
        details.push_back(ConstraintResult::leaf_failure("type", "Expected object."));
      }
      if (!details.empty()) {
        return ConstraintResult {
          .details = std::move(details),
          .name = properties_.name.clone(),
          .reason = text("This model is invalid!"),
          .flag = ConstraintResult::flags::none
        };
      }

      return ConstraintResult::ok();
    }

  protected:
    Properties properties_;

  private:
    template<GARLIC_VIEW Layer>
    inline void
    test_field(
        sequence<ConstraintResult>& details,
        const Layer& key,
        const Layer& value,
        const field_pointer& field) const {
      if (field->properties().ignore_details) {
        if (!field->quick_test(value)) {
          details.push_back(ConstraintResult {
              .details = sequence<ConstraintResult>::no_sequence(),
              .name = decode<text>(key),
              .reason = field->message(),
              .flag = ConstraintResult::flags::field
              });
        }
      } else {
        auto test = field->validate(value);
        if (!test.is_valid()) {
          details.push_back(ConstraintResult {
              .details = std::move(test.failures),
              .name = decode<text>(key),
              .reason = field->message(),
              .flag = ConstraintResult::flags::field
              });
        }
      }
    }
  };

  /*! \brief Constraint Tag that passes if the specified Model passes the layer.
   *
   *  \code{.cpp}
   *  make_constraint<model_tag>(std::shared_ptr<Model>&& model);
   *  \endcode
   */
  struct model_tag {
    struct Context : public constraint_context {
      using model_pointer = std::shared_ptr<Model>;

      template<typename... Args>
      Context(model_pointer model, Args&&... args
          ) : constraint_context(std::forward<Args>(args)...), model(std::move(model)) {}

      Context(model_pointer model
          ) : constraint_context(model->name().view(), text::no_text(), true), model(std::move(model)) {}

      model_pointer model;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static inline ConstraintResult test(const Layer& layer, const Context& context) noexcept {
      return context.model->validate(layer);
    }

    template<GARLIC_VIEW Layer>
    static inline bool quick_test(const Layer& layer, const Context& context) noexcept {
      return context.model->quick_test(layer);
    }
  };

  static inline std::shared_ptr<Field> make_field(text&& name) {
    return std::make_shared<Field>(std::move(name));
  }

  static inline std::shared_ptr<Field> make_field(text&& name, sequence<Constraint>&& constraints) {
    return std::make_shared<Field>(std::move(name), std::move(constraints));
  }

  static inline std::shared_ptr<Field> make_field(sequence<Constraint>&& constraints) {
    return std::make_shared<Field>(text::no_text(), std::move(constraints));
  }

  template<typename... Args>
  static inline std::shared_ptr<Model> make_model(Args&&...args) {
    return std::make_shared<Model>(std::forward<Args>(args)...);
  }

  /*! \brief Constraint Tag that passes if the specified Field passes the layer.
   *
   *  \code{.cpp}
   *  make_constraint<field_tag>(std::shared_ptr<Field> pointer,
   *                             bool hide = false,
   *                             bool ignore_details = false);
   *  \endcode
   *  \param field_pointer a pointer to the Field instance.
   *  \param hide if true, ConstraintResult from the first failing constraint will be
   *              returned. This practically hides this constraint and leaves no trace
   *              of it in the resulting ConstraintResult.
   *  \param ignore_details if true, failing constraints will not be added to the resulting
   *                        ConstraintResult as details.
   */
  struct field_tag {
    struct Context : public constraint_context {
      using field_pointer = std::shared_ptr<Field>;
      using field_pointer_ref = std::shared_ptr<field_pointer>;

      template<typename... Args>
      Context(
          field_pointer_ref ref, bool hide = false, bool ignore_details = false,
          Args&&... args
          ) : constraint_context(std::forward<Args>(args)...),
              ref(std::move(ref)), hide(hide), ignore_details(ignore_details) {
                this->update_name();
              }

      template<typename... Args>
      Context(
          field_pointer pointer, bool hide = false, bool ignore_details = false,
          Args&&... args
          ) : Context(std::make_shared<field_pointer>(pointer), hide, ignore_details, std::forward<Args>(args)...) {}

      template<typename... Args>
      inline ConstraintResult
      custom_message_fail(Args&&... args) const noexcept {
        return this->fail((*ref)->message(), std::forward<Args>(args)...);
      }

      void update_name() {
        if (*ref && this->name.empty()) {
          this->name = (*ref)->name();
        }
      }

      void set_field(field_pointer field) {
        ref->swap(field);
        this->update_name();
      }

      field_pointer_ref ref;
      bool hide;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.hide) {
        return test_constraints_first_failure(layer, (*context.ref)->properties().constraints);
      }
      const auto& field = *context.ref;
      if (context.ignore_details || field->properties().ignore_details) {
        if (field->quick_test(layer)) return context.ok();
        return context.custom_message_fail();
      }
      auto result = field->validate(layer);
      if (result.is_valid()) return context.ok();
      return context.custom_message_fail(std::move(result.failures));
    }

    template<GARLIC_VIEW Layer>
    static inline bool quick_test(const Layer& layer, const Context& context) noexcept {
      return (*context.ref)->quick_test(layer);
    }
  };

  //! Built-in constraint tags.
  using constraint_registry = internal::registry<
    type_tag, range_tag, regex_tag, any_tag, list_tag, tuple_tag, map_tag, all_tag, model_tag, field_tag,
    string_literal_tag, int_literal_tag, double_literal_tag, bool_literal_tag, null_literal_tag>;

  template<typename Tag, typename... Args>
   Constraint Constraint::make(Args&&... args) noexcept {
     constexpr auto position = constraint_registry::position_of<Tag>();
     static_assert(position != constraint_registry::not_found, "Unregistered tag used to make a flat constraint.");
    return Constraint(
        position,
        std::make_shared<typename Tag::context_type>(std::forward<Args>(args)...));
  }

  template<GARLIC_VIEW Layer>
  inline ConstraintResult
  Constraint::test(const Layer& value) const noexcept {
    static constexpr auto handlers = constraint_registry::test_handlers<Layer>();
    return handlers[index_](value, context_.get());
  }

  template<GARLIC_VIEW Layer>
  inline bool
  Constraint::quick_test(const Layer& value) const noexcept {
    static constexpr auto handlers = constraint_registry::quick_test_handlers<Layer>();
    return handlers[index_](value, context_.get());
  }

  //! Create a generic Constraint based on a constraint tag.
  /*! \tparam Tag any **built-in** constraint tag that is registered in constraint_registry.
   *  \tparam Args appropriate set of arguments for the constraint tag used.
   *  \note You will get a compile-time error if you use a tag that is not registered.
   */
  template<typename Tag, typename... Args>
  inline Constraint
  make_constraint(Args&&... args) noexcept {
    return Constraint::make<Tag>(std::forward<Args>(args)...);
  }

}

#endif /* end of include guard: GARLIC_CONSTRAINTS_H */
