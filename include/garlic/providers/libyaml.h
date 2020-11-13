#ifndef GARLIC_LIBYAML_H
#define GARLIC_LIBYAML_H

#include "yaml.h"
#include <cstdlib>
#include <cstring>
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

    using difference_type = ptrdiff_t;
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

    bool is_null() const { return node_->type == yaml_node_type_t::YAML_NO_NODE; }
    bool is_int() const noexcept {
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        (std::atoi(scalar_data()) != 0 || strcmp(scalar_data(), "0"))
      );
    }
    bool is_string() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_double() const noexcept {
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        check_double(scalar_data(), [](auto){})
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
    int get_int() const noexcept { return std::atoi(scalar_data()); }
    std::string get_string() const noexcept { return std::string{scalar_data()}; }
    std::string_view get_string_view() const noexcept { return std::string_view{scalar_data()}; }
    const char* get_cstr() const noexcept { return scalar_data(); }
    double get_double() const noexcept {
      double result;
      check_double(scalar_data(), [&result](auto r){result = r;});
      return result;
    }
    bool get_bool() const noexcept {
      bool result = false;
      check_bool(scalar_data(), [&result](auto r) { result = r; });
      return result;
    }

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
    bool check_double(const char* input, const Callable& cb) const {
      char* end;
      double result = strtod(input, &end);
      cb(result);
      return !(end == input || *end != '\0');
    }

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

  class Yaml {
  public:
    static YamlDocument load(const unsigned char * data, size_t lenght) {
      YamlDocument doc;
      yaml_parser_t parser;
      yaml_parser_initialize(&parser);
      yaml_parser_set_input_string(&parser, data, lenght);
      yaml_parser_load(&parser, doc.get_inner_document());
      yaml_parser_delete(&parser);
      return doc;
    }

    static YamlDocument load(const unsigned char * data) {
      return Yaml::load(data, strlen((char*)data));
    }

    static YamlDocument load(FILE * file) {
      YamlDocument doc;
      yaml_parser_t parser;
      yaml_parser_initialize(&parser);
      yaml_parser_set_input_file(&parser, file);
      yaml_parser_load(&parser, doc.get_inner_document());
      yaml_parser_delete(&parser);
      return doc;
    }

  };

}

#endif /* end of include guard: GARLIC_LIBYAML_H */
