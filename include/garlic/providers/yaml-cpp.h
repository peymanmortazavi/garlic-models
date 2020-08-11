#ifndef GARLIC_YAML_CPP_H
#define GARLIC_YAML_CPP_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

#include "../layer.h"
#include "yaml-cpp/node/node.h"
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
    bool is_string() const noexcept { return node_.IsScalar() && !node_.IsNull(); }
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
          return strcmp(key, item.key.get_cstr());
      });
    }
    ConstMemberIterator find_member(const YamlView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlView>{*this}; }

    const ValueType& get_inner_value() const { return node_; }
    YamlView get_view() const { return YamlView{node_}; }

  private:
    const ValueType& node_;
  };

}

#endif /* end of include guard: GARLIC_YAML_CPP_H */
