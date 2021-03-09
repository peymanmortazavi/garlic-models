#ifndef GARLIC_CONSTRAINTS
#define GARLIC_CONSTRAINTS

#include "layer.h"
#include "utility.h"
#include "meta.h"
#include "containers.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <regex>
#include <string>
#include <type_traits>


namespace garlic {

  struct ConstraintResult {
    enum flags : uint8_t {
      none      = 0x1 << 0,
      valid     = 0x1 << 1,
      field     = 0x1 << 2,
    };

    sequence<ConstraintResult> details;
    text name;
    text reason;
    flags flag;

    inline bool is_leaf()  const noexcept { return !details.size();  }
    inline bool is_valid() const noexcept { return flag & flags::valid; }
    inline bool is_field() const noexcept { return flag & flags::field; }

    inline void set_valid() noexcept { flag = static_cast<flags>(flag & flags::valid); }
    inline void set_field() noexcept { flag = static_cast<flags>(flag & flags::field); }

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


  struct ConstraintProperties {
    enum flags : uint8_t {
      none  = 0x1 << 0,
      fatal = 0x1 << 1,  // should stop looking at other constraints.
    };

    text name;  // constraint name.
    text message;  // custom rejection reason.
    flags flag = flags::none;

    static ConstraintProperties create_default(
        text&& name = text::no_text(), text&& message = text::no_text()) {
      return ConstraintProperties {
        std::move(name),
        std::move(message),
        flags::none,
      };
    }

    inline bool is_fatal() const noexcept { return flag & flags::fatal; }
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
        ) : name(name), message(message), flag(fatal ? flags::fatal : flags::none) {}

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
    auto fail(const char* message) const noexcept -> ConstraintResult {
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
    auto fail(const char* message, sequence<ConstraintResult>&& details) const noexcept {
      return ConstraintResult {
        .details = std::move(details),
        .name = name,
        .reason = (this->message.empty() ? message : this->message),
        .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
      };
    }

    template<bool Field = false>
    auto fail(const char* message, ConstraintResult&& inner_detail) const noexcept {
      sequence<ConstraintResult> details(1);
      details.push_back(std::move(inner_detail));
      return this->fail<Field>(message, std::move(details));
    }

    inline auto ok() const noexcept -> ConstraintResult {
      return ConstraintResult::ok();
    }
  };


