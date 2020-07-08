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

  class rapidjson_readonly_layer
  {
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

  //class rapidjson_wrapper {
  //public:
  //  using AllocatorType = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

    //  list_iterator begin_element() { return list_iterator{document_.Begin()}; }
  //  list_iterator end_element() { return list_iterator{document_.End()}; }
  //  const_list_iterator cbegin_element() const { return const_list_iterator{document_.Begin()}; }
  //  const_list_iterator cend_element() const { return const_list_iterator{document_.End()}; }
  //  template<typename T> void append(const T& value) {
  //    document_.PushBack(get_json_value(value), allocator_);
  //  }
  //  void append(const char* value) { document_.PushBack(get_json_value(value), allocator_); }

  //  object_iterator begin_member() { return object_iterator{document_.MemberBegin()}; }
  //  object_iterator end_member() { return object_iterator{document_.MemberEnd()}; }
  //  const_object_iterator cbegin_member() const { return const_object_iterator{document_.MemberBegin()}; }
  //  const_object_iterator cend_member() const { return const_object_iterator{document_.MemberEnd()}; }
  //  void set(const std::string& key, rapidjson::Value&& value) { document_.Set(std::move(value)); }
  //  void set(const char* key, rapidjson::Value&& value) { document_.Set(std::move(value)); }
  //  template<typename T> void set(const char* key, const T& value) { this->set(key, this->get_json_value(value)); }
  //  template<typename T> void set(const std::string& key, const T& value) { this->set(key.c_str(), value); }
  //  const rapidjson_wrapper get(const std::string& key) const {
  //    auto& x = document_["asd"];
  //    return rapidjson_wrapper{document_["aas"], allocator_};
  //  }
  //  rapidjson_wrapper operator[](const std::string& key) { }


  //  struct list_range {
  //    rapidjson_wrapper& self;
  //    list_iterator begin() const { return self.begin_element(); }
  //    list_iterator end() const   { return self.end_element(); }
  //  };

  //  struct const_list_range {
  //    const rapidjson_wrapper& self;
  //    const_list_iterator begin() const { return self.cbegin_element(); }
  //    const_list_iterator end() const { return self.cend_element(); }
  //  };

  //  struct object_range {
  //    rapidjson_wrapper& self;
  //  };

  //  struct const_object_range {
  //    const rapidjson_wrapper& self;
  //  };

  //  list_range get_list() { return list_range{*this}; }
  //  const_list_range get_list() const { return const_list_range{*this}; }

  //private:
  //  rapidjson::Value&& get_json_value(int value) {
  //    return std::move(rapidjson::Value().SetInt(value));
  //  }
  //  rapidjson::Value&& get_json_value(const char* value) {
  //    return std::move(rapidjson::Value().SetString(value, allocator_));
  //  }
  //  rapidjson::Value&& get_json_value(const std::string& value) {
  //    return this->get_json_value(value.c_str());
  //  }
  //  rapidjson::Value&& get_json_value(const double& value) {
  //    return std::move(rapidjson::Value().SetDouble(value));
  //  }
  //  rapidjson::Value&& get_json_value(bool value) {
  //    return std::move(rapidjson::Value().SetBool(value));
  //  }

  //  AllocatorType& allocator_;
  //  rapidjson::Value document_;
  //};

}


#endif
