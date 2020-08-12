#ifndef GARLIC_YAML_CPP_H
#define GARLIC_YAML_CPP_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

#include "../layer.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/yaml.h"

namespace garlic {

  template<typename ValueType, typename Iterator, typename KeyType = ValueType>
  class MemberIteratorWrapper {
  public:
    struct MemberWrapper {
      KeyType key;
      ValueType value;
    };
    
    using difference_type = int;
    using value_type = MemberWrapper;
    using reference = MemberWrapper&;
    using pointer = MemberWrapper*;
    using iterator_category = std::forward_iterator_tag;

    explicit MemberIteratorWrapper() {}
    explicit MemberIteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}
    explicit MemberIteratorWrapper(const Iterator& iterator) : iterator_(iterator) {}

    MemberIteratorWrapper& operator ++ () { iterator_++; return *this; }
    MemberIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const MemberIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const MemberIteratorWrapper& other) const { return !(other == *this); }

    MemberWrapper operator * () const { return MemberWrapper{KeyType{this->iterator_->first}, ValueType{this->iterator_->second}}; }

  private:
    Iterator iterator_;
  };

  class YamlView {
  public:
    using ValueType = YAML::Node;
    using ConstValueIterator = ValueIteratorWrapper<YamlView, typename ValueType::const_iterator>;
    using ConstMemberIterator = MemberIteratorWrapper<YamlView, typename ValueType::const_iterator>;

    YamlView (const ValueType& node) : node_(node) {}
    YamlView (const YamlView& another) : node_(another.node_) {}

    bool is_null() const noexcept { return node_.IsNull(); }
    bool is_int() const noexcept { return this->is<int>(); }
    bool is_string() const noexcept { return node_.IsScalar(); }
    bool is_double() const noexcept { return this->is<double>(); }
    bool is_object() const noexcept { return node_.IsMap(); }
    bool is_list() const noexcept { return node_.IsSequence(); }
    bool is_bool() const noexcept { return this->is<bool>(); }

    template<typename T>
    bool is() const noexcept {
      T placeholder;
      return YAML::convert<T>::decode(node_, placeholder);
    }

    int get_int() const noexcept { return node_.as<int>(); }
    std::string get_string() const noexcept { return node_.as<std::string>(); }
    std::string_view get_string_view() const noexcept { return std::string_view{node_.Scalar().c_str()}; }
    const char* get_cstr() const noexcept { return node_.Scalar().c_str(); }
    double get_double() const noexcept { return node_.as<double>(); }
    bool get_bool() const noexcept { return node_.as<bool>(); }

    ConstValueIterator begin_list() const { return ConstValueIterator(node_.begin()); }
    ConstValueIterator end_list() const { return ConstValueIterator(node_.end()); }
    auto get_list() const { return ConstListRange<YamlView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(node_.begin()); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(node_.end()); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
          return strcmp(key, item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(const YamlView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlView>{*this}; }

    const ValueType& get_inner_value() const { return node_; }
    YamlView get_view() const { return YamlView{node_}; }

  private:
    const ValueType& node_;
  };

  /**
   * Due to lack of support for some critical pieces related to the sequences, this YamlRef
   * does not actually conform to a GarlicLayer and it cannot be used as a GarlicLayer.
   */
  class YamlRef : public YamlView {
  public:
    using MemberIterator = MemberIteratorWrapper<YamlRef, typename ValueType::iterator>;
    using YamlView::begin_list;
    using YamlView::end_list;
    using YamlView::begin_member;
    using YamlView::end_member;
    using YamlView::find_member;
    using YamlView::get_list;
    using YamlView::get_object;

    YamlRef(ValueType& node) : node_(node), YamlView{node} {}
    YamlRef(const YamlRef& another) : YamlView{another.node_}, node_(another.node_) {}

    void set_string(const char* value) { node_ = value; }
    void set_string(const std::string& value) { node_ = value; }
    void set_string(const std::string_view value) { node_ = value.data(); }
    void set_int(int value) { node_ = value; }
    void set_double(double value) { node_ = value; }
    void set_bool(bool value) { node_ = value; }
    void set_null() { node_.reset(); }
    void set_list() { if(!is_list()) node_.reset(YAML::Node{YAML::NodeType::Sequence}); }
    void set_object() { if(!is_object()) node_.reset(YAML::Node{YAML::NodeType::Map}); }

    YamlRef& operator = (double value) { this->set_double(value); return *this; }
    YamlRef& operator = (int value) { this->set_int(value); return *this; }
    YamlRef& operator = (const char* value) { this->set_string(value); return *this; }
    YamlRef& operator = (const std::string& value) { this->set_string(value); return *this; }
    YamlRef& operator = (std::string_view value) { this->set_string(value); return *this; }
    YamlRef& operator = (bool value) { this->set_bool(value); return *this; }

    MemberIterator begin_member() { return MemberIterator(node_.begin()); }
    MemberIterator end_member() { return MemberIterator(node_.end()); }
    auto get_object() { return MemberRange<YamlRef>{*this}; }

    // list functions.
    void clear() { node_.reset(YAML::Node{YAML::NodeType::Sequence}); }
    void push_back() { node_.push_back(YAML::Node()); }
    void push_back(const YamlView& value) { node_.push_back(value.get_inner_value()); }
    void push_back(const std::string& value) { node_.push_back(value); }
    void push_back(const std::string_view value) { node_.push_back(value.data()); }
    void push_back(const char* value) { node_.push_back(value); }
    void push_back(int value) { node_.push_back(value); }
    void push_back(double value) { node_.push_back(value); }
    void push_back(bool value) { node_.push_back(value); }

    // member functions.
    MemberIterator find_member(const char* key) {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return strcmp(item.key.get_cstr(), key) == 0;
      });
    }
    MemberIterator find_member(const YamlView& value) { return this->find_member(value.get_cstr()); }
    void add_member(const YamlView& key, const YamlView& value) {
      node_.force_insert(key.get_inner_value(), value.get_inner_value());
    }
    void add_member(const YamlView& key) { node_.force_insert(key.get_inner_value(), YAML::Node()); }
    void add_member(YAML::Node&& key, YAML::Node&& value) {
      node_.force_insert(std::move(key), std::move(value));
    }
    void add_member(const char* key) { this->add_member(YAML::Node(key)); }
    void add_member(const char* key, YAML::Node&& value) { this->add_member(key, std::move(value)); }
    void add_member(const char* key, const char* value) { this->add_member(key, YAML::Node(value)); }
    void add_member(const char* key, const std::string& value) { this->add_member(key, YAML::Node(value)); }
    void add_member(const char* key, const std::string_view value) { this->add_member(key, YAML::Node(value.data())); }
    void add_member(const char* key, double value) { this->add_member(key, YAML::Node(value)); }
    void add_member(const char* key, int value) { this->add_member(key, YAML::Node(value)); }
    void add_member(const char* key, bool value) { this->add_member(key, YAML::Node(value)); }

    void remove_member(const char* key) { node_.remove(key); }
    void remove_member(const YamlView& key) { node_.remove(key.get_inner_value()); }
    void erase_member(MemberIterator position) {
      auto pair = *position;
      if (pair.key.is_string()) this->remove_member(pair.key.get_cstr());
    }

  private:
    ValueType& node_;
  };

  class YamlDocument {
  public:
    using ValueType = YAML::Node;

    YamlDocument() = default; 
    YamlDocument(YAML::Node&& node) : node_(std::move(node)) {}
    YamlDocument(const YAML::Node& node) : node_(node) {}

    YamlRef get_reference() { return YamlRef{node_}; }
    YamlView get_view() const { return YamlView{node_}; }
    YAML::Node& get_inner_node() { return node_; }
    const YAML::Node& get_inner_node() const { return node_; }

  private:
    ValueType node_;
  };

}

#endif /* end of include guard: GARLIC_YAML_CPP_H */
