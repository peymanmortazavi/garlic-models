#ifndef GARLIC_YAML_CPP_H
#define GARLIC_YAML_CPP_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <istream>
#include <iterator>
#include <string>

#include "../layer.h"
#include "../utility.h"
#include "yaml-cpp/node/node.h"
#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/node/type.h"
#include "yaml-cpp/yaml.h"

namespace garlic::providers::yamlcpp {

  class YamlNode {

    template<typename Iterator>
    struct MemberIteratorWrapper {
      using output_type = MemberPair<YamlNode>;
      using iterator_type = Iterator;

      iterator_type iterator;

      inline output_type wrap() const {
        return output_type {
          YamlNode{this->iterator->first},
          YamlNode{this->iterator->second}
        };
      }
    };

  public:
    using ValueType = YAML::Node;
    using ConstValueIterator = BasicLayerForwardIterator<YamlNode, typename ValueType::const_iterator>;
    using ConstMemberIterator = LayerForwardIterator<MemberIteratorWrapper<typename ValueType::const_iterator>>;
    using MemberIterator = LayerForwardIterator<MemberIteratorWrapper<typename ValueType::iterator>>;

    YamlNode () = default;
    YamlNode (const ValueType& node) : node_(node) {}

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

    void set_string(const char* value) { node_ = value; }
    void set_string(const std::string& value) { node_ = value; }
    void set_string(const std::string_view value) { node_ = value.data(); }
    void set_int(int value) { node_ = value; }
    void set_double(double value) { node_ = value; }
    void set_bool(bool value) { node_ = value; }
    void set_null() { node_.reset(); }
    void set_list() { if(!is_list()) node_.reset(YAML::Node{YAML::NodeType::Sequence}); }
    void set_object() { if(!is_object()) node_.reset(YAML::Node{YAML::NodeType::Map}); }

    YamlNode& operator = (double value) { this->set_double(value); return *this; }
    YamlNode& operator = (int value) { this->set_int(value); return *this; }
    YamlNode& operator = (const char* value) { this->set_string(value); return *this; }
    YamlNode& operator = (const std::string& value) { this->set_string(value); return *this; }
    YamlNode& operator = (std::string_view value) { this->set_string(value); return *this; }
    YamlNode& operator = (bool value) { this->set_bool(value); return *this; }

    bool operator == (const YamlNode& node) const {
      return node.node_ == node_;
    }

    ConstValueIterator begin_list() const { return ConstValueIterator({node_.begin()}); }
    ConstValueIterator end_list() const { return ConstValueIterator({node_.end()}); }
    auto get_list() const { return ConstListRange<YamlNode>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator({node_.begin()}); }
    ConstMemberIterator end_member() const { return ConstMemberIterator({node_.end()}); }
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
    ConstMemberIterator find_member(const YamlNode& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlNode>{*this}; }

    MemberIterator begin_member() { return MemberIterator({node_.begin()}); }
    MemberIterator end_member() { return MemberIterator({node_.end()}); }
    auto get_object() { return MemberRange<YamlNode>{*this}; }

    // list functions.
    void clear() { node_.reset(YAML::Node{YAML::NodeType::Sequence}); }
    void push_back() { node_.push_back(YAML::Node()); }
    void push_back(const YamlNode& value) { node_.push_back(value.get_inner_value()); }
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
    MemberIterator find_member(const YamlNode& value) { return this->find_member(value.get_cstr()); }
    void add_member(const YamlNode& key, const YamlNode& value) {
      node_.force_insert(key.get_inner_value(), value.get_inner_value());
    }
    void add_member(const YamlNode& key) { node_.force_insert(key.get_inner_value(), YAML::Node()); }
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
    void remove_member(const YamlNode& key) { node_.remove(key.get_inner_value()); }
    void erase_member(MemberIterator position) {
      auto pair = *position;
      if (pair.key.is_string()) this->remove_member(pair.key.get_cstr());
    }

    const ValueType& get_inner_value() const { return node_; }
    YamlNode get_view() const { return YamlNode{node_}; }

  private:
    ValueType node_;
  };


  class Yaml {
  public:
    static YamlNode load(const char * data) {
      return YamlNode{YAML::Load(data)};
    }

    static YamlNode load(FILE * file) {
      auto buffer = FileStreamBuffer(file);
      std::istream input_stream(&buffer);
      return YamlNode{YAML::Load(input_stream)};
    }
  };

}

#endif /* end of include guard: GARLIC_YAML_CPP_H */
