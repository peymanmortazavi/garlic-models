#ifndef JSON_H
#define JSON_H

#include <iterator>
#include <string>
#include <memory>

#include "layer.h"
#include "rapidjson/document.h"


namespace garlic {

  template<typename ValueType, typename Iterator>
  class ValueIteratorWrapper {
  public:
    using difference_type = int;
    using value_type = ValueType;
    using iterator_category = std::forward_iterator_tag;

    explicit ValueIteratorWrapper() {}
    explicit ValueIteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}

    ValueIteratorWrapper& operator ++ () { iterator_++; return *this; }
    ValueIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const ValueIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const ValueIteratorWrapper& other) const { return !(other == *this); }

    ValueType operator * () const { return ValueType{*this->iterator_}; }

  private:
    Iterator iterator_;
  };

  template<typename ValueType, typename Iterator, typename KeyType = ValueType>
  class MemberIteratorWrapper {
  public:
    struct MemberWrapper {
      KeyType key;
      ValueType value;
    };

    using difference_type = int;
    using value_type = MemberWrapper;
    using iterator_category = std::forward_iterator_tag;

    explicit MemberIteratorWrapper() {}
    explicit MemberIteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}

    MemberIteratorWrapper& operator ++ () { iterator_++; return *this; }
    MemberIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const MemberIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const MemberIteratorWrapper& other) const { return !(other == *this); }

    MemberWrapper operator * () const { return MemberWrapper{KeyType{this->iterator_->name}, ValueType{this->iterator_->value}}; }

  private:
    Iterator iterator_;
  };

  template <typename LayerType>
  struct ConstListRange {
    const LayerType& layer;

    ConstValueIterator<LayerType> begin() const { return layer.begin_list(); }
    ConstValueIterator<LayerType> end() const { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ConstMemberRange {
    const LayerType& layer;

    ConstMemberIterator<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIterator<LayerType> end() const { return layer.end_member(); }
  };

  class rapidjson_readonly_layer {
  public:
    using ValueType = rapidjson::Value;
    using ConstValueIterator = ValueIteratorWrapper<rapidjson_readonly_layer, typename rapidjson::Value::ConstValueIterator>;
    using ConstMemberIterator = MemberIteratorWrapper<rapidjson_readonly_layer, typename rapidjson::Value::ConstMemberIterator>;

    rapidjson_readonly_layer (const ValueType& value) : value_(value) {}

    bool is_null() const { return value_.IsNull(); }
    bool is_int() const noexcept { return value_.IsInt(); }
    bool is_string() const noexcept { return value_.IsString(); }
    bool is_double() const noexcept { return value_.IsDouble(); }
    bool is_object() const noexcept { return value_.IsObject(); }
    bool is_list() const noexcept { return value_.IsArray(); }
    bool is_bool() const noexcept { return value_.IsBool(); }

    int get_int() const noexcept { return value_.GetInt(); }
    std::string get_string() const noexcept { return value_.GetString(); }
    std::string_view get_string_view() const noexcept { return std::string_view(value_.GetString()); }
    const char* get_cstr() const noexcept { return value_.GetString(); }
    double get_double() const noexcept { return value_.GetDouble(); }
    bool get_bool() const noexcept { return value_.GetBool(); }

    ConstValueIterator begin_list() const { return ConstValueIterator(value_.Begin()); }
    ConstValueIterator end_list() const { return ConstValueIterator(value_.End()); }
    auto get_list() const { return ConstListRange<rapidjson_readonly_layer>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(value_.MemberBegin()); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(value_.MemberEnd()); }
    auto get_object() const { return ConstMemberRange<rapidjson_readonly_layer>{*this}; }
  
  private:
    const ValueType& value_;
  };

  class rapidjson_layer : public rapidjson_readonly_layer {
  public:
    using DocumentType = rapidjson::Document;

    rapidjson_layer(
        ValueType& value,
        DocumentType::AllocatorType& allocator
    ) : rapidjson_readonly_layer(value), value_(value), allocator_(allocator) {}

    rapidjson_layer(DocumentType& doc) : rapidjson_readonly_layer(doc), value_(doc), allocator_(doc.GetAllocator()) {}

    void set_string(const char* value, size_t len) { value_.SetString(value, len, allocator_); }
    void set_string(const std::string& value) { value_.SetString(value.data(), value.length(), allocator_); }
    void set_string(const std::string_view& value) { value_.SetString(value.data(), value.length(), allocator_); }

  private:
    ValueType& value_;
    DocumentType::AllocatorType& allocator_;
  };

}


#endif