  template<GARLIC_VIEW Layer, typename Container, typename BackInserterIterator>
  inline void test_constraints(
      Layer&& value,
      Container&& constraints,
      BackInserterIterator it) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.is_valid()) {
        it = std::move(result);
        if (constraint->skip_constraints()) break;
      }
    }
  }


  template<GARLIC_VIEW Layer, typename Container>
  static inline bool
  test_constraints_quick(Layer&& value, Container&& constraints) {
    return std::all_of(
        std::begin(constraints), std::end(constraints),
        [&value](const auto& constraint) { return constraint->quick_test(value); }
        );
  }


  template<GARLIC_VIEW Layer, typename Container>
  static inline ConstraintResult
  test_constraints_first_failure(Layer&& value, Container&& constraints) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.is_valid()) {
        return result;
      }
    }
    return ConstraintResult::ok();
  }


  template<GARLIC_VIEW Layer>
  static inline text 
  get_text(Layer&& layer, const char* key, text&& default_value) noexcept {
    get_member(layer, key, [&default_value](const auto& result) {
        auto view = result.get_string_view();
        default_value = text(view.data(), view.size(), text_type::copy);
        });
    return std::move(default_value);
  }


  template<bool Fatal = false, GARLIC_VIEW Layer>
  static inline ConstraintProperties
  build_constraint_properties(
      Layer&& layer,
      const char* name = "",
      const char* message = "") noexcept {
    return ConstraintProperties {
      .name = get_text(layer, "name", name),
      .message = get_text(layer, "message", message),
      .flag = (get(layer, "fatal", Fatal) ? ConstraintProperties::flags::fatal : ConstraintProperties::flags::none),
    };
  }

  template<typename T>
  using constraint_test_handler = ConstraintResult (*)(const T&, const constraint_context*);

  template<typename T>
  using constraint_quick_test_handler = bool (*)(const T&, const constraint_context*);

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
      return rest::template position_of<Query>() + 1;
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

  class FlatConstraint final {
  private:
    using context_pointer = std::unique_ptr<constraint_context>;
    context_pointer context_;
    unsigned index_;

    FlatConstraint(unsigned index, context_pointer&& context) : context_(std::move(context)), index_(index) {}

  public:
    template<typename Tag, typename... Args>
    static FlatConstraint make(Args&&... args) noexcept;

    template<GARLIC_VIEW Layer>
    inline ConstraintResult test(const Layer& value) const noexcept;

    template<GARLIC_VIEW Layer>
    inline bool quick_test(const Layer& value) const noexcept;

    inline const constraint_context& context() const noexcept {
      return *context_;
    }

    template<typename, typename... Args>
    friend inline std::shared_ptr<FlatConstraint> make_constraint(Args&&...) noexcept;
  };


  template<GARLIC_VIEW Layer, typename Container, typename BackInserterIterator>
  inline void test_flat_constraints(
      Layer&& value,
      Container&& constraints,
      BackInserterIterator it) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.is_valid()) {
        it = std::move(result);
        if (constraint->context().is_fatal())
          break;
      }
    }
  }


  template<GARLIC_VIEW Layer, typename Container>
  static inline bool
  test_flat_constraints_quick(Layer&& value, Container&& constraints) {
    return std::all_of(
        std::begin(constraints), std::end(constraints),
        [&value](const auto& constraint) { return constraint->quick_test(value); }
        );
  }


  template<GARLIC_VIEW Layer, typename Container>
  static inline ConstraintResult
  test_flat_constraints_first_failure(Layer&& value, Container&& constraints) {
    for (const auto& constraint : constraints) {
      if (auto result = constraint->test(value); !result.is_valid()) {
        return result;
      }
    }
    return ConstraintResult::ok();
  }

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
        size_type count = 0;
        for (const auto& item : layer.get_list()) {
          ++count;
          if (count > context.max) return context.fail("too many items in the list.");
        }
        if (count < context.min) return context.fail("too few items in the list.");
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
        size_type count = 0;
        for (const auto& item : layer.get_list()) {
          ++count;
          if (count > context.max) return false;
        }
        if (count < context.min) return false;
      }
      return true;

    }
  };


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


  struct any_tag {
    struct Context : public constraint_context {
      using constraint_pointer = std::shared_ptr<FlatConstraint>;

      template<typename... Args>
      Context(sequence<constraint_pointer>&& constraints, Args&&... args
          ) : constraint_context(std::forward<Args>(args)...), constraints(std::move(constraints)) {}

      sequence<constraint_pointer> constraints;
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
          [&layer](const auto& item) { return item->quick_test(layer); });
    }
  };


  struct list_tag {
    struct Context : public constraint_context {
      using constraint_pointer = std::shared_ptr<FlatConstraint>;

      template<typename... Args>
      Context(
          constraint_pointer&& ref, bool ignore_details = false,
          text&& name = "list_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              constraint(std::move(ref)), ignore_details(ignore_details) {}

      constraint_pointer constraint;
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
          [&context](const auto& item) { return context.constraint->quick_test(item); }
          );
    }

    template<bool IgnoreDetails, GARLIC_VIEW Layer>
    inline static ConstraintResult test(const Layer& layer, const Context& context) noexcept {
      size_t index = 0;
      for (const auto& item : layer.get_list()) {
        if (IgnoreDetails) {  // no runtime penalty since IgnoreDetails is a template parameter.
          if (!context.constraint->quick_test(item)) {
            return context.fail(
                "Invalid value found in the list.",
                ConstraintResult::leaf_field_failure(
                  text(std::to_string(index), text_type::copy),
                  "invalid value."));
          }
        } else {
          auto result = context.constraint->test(item);
          if (!result.is_valid()) {
            sequence<ConstraintResult> inner_details(1);
            inner_details.push_back(std::move(result));
            return context.fail(
                "Invalid value found in the list.",
                ConstraintResult::field_failure(
                  text(std::to_string(index), text_type::copy),
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

  class tuple_tag {
  public:
    struct Context : public constraint_context {
      using constraint_pointer = std::shared_ptr<FlatConstraint>;

      template<typename... Args>
      Context(
          sequence<constraint_pointer>&& constraints, bool strict, bool ignore_details,
          text&& name = "tuple_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              constraints(std::move(constraints)), strict(strict), ignore_details(ignore_details) {}

      sequence<constraint_pointer> constraints;
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
        if (!(*constraint_it)->quick_test(*tuple_it)) return false;
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
          auto result = (*constraint_it)->quick_test(*tuple_it);
          if (!result) {
            return context.fail(
                "Invalid value found in the tuple.",
                ConstraintResult::leaf_field_failure(
                  text(std::to_string(index), text_type::copy),
                  "invalid value."
                  ));
          }
        } else {
          auto result = (*constraint_it)->test(*tuple_it);
          if (!result.is_valid()) {
            sequence<ConstraintResult> inner_details(1);
            inner_details.push_back(std::move(result));
            return context.fail(
                "Invalid value found in the tuple.",
                ConstraintResult::field_failure(
                  text(std::to_string(index), text_type::copy),
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

  class map_tag {
  public:
    struct Context : public constraint_context {
      using constriant_pointer = std::shared_ptr<FlatConstraint>;

      template<typename... Args>
      Context(
          constriant_pointer&& key_constraint, constriant_pointer&& value_constraint, bool ignore_details = false,
          text&& name = "map_constraint", Args&&... args
          ) : constraint_context(std::move(name), std::forward<Args>(args)...),
              key(std::move(key_constraint)), value(std::move(value_constraint)), ignore_details(ignore_details) {}

      constriant_pointer key;
      constriant_pointer value;
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
            if (HasKeyConstraint && !context.key->quick_test(item.key)) return false;
            if (HasValueConstraint && !context.value->quick_test(item.value)) return false;
            return true;
            });
    }

    template<bool IgnoreDetails, GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.key) {
        if (context.value) return test<IgnoreDetails, true, true>(layer, context);
        return test<IgnoreDetails, true, false>(layer, context);
      } else if (context.key) return test<IgnoreDetails, false, true>(layer, context);
      return context.ok();  // meaning there is no key or value constraint.
    }

    template<bool IgnoreDetails, bool HasKeyConstraint, bool HasValueConstraint, GARLIC_VIEW Layer>
    static inline ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      for (const auto& item : layer.get_object()) {
        if (HasKeyConstraint) {
          if (IgnoreDetails && !context.key->quick_test(item.key)) {
            return context.fail("Object contains invalid key.");
          } else {
            if (auto result = context.key->test(item.key); !result.is_valid()) {
              return context.fail("Object contains invalid key.", std::move(result));
            }
          }
        }
        if (HasValueConstraint) {
          if (IgnoreDetails && !context.value->quick_test(item.value)) {
            return context.fail("Object contains invalid value.");
          } else {
            if (auto result = context.value->test(item.value); !result.is_valid()) {
              return context.fail("Object contains invalid value.", std::move(result));
            }
          }
        }
      }
      return context.ok();
    }
  };

  struct all_tag {
    struct Context : public constraint_context {
      using constraint_pointer = std::shared_ptr<FlatConstraint>;

      template<typename... Args>
      Context(
          sequence<constraint_pointer>&& constraints, bool hide = true, bool ignore_details = false,
          Args&&... args
          ) : constraint_context(std::forward<Args>(args)...),
              constraints(std::move(constraints)), hide(hide), ignore_details(ignore_details) {}

      sequence<constraint_pointer> constraints;
      bool hide;
      bool ignore_details;
    };

    using context_type = Context;

    template<GARLIC_VIEW Layer>
    static ConstraintResult
    test(const Layer& layer, const Context& context) noexcept {
      if (context.hide)
        return test_flat_constraints_first_failure(layer, context.constraints);
      if (context.ignore_details) {
        if (test_flat_constraints_quick(layer, context.constraints))
          return context.ok();
        else
          return context.fail("Some of the constraints fail on this value.");
      }
      sequence<ConstraintResult> results;
      test_flat_constraints(layer, context.constraints, std::back_inserter(results));
      if (results.empty())
        return context.ok();
      return context.fail("Some of the constraints fail on this value.", std::move(results));
    }

    template<GARLIC_VIEW Layer>
    static bool quick_test(const Layer& layer, const Context& context) noexcept {
      return test_flat_constraints_quick(layer, context.constraints);
    }
  };

  template<typename T>
  struct literal_tag {
    struct Context : public constraint_context {

      template<typename V, typename... Args>
      Context(Args&&... args,
          typename std::enable_if<std::is_same<V, VoidType>::value, VoidType>::type value = VoidType()
          ) : constraint_context(std::forward<Args>(args)...), value(value) {}

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

    template<typename V>
    using enable_if_null = typename std::enable_if<std::is_same<V, VoidType>::value, bool>::type;

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

    template<typename ValueType, GARLIC_VIEW Layer>
    static inline enable_if_null<ValueType>
    validate(const Layer& layer, ValueType expectation) noexcept {
      return layer.is_null();
    }
  };

  using string_literal_tag = literal_tag<std::string>;
  using int_literal_tag    = literal_tag<int>;
  using double_literal_tag = literal_tag<double>;
  using bool_literal_tag   = literal_tag<bool>;
  using null_literal_tag   = literal_tag<VoidType>;

  using default_registry = registry<
    type_tag,
    range_tag,
    regex_tag,
    any_tag,
    list_tag,
    tuple_tag,
    map_tag,
    all_tag,
    string_literal_tag, int_literal_tag, double_literal_tag, bool_literal_tag, null_literal_tag>;

  template<typename Tag, typename... Args>
   FlatConstraint FlatConstraint::make(Args&&... args) noexcept {
    return FlatConstraint(
        default_registry::position_of<Tag>(),
        std::make_unique<typename Tag::context_type>(std::forward<Args>(args)...));
  }

  template<GARLIC_VIEW Layer>
  inline ConstraintResult
  FlatConstraint::test(const Layer& value) const noexcept {
    static constexpr auto handlers = default_registry::test_handlers<Layer>();
    return handlers[index_](value, context_.get());
  }

  template<GARLIC_VIEW Layer>
  inline bool
  FlatConstraint::quick_test(const Layer& value) const noexcept {
    static constexpr auto handlers = default_registry::quick_test_handlers<Layer>();
    return handlers[index_](value, context_.get());
  }

  template<typename Tag, typename... Args>
  inline std::shared_ptr<FlatConstraint>
  make_constraint(Args&&... args) noexcept {
    return std::shared_ptr<FlatConstraint>(
        new FlatConstraint(
          default_registry::position_of<Tag>(),
          std::make_unique<typename Tag::context_type>(std::forward<Args>(args)...)));
  }

  template<GARLIC_VIEW LayerType>
  class Constraint {
  public:
    explicit Constraint(ConstraintProperties&& props) : props_(std::move(props)) {}

    virtual ConstraintResult test(const LayerType& value) const noexcept = 0;
    virtual bool quick_test(const LayerType& value) const noexcept = 0;
    const text& get_name() const noexcept { return props_.name; };
    inline bool skip_constraints() const noexcept { return props_.is_fatal(); };

    template<bool Field = false, bool Leaf = true>
    auto fail() const noexcept -> ConstraintResult {
      return ConstraintResult {
        .details = (Leaf ? sequence<ConstraintResult>::no_sequence() : sequence<ConstraintResult>()),
        .name = props_.name,
        .reason = props_.message,
        .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
      };
    }

    template<bool Field = false, bool Leaf = true>
    auto fail(const char* message) const noexcept -> ConstraintResult {
      if (this->props_.message.empty())
        return ConstraintResult {
          .details = (Leaf ? sequence<ConstraintResult>::no_sequence() : sequence<ConstraintResult>()),
          .name = props_.name,
          .reason = message,
          .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
        };
      return this->fail<Field, Leaf>();
    }

    template<bool Field = false>
    auto fail(const char* message, sequence<ConstraintResult>&& details) const noexcept {
      return ConstraintResult {
        .details = std::move(details),
        .name = props_.name,
        .reason = (props_.message.empty() ? message : props_.message),
        .flag = (Field ? ConstraintResult::flags::field : ConstraintResult::flags::none)
      };
    }

    template<bool Field = false>
    auto fail(const char* message, ConstraintResult&& inner_detail) const noexcept {
      sequence<ConstraintResult> details(1);
      details.push_back(std::move(inner_detail));
      return this->fail<Field>(message, std::move(details));
    }

    inline auto ok() const noexcept -> ConstraintResult {
      return ConstraintResult::ok();
    }

  protected:
    ConstraintProperties props_;
  };


  template<GARLIC_VIEW LayerType>
  class TypeConstraint : public Constraint<LayerType> {
  public:

    TypeConstraint(
        TypeFlag required_type,
        text&& name = "type_constraint"
        ) : Constraint<LayerType>(ConstraintProperties {
          .name = std::move(name),
          .message = text::no_text(),
          .flag = ConstraintProperties::flags::fatal,
          }), flag_(required_type) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: {
          if (value.is_null()) { return this->ok(); }
          else return this->fail("Expected null.");
        }
        case TypeFlag::Boolean: {
          if (value.is_bool()) { return this->ok(); }
          else return this->fail("Expected boolean type.");
        }
        case TypeFlag::Double: {
          if (value.is_double()) { return this->ok(); }
          else return this->fail("Expected double type.");
        }
        case TypeFlag::Integer: {
          if (value.is_int()) { return this->ok(); }
          else return this->fail("Expected integer type.");
        }
        case TypeFlag::String: {
          if (value.is_string()) { return this->ok(); }
          else return this->fail("Expected string type.");
        }
        case TypeFlag::List: {
          if (value.is_list()) { return this->ok(); }
          else return this->fail("Expected a list.");
        }
        case TypeFlag::Object: {
          if (value.is_object()) { return this->ok(); }
          else return this->fail("Expected an object.");
        }
        default: return this->fail();
      }
    }

    bool quick_test(const LayerType& value) const noexcept override {
      switch (flag_) {
        case TypeFlag::Null: return value.is_null();
        case TypeFlag::Boolean: return value.is_bool();
        case TypeFlag::Double: return value.is_double();
        case TypeFlag::Integer: return value.is_int();
        case TypeFlag::String: return value.is_string();
        case TypeFlag::List: return value.is_list();
        case TypeFlag::Object: return value.is_object();
        default: return false;
      }
    }

    private:
      TypeFlag flag_;
  };


  template<GARLIC_VIEW LayerType>
  class RangeConstraint : public Constraint<LayerType> {
  public:
    using SizeType = size_t;

    RangeConstraint(
        SizeType min,
        SizeType max,
        text&& name = "range_constraint"
        ) : min_(min), max_(max), Constraint<LayerType>(ConstraintProperties {
          .name = std::move(name),
          .message = text::no_text(),
          .flag = ConstraintProperties::flags::none,
          }) {}

    RangeConstraint(
        SizeType min,
        SizeType max,
        ConstraintProperties&& props
        ) : min_(min), max_(max), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return this->fail("invalid string length.");
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return this->fail("out of range value.");
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(static_cast<SizeType>(ivalue) > max_ || static_cast<SizeType>(ivalue) < min_) return this->fail("out of range value.");
      } else if (value.is_list()) {
        SizeType count = 0;
        for (const auto& item : value.get_list()) {
          ++count;
          if (count > max_) return this->fail("too many items in the list.");
        }
        if (count < min_) return this->fail("too few items in the list.");
      }
      return this->ok();
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (value.is_string()) {
        auto length = value.get_string_view().size();
        if (length > max_ || length < min_) return false;
      } else if (value.is_double()) {
        auto dvalue = value.get_double();
        if(dvalue > max_ || dvalue < min_) return false;
      } else if (value.is_int()) {
        auto ivalue = value.get_int();
        if(static_cast<SizeType>(ivalue) > max_ || static_cast<SizeType>(ivalue) < min_) return false;
      } else if (value.is_list()) {
        SizeType count = 0;
        for (const auto& item : value.get_list()) {
          ++count;
          if (count > max_) return false;
        }
        if (count < min_) return false;
      }
      return true;
    }

  private:
    SizeType min_;
    SizeType max_;
    std::string name_;
  };


  template<GARLIC_VIEW LayerType>
  class RegexConstraint : public Constraint<LayerType> {
  public:

    RegexConstraint(
        const text& pattern,
        text&& name = "regex_constraint"
        ) : Constraint<LayerType>(ConstraintProperties {
          .name = std::move(name),
          .message = text::no_text(),
          .flag = ConstraintProperties::flags::none,
          }), pattern_(pattern.data(), pattern.size()) {}

    RegexConstraint(
        const text& pattern,
        ConstraintProperties&& props
        ) : Constraint<LayerType>(std::move(props)), pattern_(pattern.data(), pattern.size()) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return this->ok();
      if (std::regex_match(value.get_cstr(), pattern_)) { return this->ok(); }
      else { return this->fail("invalid value."); }
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_string()) return true;
      if (std::regex_match(value.get_cstr(), pattern_)) return true;
      else return false;
    }

  private:
    std::regex pattern_;
  };


  template<GARLIC_VIEW LayerType>
  class AnyConstraint : public Constraint<LayerType> {
  public:
    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    AnyConstraint(
        sequence<constraint_pointer>&& constraints,
        ConstraintProperties&& props
        ) : constraints_(std::move(constraints)), Constraint<LayerType>(std::move(props)) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (this->validate(value))
        return this->ok();
      return this->fail("None of the constraints read this value.");
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return this->validate(value);
  }

  private:
    sequence<constraint_pointer> constraints_;

    inline bool validate(const LayerType& value) const noexcept {
      return std::any_of(
          constraints_.begin(),
          constraints_.end(),
          [&value](const auto& item) { return item->quick_test(value); });
    }
  };


  template<GARLIC_VIEW LayerType>
  class ListConstraint : public Constraint<LayerType> {
  public:

    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    ListConstraint() : Constraint<LayerType>(ConstraintProperties::create_default("list_constraint")) {}

    ListConstraint(
        constraint_pointer&& constraint,
        ConstraintProperties&& props,
        bool ignore_details = false
        ) : constraint_(std::move(constraint)),
            Constraint<LayerType>(std::move(props)), ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_list()) return this->fail("Expected a list.");
      if (ignore_details_) return this->test<true>(value);
      return this->test<false>(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_list()) return false;
      return std::all_of(
          value.begin_list(), value.end_list(),
          [this](const auto& item) { return constraint_->quick_test(item); }
          );
    }

  private:
    constraint_pointer constraint_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      int index = 0;
      for (const auto& item : value.get_list()) {
        if (IgnoreDetails) {
          if (!constraint_->quick_test(item)) {
            return this->fail(
                "Invalid value found in the list.",
                ConstraintResult::leaf_field_failure(
                  text(std::to_string(index), text_type::copy),
                  "invalid value."));
          }
        } else {
          auto result = constraint_->test(item);
          if (!result.is_valid()) {
            sequence<ConstraintResult> inner_details(1);
            inner_details.push_back(std::move(result));
            return this->fail(
                "Invalid value found in the list.",
                ConstraintResult::field_failure(
                  text(std::to_string(index), text_type::copy),
                  std::move(result),
                  "invalid value."
                  ));
          }
        }
        ++index;
      }
      return this->ok();
    }
  };


  template<GARLIC_VIEW LayerType>
  class TupleConstraint : public Constraint<LayerType> {
  public:

    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    TupleConstraint() : Constraint<LayerType>(ConstraintProperties::create_default("tuple_constraint")) {}

    TupleConstraint(
      sequence<constraint_pointer>&& constraints,
      bool strict,
      ConstraintProperties&& props,
      bool ignore_details = false
      ) : constraints_(std::move(constraints)),
          strict_(strict),
          Constraint<LayerType>(std::move(props)),
          ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (ignore_details_)return this->test<true>(value);
      return this->test<false>(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_list()) return false;
      auto tuple_it = value.begin_list();
      auto constraint_it = constraints_.begin();
      while (constraint_it != constraints_.end() && tuple_it != value.end_list()) {
        if (!(*constraint_it)->quick_test(*tuple_it)) return false;
        std::advance(tuple_it, 1);
        std::advance(constraint_it, 1);
      }
      if (strict_ && tuple_it != value.end_list()) return false;
      if (constraint_it != constraints_.end()) return false;
      return true;
    }

  private:
    sequence<constraint_pointer> constraints_;
    bool strict_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      if (!value.is_list()) return this->fail("Expected a list (tuple).");
      int index = 0;
      auto tuple_it = value.begin_list();
      auto constraint_it = constraints_.begin();
      while (constraint_it != constraints_.end() && tuple_it != value.end_list()) {
        if (IgnoreDetails) {
          auto result = (*constraint_it)->quick_test(*tuple_it);
          if (!result) {
            return this->fail(
                "Invalid value found in the tuple.",
                ConstraintResult::leaf_field_failure(
                  text(std::to_string(index), text_type::copy),
                  "invalid value."
                  ));
          }
        } else {
          auto result = (*constraint_it)->test(*tuple_it);
          if (!result.is_valid()) {
            sequence<ConstraintResult> inner_details(1);
            inner_details.push_back(std::move(result));
            return this->fail(
                "Invalid value found in the tuple.",
                ConstraintResult::field_failure(
                  text(std::to_string(index), text_type::copy),
                  std::move(result),
                  "invalid value."
                  ));
          }
        }
        std::advance(tuple_it, 1);
        std::advance(constraint_it, 1);
        ++index;
      }
      if (strict_ && tuple_it != value.end_list()) {
        return this->fail("Too many values in the tuple.");
      }
      if (constraint_it != constraints_.end()) {
        return this->fail("Too few values in the tuple.");
      }
      return this->ok();
    }
  };


  template<GARLIC_VIEW LayerType>
  class MapConstraint : public Constraint<LayerType> {
  public:
    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    MapConstraint() : Constraint<LayerType>(ConstraintProperties::create_default("map_constraint")) {}

    MapConstraint(
      constraint_pointer&& key_constraint,
      constraint_pointer&& value_constraint,
      ConstraintProperties&& props,
      bool ignore_details = false
      ) : key_constraint_(std::move(key_constraint)),
          value_constraint_(std::move(value_constraint)),
          Constraint<LayerType>(std::move(props)),
          ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (!value.is_object()) return this->fail("Expected an object.");
      if (ignore_details_) return this->test<true>(value);
      return this->test<false>(value);
    }

    bool quick_test(const LayerType& value) const noexcept override {
      if (!value.is_object()) return false;
      if (key_constraint_) {
        if (value_constraint_) return quick_test<true, true>(value);
        return quick_test<true, false>(value);
      } else if (value_constraint_) return quick_test<false, true>(value);
      return true;  // meaning there is no key or value constraint.
    }

  private:
    constraint_pointer key_constraint_;
    constraint_pointer value_constraint_;
    bool ignore_details_;

    template<bool IgnoreDetails>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      if (key_constraint_) {
        if (value_constraint_) return test<IgnoreDetails, true, true>(value);
        return test<IgnoreDetails, true, false>(value);
      } else if (value_constraint_) return test<IgnoreDetails, false, true>(value);
      return this->ok();  // meaning there is no key or value constraint.
    }

    template<bool IgnoreDetails, bool HasKeyConstraint, bool HasValueConstraint>
    inline ConstraintResult test(const LayerType& value) const noexcept {
      for (const auto& item : value.get_object()) {
        if (HasKeyConstraint) {
          if (IgnoreDetails && !key_constraint_->quick_test(item.key)) {
            return this->fail("Object contains invalid key.");
          } else {
            if (auto result = key_constraint_->test(item.key); !result.is_valid()) {
              return this->fail("Object contains invalid key.", std::move(result));
            }
          }
        }
        if (HasValueConstraint) {
          if (IgnoreDetails && !value_constraint_->quick_test(item.value)) {
            return this->fail("Object contains invalid value.");
          } else {
            if (auto result = value_constraint_->test(item.value); !result.is_valid()) {
              return this->fail("Object contains invalid value.", std::move(result));
            }
          }
        }
      }
      return this->ok();
    }

    template<bool HasKeyConstraint, bool HasValueConstraint>
    inline bool quick_test(const LayerType& value) const noexcept {
      return std::all_of(
          value.begin_member(), value.end_member(),
          [this](const auto& item) {
            if (HasKeyConstraint && !key_constraint_->quick_test(item.key)) return false;
            if (HasValueConstraint && !value_constraint_->quick_test(item.value)) return false;
            return true;
          }
          );
    }
  };


  template<GARLIC_VIEW LayerType>
  class AllConstraint : public Constraint<LayerType> {
  public:

    using constraint_type = Constraint<LayerType>;
    using constraint_pointer = std::shared_ptr<constraint_type>;

    AllConstraint(
        ) : Constraint<LayerType>(ConstraintProperties {
          .name = text::no_text(),
          .message = text::no_text(),
          .flag = ConstraintProperties::flags::fatal,
          }) {}

    AllConstraint(
      sequence<constraint_pointer>&& constraints,
      ConstraintProperties&& props,
      bool hide = true,
      bool ignore_details = false
      ) : constraints_(std::move(constraints)),
          Constraint<LayerType>(std::move(props)),
          hide_(hide), ignore_details_(ignore_details) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      if (hide_) return this->test_hide(value);
      if (ignore_details_) {
        if (test_constraints_quick(value, constraints_)) return this->ok();
        else return this->fail("Some of the constraints fail on this value.");
      }
      sequence<ConstraintResult> results;
      test_constraints(value, constraints_, std::back_inserter(results));
      if (results.empty())
        return this->ok();
      return this->fail("Some of the constraints fail on this value.", std::move(results));
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return test_constraints_quick(value, constraints_);
    }

  private:
    sequence<constraint_pointer> constraints_;
    bool hide_;
    bool ignore_details_;

    inline ConstraintResult test_hide(const LayerType& value) const noexcept {
      return test_constraints_first_failure(value, constraints_);
    }
  };

  template<GARLIC_VIEW LayerType, typename ValueType = VoidType>
  class LiteralConstraint : public Constraint<LayerType> {
  public:

    template<typename V = ValueType>
    LiteralConstraint(
        ConstraintProperties&& props,
        std::enable_if_t<std::is_same<V, VoidType>::value, VoidType> = VoidType {}
        ) : Constraint<LayerType>(std::move(props)) {}

    LiteralConstraint(
        ConstraintProperties&& props,
        ValueType value
        ) : Constraint<LayerType>(std::move(props)), value_(value) {}

    ConstraintResult test(const LayerType& value) const noexcept override {
      return (this->validate(value, value_) ? this->ok() : this->fail("invalid value."));
    }

    bool quick_test(const LayerType& value) const noexcept override {
      return this->validate(value, value_);
    }

  private:
    ValueType value_;

    template<typename V>
    static inline
    std::enable_if_t<std::is_same<V, int>::value, bool>
    validate(const LayerType& value, V expectation) noexcept {
      return value.is_int() && expectation == value.get_int();
    }

    template<typename V>
    static inline
    std::enable_if_t<std::is_floating_point<V>::value, bool>
    validate(const LayerType& value, const V& expectation) noexcept {
      return value.is_double() && expectation == value.get_double();
    }

    template<typename V>
    static inline
    std::enable_if_t<is_std_string<V>::value, bool>
    validate(const LayerType& value, const V& expectation) noexcept {
      return value.is_string() && !expectation.compare(value.get_cstr());
    }

    template<typename V>
    static inline
    std::enable_if_t<std::is_same<V, const char*>::value, bool>
    validate(const LayerType& value, V expectation) noexcept {
      return value.is_string() && !strcmp(expectation, value.get_cstr());
    }

    template<typename V>
    static inline
    std::enable_if_t<std::is_same<V, bool>::value, bool>
    validate(const LayerType& value, V expectation) noexcept {
      return value.is_bool() && expectation == value.get_bool();
    }

    template<typename V>
    static inline 
    std::enable_if_t<std::is_same<V, VoidType>::value, bool>
    validate(const LayerType& value, V expectation) noexcept {
      return value.is_null();
    }
  };

}

#endif /* end of include guard: GARLIC_CONSTRAINTS */
