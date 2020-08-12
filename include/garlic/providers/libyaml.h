#ifndef GARLIC_LIBYAML_H
#define GARLIC_LIBYAML_H

#include "yaml.h"
#include <cstdlib>
#include <iterator>
#include <string>
#include "../layer.h"


namespace garlic::providers::libyaml {

  template<typename ValueType, typename KeyType = ValueType>
  class MemberIterator {
  public:
    struct MemberWrapper {
      KeyType key;
      ValueType value;
    };

    MemberIterator() = default;
    MemberIterator(yaml_document_t* doc, yaml_node_pair_t* ptr) : doc_(doc), ptr_(ptr) {}
    MemberIterator(const MemberIterator& another) : ptr_(another.ptr_), doc_(another.doc_) {}

    using difference_type = int;
    using value_type = MemberWrapper;
    using reference_type = MemberWrapper&;
    using pointer_type = MemberWrapper*;
    using iterator_category = std::forward_iterator_tag;

    MemberIterator& operator ++ () { ptr_++; return *this; }
    MemberIterator operator ++ (int) { auto it = *this; ptr_++; return it; }
    bool operator == (const MemberIterator& other) const { return ptr_ == other.ptr_; }
    bool operator != (const MemberIterator& other) const { return ptr_ != other.ptr_; }

    MemberWrapper operator * () const {
      return MemberWrapper {
        KeyType{doc_, yaml_document_get_node(doc_, ptr_->key)},
        ValueType{doc_, yaml_document_get_node(doc_, ptr_->value)}
      };
    }

  private:
    yaml_node_pair_t* ptr_;
    yaml_document_t* doc_;
  };

  template<typename ValueType>
  class ValueIterator {
  public:
    ValueIterator() = default;
    ValueIterator(yaml_document_t* doc, yaml_node_item_t* ptr) : doc_(doc), ptr_(ptr) {}
    ValueIterator(const ValueIterator& another) : ptr_(another.ptr_), doc_(another.doc_) {}

    using difference_type = int;
    using value_type = ValueType;
    using reference_type = ValueType&;
    using pointer_type = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    ValueIterator& operator ++ () { ptr_++; return *this; }
    ValueIterator operator ++ (int) { auto it = *this; ptr_++; return it; }
    bool operator == (const ValueIterator& other) const { return ptr_ == other.ptr_; }
    bool operator != (const ValueIterator& other) const { return ptr_ != other.ptr_; }

    ValueType operator * () const {
      return ValueType{doc_, yaml_document_get_node(doc_, *ptr_)};
    }

  private:
    yaml_node_item_t* ptr_;
    yaml_document_t* doc_;
  };

  class YamlView {
  public:
    using ConstValueIterator = ValueIterator<YamlView>;
    using ConstMemberIterator = MemberIterator<YamlView>;

    YamlView (yaml_document_t* doc, yaml_node_t* node) : doc_(doc), node_(node) {}
    YamlView (yaml_document_t* doc) : doc_(doc) { node_ = yaml_document_get_root_node(doc); }
    YamlView (const YamlView& another) : node_(another.node_), doc_(another.doc_) {}

    bool is_null() const { return node_->type == yaml_node_type_t::YAML_NO_NODE; }
    bool is_int() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_string() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_double() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_object() const noexcept { return node_->type == yaml_node_type_t::YAML_MAPPING_NODE; }
    bool is_list() const noexcept { return node_->type == yaml_node_type_t::YAML_SEQUENCE_NODE; }
    bool is_bool() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }

    char* scalar_data() const noexcept { return (char*)node_->data.scalar.value; }
    int get_int() const noexcept { return std::atoi(scalar_data()); }
    std::string get_string() const noexcept { return std::string{scalar_data()}; }
    std::string_view get_string_view() const noexcept { return std::string_view{scalar_data()}; }
    const char* get_cstr() const noexcept { return scalar_data(); }
    double get_double() const noexcept { return std::stod(scalar_data()); }
    bool get_bool() const noexcept { return strcmp("true", scalar_data()) == 0; }

    ConstValueIterator begin_list() const { return ConstValueIterator(doc_, node_->data.sequence.items.start); }
    ConstValueIterator end_list() const { return ConstValueIterator(doc_, node_->data.sequence.items.top); }
    auto get_list() const { return ConstListRange<YamlView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(doc_, node_->data.mapping.pairs.start); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(doc_, node_->data.mapping.pairs.top); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return strcmp(key, item.key.get_cstr()) == 0;
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
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_H */
