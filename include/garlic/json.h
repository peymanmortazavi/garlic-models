#ifndef JSON_H
#define JSON_H

#include <iterator>
#include <string>
#include <memory>

#include "rapidjson/document.h"


namespace garlic {

  template<typename ValueType, typename Iterator>
  class IteratorWrapper {
  public:
    using difference_type = int;
    using value_type = ValueType;
    using iterator_category = std::forward_iterator_tag;

    explicit IteratorWrapper() {}
    explicit IteratorWrapper(Iterator&& iterator) : iterator_(std::move(iterator)) {}

    IteratorWrapper& operator ++ () { iterator_++; return *this; }
    IteratorWrapper operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const IteratorWrapper& other) const { return other.iterator_ == iterator_; }
    bool operator != (const IteratorWrapper& other) const { return !(other == *this); }

    ValueType operator * () const { return ValueType{*iterator_}; }

  private:
    Iterator iterator_;
  };

  class rapidjson_readonly_layer
  {
  public:
    using ValueType = rapidjson::Value;
    using ConstValueIterator = IteratorWrapper<rapidjson_readonly_layer, typename rapidjson::Value::ConstValueIterator>;

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
  
  private:
    const ValueType& value_;
  };

  //class rapidjson_wrapper {
  //public:
  //  using AllocatorType = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

  //  explicit rapidjson_wrapper(rapidjson::Value&& doc, AllocatorType& allocator) : document_(std::move(doc)), allocator_(allocator) {};

  //  bool is_null() const noexcept { return document_.IsNull(); }
  //  bool is_int() const noexcept { return document_.IsInt(); }
  //  bool is_string() const noexcept { return document_.IsString(); }
  //  bool is_double() const noexcept { return document_.IsDouble(); }
  //  bool is_object() const noexcept { return document_.IsObject(); }
  //  bool is_list() const noexcept { return document_.IsArray(); }
  //  bool is_bool() const noexcept { return document_.IsBool(); }

  //  int get_int() const { return document_.GetInt(); }
  //  const char* get_cstr() const { return document_.GetString(); }
  //  std::string get_string() const { return document_.GetString(); }
  //  std::string_view get_string_view() const { return document_.GetString(); }
  //  double get_double() const { return document_.GetDouble(); }
  //  bool get_bool() const { return document_.GetBool(); }

  //  template<typename T, typename IT>
  //  class list_iterator_base : public std::iterator<std::forward_iterator_tag, rapidjson_wrapper> {
  //  private:
  //    IT iterator_;
  //  public:
  //    explicit list_iterator_base(IT&& iterator) : iterator_(std::move(iterator)) {}
  //    list_iterator_base& operator ++ () { iterator_++; return *this; }
  //    list_iterator_base& operator ++ (int) { auto old_it = *this; ++(*this); return old_it; }
  //    bool operator == (const list_iterator_base& other) const { return other.iterator_ == iterator_; }
  //    bool operator != (const list_iterator_base& other) const { return !(other == *this); }
  //    T& operator * () const { return **iterator_; }
  //  };

  //  using list_iterator = list_iterator_base<rapidjson_wrapper, typename rapidjson::Value::ValueIterator>;
  //  using const_list_iterator = list_iterator_base<const rapidjson_wrapper, typename rapidjson::Value::ConstValueIterator>;

  //  struct object_element {
  //    const std::string& first;
  //    rapidjson_wrapper& second;
  //  };

  //  template<typename T, typename IT>
  //  class object_iterator_base : public std::iterator<std::forward_iterator_tag, rapidjson_wrapper> {
  //  private:
  //    IT iterator_;
  //  public:
  //    explicit object_iterator_base(IT&& iterator) : iterator_(std::move(iterator)) {}
  //    object_iterator_base& operator ++ () { iterator_++; return *this; }
  //    object_iterator_base& operator ++ (int) { auto old_it = *this; ++(*this); old_it; }
  //    bool operator == (const object_iterator_base& other) const { return other.iterator_ == iterator_; }
  //    bool operator != (const object_iterator_base& other) const { return !(*this == other); }
  //    T operator * () const { return object_element{iterator_->first, *iterator_->second}; }
  //  };

  //  using object_iterator = object_iterator_base<
  //    object_element, typename rapidjson::Value::MemberIterator
  //  >;
  //  using const_object_iterator = object_iterator_base<
  //    const object_element, typename rapidjson::Value::ConstMemberIterator
  //  >;

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
