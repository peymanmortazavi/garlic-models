#ifndef JSON_H
#define JSON_H

#include <iostream>
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

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const { return ValueType{*this->iterator_}; }

  private:
    Iterator iterator_;
  };

  template<typename ValueType, typename Iterator, typename AllocatorType>
  class RefValueIteratorWrapper {
  public:
    using difference_type = int;
    using value_type = ValueType;
    using iterator_category = std::forward_iterator_tag;

    explicit RefValueIteratorWrapper() {}
    explicit RefValueIteratorWrapper(Iterator&& iterator, AllocatorType& allocator) : iterator_(std::move(iterator)), allocator_(&allocator) {}

    RefValueIteratorWrapper& operator ++ () { iterator_++; return *this; }
    RefValueIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const RefValueIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const RefValueIteratorWrapper& other) const { return !(other == *this); }

    Iterator& get_inner_iterator() { return iterator_; }
    const Iterator& get_inner_iterator() const { return iterator_; }

    ValueType operator * () const { return ValueType{*this->iterator_, *allocator_}; }

  private:
    Iterator iterator_;
    AllocatorType* allocator_;
  };

  template<typename ValueType, typename Iterator, typename AllocatorType, typename KeyType = ValueType>
  class RefMemberIteratorWrapper {
  public:
    struct MemberWrapper {
      KeyType key;
      ValueType value;
    };

    using difference_type = int;
    using value_type = MemberWrapper;
    using iterator_category = std::forward_iterator_tag;

    explicit RefMemberIteratorWrapper() {}
    explicit RefMemberIteratorWrapper(Iterator&& iterator, AllocatorType& allocator) : iterator_(std::move(iterator)), allocator_(&allocator) {}

    RefMemberIteratorWrapper& operator ++ () { iterator_++; return *this; }
    RefMemberIteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const RefMemberIteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const RefMemberIteratorWrapper& other) const { return !(other == *this); }

    MemberWrapper operator * () const {
      return MemberWrapper{KeyType{this->iterator_->name, *allocator_}, ValueType{this->iterator_->value, *allocator_}};
    }

  private:
    Iterator iterator_;
    AllocatorType* allocator_;
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
  struct ListRange {
    LayerType& layer;

    ValueIterator<LayerType> begin() { return layer.begin_list(); }
    ValueIterator<LayerType> end() { return layer.end_list(); }
  };

  template <typename LayerType>
  struct ConstMemberRange {
    const LayerType& layer;

    ConstMemberIterator<LayerType> begin() const { return layer.begin_member(); }
    ConstMemberIterator<LayerType> end() const { return layer.end_member(); }
  };

  template <typename LayerType>
  struct MemberRange {
    LayerType& layer;

    MemberIterator<LayerType> begin() { return layer.begin_member(); }
    MemberIterator<LayerType> end() { return layer.end_member(); }
  };

  class JsonView {
  public:
    using ValueType = rapidjson::Value;
    using ConstValueIterator = ValueIteratorWrapper<JsonView, typename rapidjson::Value::ConstValueIterator>;
    using ConstMemberIterator = MemberIteratorWrapper<JsonView, typename rapidjson::Value::ConstMemberIterator>;

    JsonView (const ValueType& value) : value_(value) {}

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
    auto get_list() const { return ConstListRange<JsonView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(value_.MemberBegin()); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(value_.MemberEnd()); }
    ConstMemberIterator find_member(const char* key) const { return ConstMemberIterator{value_.FindMember(key)}; }
    ConstMemberIterator find_member(const JsonView& value) const { return ConstMemberIterator{value_.FindMember(value.get_inner_value())}; }
    auto get_object() const { return ConstMemberRange<JsonView>{*this}; }

    const ValueType& get_inner_value() const { return value_; }
    JsonView get_view() const { return JsonView{value_}; }
  
  private:
    const ValueType& value_;
  };

  class JsonRef : public JsonView {
  public:
    using DocumentType = rapidjson::Document;
    using AllocatorType = DocumentType::AllocatorType;
    using ValueIterator = RefValueIteratorWrapper<JsonRef, typename rapidjson::Value::ValueIterator, AllocatorType>;
    using MemberIterator = RefMemberIteratorWrapper<JsonRef, typename rapidjson::Value::MemberIterator, AllocatorType>;
    using JsonView::begin_list;
    using JsonView::end_list;
    using JsonView::begin_member;
    using JsonView::end_member;
    using JsonView::find_member;
    using JsonView::get_list;
    using JsonView::get_object;

    JsonRef(
        ValueType& value,
        DocumentType::AllocatorType& allocator
    ) : JsonView(value), value_(value), allocator_(allocator) {}

    JsonRef(DocumentType& doc) : JsonView(doc), value_(doc), allocator_(doc.GetAllocator()) {}

    void set_string(const char* value) { value_.SetString(value, allocator_); }
    void set_string(const std::string& value) { value_.SetString(value.data(), value.length(), allocator_); }
    void set_string(const std::string_view& value) { value_.SetString(value.data(), value.length(), allocator_); }
    void set_int(int value) { value_.SetInt(value); }
    void set_double(double value) { value_.SetDouble(value); }
    void set_bool(bool value) { value_.SetBool(value); }
    void set_null() { value_.SetNull(); }
    void set_list() { value_.SetArray(); }
    void set_object() { value_.SetObject(); }

    JsonRef& operator = (double value) { this->set_double(value); return *this; }
    JsonRef& operator = (int value) { this->set_int(value); return *this; }
    JsonRef& operator = (const char* value) { this->set_string(value); return *this; }
    JsonRef& operator = (const std::string& value) { this->set_string(value); return *this; }
    JsonRef& operator = (const std::string_view& value) { this->set_string(value); return *this; }
    JsonRef& operator = (bool value) { this->set_bool(value); return *this; }

    ValueIterator begin_list() { return ValueIterator(value_.Begin(), allocator_); }
    ValueIterator end_list() { return ValueIterator(value_.End(), allocator_); }
    auto get_list() { return ListRange<JsonRef>{*this}; }

    MemberIterator begin_member() { return MemberIterator(value_.MemberBegin(), allocator_); }
    MemberIterator end_member() { return MemberIterator(value_.MemberEnd(), allocator_); }
    auto get_object() { return MemberRange<JsonRef>{*this}; }

    JsonRef get_reference() { return JsonRef{value_, allocator_}; }

    // list functions.
    void clear() { value_.Clear(); }
    void push_back() { value_.PushBack(ValueType().Move(), allocator_); }
    void push_back(const JsonView& value) {
      value_.PushBack(ValueType(value.get_inner_value(), allocator_), allocator_);
    }
    void push_back(const char* value) {
      value_.PushBack(ValueType().SetString(value, allocator_), allocator_);
    }
    void push_back(const std::string& value) {
      value_.PushBack(ValueType().SetString(value.data(), value.length(), allocator_), allocator_);
    }
    void push_back(const std::string_view& value) {
      value_.PushBack(ValueType().SetString(value.data(), value.length(), allocator_), allocator_);
    }
    void push_back(int value) { value_.PushBack(ValueType(value).Move(), allocator_); }
    void push_back(double value) { value_.PushBack(ValueType(value).Move(), allocator_); }
    void push_back(bool value) { value_.PushBack(ValueType(value).Move(), allocator_); }
    void pop_back() { value_.PopBack(); }
    void erase(const ConstValueIterator& position) { value_.Erase(position.get_inner_iterator()); }
    void erase(const ValueIterator& position) { value_.Erase(position.get_inner_iterator()); }
    void erase(const ValueIterator& first, const ValueIterator& last) {
      value_.Erase(first.get_inner_iterator(), last.get_inner_iterator());
    }
    void erase(const ConstValueIterator& first, const ConstValueIterator& last) {
      value_.Erase(first.get_inner_iterator(), last.get_inner_iterator());
    }

    // member functions.
    MemberIterator find_member(const char* key) { return MemberIterator{value_.FindMember(key), allocator_}; }
    MemberIterator find_member(const JsonView& value) { return MemberIterator{value_.FindMember(value.get_inner_value()), allocator_}; }
    void add_member(const JsonView& key, const JsonView& value) {
      value_.AddMember(
        ValueType(key.get_inner_value(), allocator_),
        ValueType(value.get_inner_value(), allocator_),
        allocator_
      );
    }

    void add_member(const JsonView& key) {
      value_.AddMember(
        ValueType(key.get_inner_value(), allocator_),
        ValueType().Move(),
        allocator_
      );
    }

    void add_member(const char* key) { this->add_member(ValueType().SetString(key, allocator_)); }
    void add_member(const char* key, const JsonRef& value) {
      this->add_member(ValueType().SetString(key, allocator_), value.get_inner_value());
    }
    void add_member(const char* key, const char* value) {
      this->add_member(key, JsonRef{ValueType().SetString(value, allocator_), allocator_});
    }
    void add_member(const char* key, const std::string& value) {
      this->add_member(
          key,
          JsonRef{ValueType().SetString(value.data(), value.length(), allocator_), allocator_}
      );
    }
    void add_member(const char* key, const std::string_view& value) {
      this->add_member(
          key,
          JsonRef{ValueType().SetString(value.data(), value.length(), allocator_), allocator_}
      );
    }
    void add_member(const char* key, double value) {
      this->add_member(key, JsonRef{ValueType().SetDouble(value), allocator_});
    }
    void add_member(const char* key, int value) {
      this->add_member(key, JsonRef{ValueType().SetInt(value), allocator_});
    }
    void add_member(const char* key, bool value) {
      this->add_member(key, JsonRef{ValueType().SetBool(value), allocator_});
    }

    void remove_member(const char* key) { value_.RemoveMember(key); }
    void remove_member(const JsonView& key) { value_.RemoveMember(key.get_inner_value()); }

    AllocatorType& get_allocator() { return allocator_; }

  private:
    ValueType& value_;
    AllocatorType& allocator_;
  };

  class JsonDocument : public JsonRef {
  public:
    explicit JsonDocument() : JsonRef(doc_) {}
    explicit JsonDocument(DocumentType&& doc) : doc_(std::move(doc)), JsonRef(doc_) {}

    DocumentType& get_inner_doc() { return doc_; }
    const DocumentType& get_inner_doc() const { return doc_; }

  private:
    DocumentType doc_;
  };


  class JsonValue : public JsonRef {
  public:
    explicit JsonValue (JsonDocument& doc) : JsonRef(value_, doc.get_allocator()) { }
    explicit JsonValue (
        ValueType&& value,
        AllocatorType& allocator
      ) : value_(std::move(value)), JsonRef(value_, allocator) {}

  private:
    ValueType value_;
  };

}


#endif
