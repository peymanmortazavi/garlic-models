#ifndef GARLIC_LIBYAML_LAYER_H
#define GARLIC_LIBYAML_LAYER_H

#include "../../layer.h"
#include "../../parsing/numbers.h"

#include "yaml.h"


namespace garlic::adapters::libyaml {

  class YamlView {

    struct ValueIteratorWrapper {
      using iterator_type = yaml_node_item_t*;
      using output_type = YamlView;

      iterator_type iterator;
      yaml_document_t* doc;

      inline YamlView wrap() const {
        return YamlView(doc, yaml_document_get_node(doc, *iterator));
      }
    };

    struct MemberIteratorWrapper {
      using iterator_type = yaml_node_pair_t*;
      using output_type = MemberPair<YamlView>;

      iterator_type iterator;
      yaml_document_t* doc;

      inline output_type wrap() const {
        return output_type {
          YamlView{doc, yaml_document_get_node(doc, iterator->key)},
          YamlView{doc, yaml_document_get_node(doc, iterator->value)},
        };
      }
    };

  public:
    using ConstValueIterator = RandomAccessIterator<ValueIteratorWrapper>;
    using ConstMemberIterator = RandomAccessIterator<MemberIteratorWrapper>;

    YamlView (yaml_document_t* doc, yaml_node_t* node) : doc_(doc), node_(node) {}
    YamlView (yaml_document_t* doc) : doc_(doc) { node_ = yaml_document_get_root_node(doc); }

    bool is_null() const {
      return node_->type == yaml_node_type_t::YAML_NO_NODE || (
          is_string() && strcmp("null", get_cstr()) == 0
          );
    }
    bool is_int() const noexcept {
      int holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseInt(scalar_data(), holder)
      );
    }
    bool is_string() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_double() const noexcept {
      double holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseDouble(scalar_data(), holder)
      );
    }
    bool is_object() const noexcept { return node_->type == yaml_node_type_t::YAML_MAPPING_NODE; }
    bool is_list() const noexcept { return node_->type == yaml_node_type_t::YAML_SEQUENCE_NODE; }
    bool is_bool() const noexcept {
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        check_bool(scalar_data(), [](auto){})
      );
    }

    char* scalar_data() const noexcept { return (char*)node_->data.scalar.value; }
    int get_int() const noexcept {
      int result = 0;
      parsing::ParseInt(scalar_data(), result);
      return result;
    }
    std::string get_string() const noexcept { return std::string{scalar_data()}; }
    std::string_view get_string_view() const noexcept { return std::string_view{scalar_data()}; }
    const char* get_cstr() const noexcept { return scalar_data(); }
    double get_double() const noexcept {
      double result;
      parsing::ParseDouble(scalar_data(), result);
      return result;
    }
    bool get_bool() const noexcept {
      bool result = false;
      check_bool(scalar_data(), [&result](auto r) { result = r; });
      return result;
    }

    ConstValueIterator begin_list() const { return ConstValueIterator({node_->data.sequence.items.start, doc_}); }
    ConstValueIterator end_list() const { return ConstValueIterator({node_->data.sequence.items.top, doc_}); }
    auto get_list() const { return ConstListRange<YamlView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator({node_->data.mapping.pairs.start, doc_}); }
    ConstMemberIterator end_member() const { return ConstMemberIterator({node_->data.mapping.pairs.top, doc_}); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return strcmp(key, item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(std::string_view key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return key.compare(item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(const YamlView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlView>{*this}; }

    const yaml_node_t& get_inner_value() const { return *node_; }
    const yaml_document_t& get_inner_document() const { return *doc_; }
    YamlView get_view() const { return YamlView(*this); }

  private:
    yaml_document_t* doc_;
    yaml_node_t* node_;

    template<typename Callable>
    bool check_bool(const char* input, const Callable& cb) const {
      struct pair {
        const char* name;
        bool value;
      };
      static const pair pairs[8] = {
        {"y", true}, {"n", false},
        {"true", true}, {"false", false},
        {"on", true}, {"off", false},
        {"yes", true}, {"no", false},
      };
      for(auto it = std::begin(pairs); it != std::end(pairs); it++) {
        if (strcmp(it->name, input) == 0) {
          cb(it->value);
          return true;
        }
      }
      return false;
    }
  };

  class YamlDocument {
  public:
    ~YamlDocument() { yaml_document_delete(&doc_); }

    YamlView get_view() {
      return YamlView{&doc_};
    }
    yaml_document_t* get_inner_document() { return &doc_; }

  protected:
    yaml_document_t doc_;
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_LAYER_H */