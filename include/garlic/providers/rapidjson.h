#ifndef GARLIC_RAPIDJSON_H
#define GARLIC_RAPIDJSON_H

#include <iterator>
#include <string>
#include <type_traits>

#include "../layer.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/writer.h"


namespace garlic::providers::rapidjson {

  template<typename Layer, typename Iterator, typename Allocator>
  struct MemberIteratorWrapper {
    using output_type = MemberPair<Layer>;
    using iterator_type = Iterator;
    using allocator_type = Allocator;

    iterator_type iterator;
    allocator_type* allocator;

    inline output_type wrap() const {
      return output_type {
        Layer{iterator->name, *allocator},
        Layer{iterator->value, *allocator}
      };
    }
  };

  template<typename Layer, typename Iterator>
  struct ConstMemberIteratorWrapper {
    using output_type = MemberPair<Layer>;
    using iterator_type = Iterator;

    iterator_type iterator;

    inline output_type wrap() const {
      return output_type {
        Layer{iterator->name},
        Layer{iterator->value}
      };
    }
  };

  template<typename Layer, typename Iterator, typename Allocator>
  struct ValueIteratorWrapper {
    using output_type = Layer;
    using iterator_type = Iterator;
    using allocator_type = Allocator;

    iterator_type iterator;
    allocator_type* allocator;

    inline output_type wrap() const {
      return output_type { *iterator, *allocator };
    }
  };

  class JsonView {
  public:
    using ProviderValueType = ::rapidjson::Value;
    using ProviderConstValueIterator = typename ProviderValueType::ConstValueIterator;
    using ProviderConstMemberIterator = typename ProviderValueType::ConstMemberIterator;
    using ConstValueIterator = BasicRandomAccessIterator<
      JsonView, typename ::rapidjson::Value::ConstValueIterator>;
    using ConstMemberIterator = RandomAccessIterator<
      ConstMemberIteratorWrapper<JsonView, typename ::rapidjson::Value::ConstMemberIterator>>;

    JsonView (const ProviderValueType& value) : value_(value) {}

    bool is_null() const noexcept { return value_.IsNull(); }
    bool is_int() const noexcept { return value_.IsInt(); }
    bool is_string() const noexcept { return value_.IsString(); }
    bool is_double() const noexcept { return value_.IsDouble(); }
    bool is_object() const noexcept { return value_.IsObject(); }
    bool is_list() const noexcept { return value_.IsArray(); }
    bool is_bool() const noexcept { return value_.IsBool(); }

    int get_int() const noexcept { return value_.GetInt(); }
    std::string get_string() const noexcept {
      return std::string(value_.GetString(), value_.GetStringLength());
    }
    std::string_view get_string_view() const noexcept {
      return std::string_view(value_.GetString(), value_.GetStringLength());
    }
    const char* get_cstr() const noexcept { return value_.GetString(); }
    double get_double() const noexcept { return value_.GetDouble(); }
    bool get_bool() const noexcept { return value_.GetBool(); }

    size_t string_length() const noexcept { return value_.GetStringLength(); }

    JsonView operator = (const JsonView& another) { return JsonView(another); }

    ConstValueIterator begin_list() const {
      return ConstValueIterator({value_.Begin()});
    }
    ConstValueIterator end_list() const {
      return ConstValueIterator({value_.End()});
    }
    auto get_list() const { return ConstListRange<JsonView>{*this}; }

    ConstMemberIterator begin_member() const {
      return ConstMemberIterator({value_.MemberBegin()});
    }
    ConstMemberIterator end_member() const {
      return ConstMemberIterator({value_.MemberEnd()});
    }
    ConstMemberIterator find_member(const char* key) const {
      return ConstMemberIterator({value_.FindMember(key)});
    }
    ConstMemberIterator find_member(std::string_view key) const {
      return std::find_if(this->begin_member(), this->end_member(),
          [&key](const auto& item) {
            return key.compare(item.key.get_cstr()) == 0;
          });
    }
    ConstMemberIterator find_member(const JsonView& value) const {
      return ConstMemberIterator({value_.FindMember(value.get_inner_value())});
    }
    auto get_object() const { return ConstMemberRange<JsonView>{*this}; }

    const ProviderValueType& get_inner_value() const { return value_; }
    JsonView get_view() const { return JsonView{value_}; }

    bool operator == (const JsonView& view) const {
      return view.value_ == value_;
    }
  
  private:
    const ProviderValueType& value_;
  };

  namespace internal {
    template<typename T>
    class JsonWrapper : public JsonView {
    public:
      using ProviderDocumentType = ::rapidjson::Document;
      using ProviderValueIterator = typename ProviderValueType::ValueIterator;
      using ProviderMemberIterator = typename ProviderValueType::MemberIterator;

      using ValueType = JsonWrapper<ProviderValueType>;
      using DocumentType = JsonWrapper<ProviderDocumentType>;
      using ReferenceType = JsonWrapper<ProviderValueType&>;
      using ViewType = JsonView;
      using AllocatorType = ProviderDocumentType::AllocatorType;
      using ValueIterator = RandomAccessIterator<
        ValueIteratorWrapper<ReferenceType, ProviderValueIterator, AllocatorType>>;
      using MemberIterator = RandomAccessIterator<
        MemberIteratorWrapper<ReferenceType, ProviderMemberIterator, AllocatorType>>;

      using JsonView::begin_list;
      using JsonView::end_list;
      using JsonView::begin_member;
      using JsonView::end_member;
      using JsonView::find_member;
      using JsonView::get_list;
      using JsonView::get_object;

      JsonWrapper(
          std::add_rvalue_reference_t<T> value,
          AllocatorType& allocator
          ) : JsonView(value), value_(std::forward<T>(value)), allocator_(allocator) {}

      template<typename Target, typename Type>
      using enable_for_reference_type =
          std::enable_if_t<std::is_reference<Target>::value, Type>;

      template<typename Target, typename Type>
      using enable_for_value_type =
          std::enable_if_t<std::is_same<Target, ProviderValueType>::value, Type>;

      template<typename Target, typename Type>
      using enable_for_document_type =
          std::enable_if_t<std::is_same<Target, ProviderDocumentType>::value, Type>;

      template<typename Target = T>
      JsonWrapper(enable_for_reference_type<Target, ProviderDocumentType&> doc)
        : JsonView(doc), value_(doc), allocator_(doc.GetAllocator()) {}

      template<typename Target = T>
      JsonWrapper(enable_for_reference_type<Target, DocumentType&> doc)
        : JsonView(doc.get_inner_value()),
          value_(doc.get_inner_value()),
          allocator_(doc.get_allocator()) {}

      template<typename Target = T>
      JsonWrapper(
          enable_for_document_type<Target, ProviderDocumentType> doc = ProviderDocumentType()
          ) : value_(std::forward<Target>(doc)),
              JsonView(value_),
              allocator_(value_.GetAllocator()) {}

      template<typename Target = T>
      JsonWrapper(enable_for_value_type<Target, AllocatorType&> allocator)
        : JsonView(value_), allocator_(allocator) {}

      template<typename Target = T>
      JsonWrapper(enable_for_value_type<Target, ProviderDocumentType&> doc)
        : JsonView(value_), allocator_(doc.GetAllocator()) {}

      template<typename Target = T>
      JsonWrapper(enable_for_value_type<Target, DocumentType&> doc)
        : JsonView(value_), allocator_(doc.get_allocator()) {}

      void set_string(const char* value) {
        value_.SetString(value, allocator_);
      }
      void set_string(const std::string& value) {
        value_.SetString(value.data(), value.length(), allocator_);
      }
      void set_string(std::string_view value) {
        value_.SetString(value.data(), value.length(), allocator_);
      }
      void set_int(int value) { value_.SetInt(value); }
      void set_double(double value) { value_.SetDouble(value); }
      void set_bool(bool value) { value_.SetBool(value); }
      void set_null() { value_.SetNull(); }
      void set_list() { value_.SetArray(); }
      void set_object() { value_.SetObject(); }

      JsonWrapper& operator = (double value) { this->set_double(value); return *this; }
      JsonWrapper& operator = (int value) { this->set_int(value); return *this; }
      JsonWrapper& operator = (const char* value) { this->set_string(value); return *this; }
      JsonWrapper& operator = (const std::string& value) { this->set_string(value); return *this; }
      JsonWrapper& operator = (std::string_view value) { this->set_string(value); return *this; }
      JsonWrapper& operator = (bool value) { this->set_bool(value); return *this; }

      bool operator == (const ViewType& view) const {
        return view.get_inner_value() == value_;
      }

      bool operator == (const DocumentType& doc) const {
        return doc.get_inner_value() == value_;
      }

      bool operator == (const ValueType& value) const {
        return value.get_inner_value() == value_;
      }

      bool operator == (const ReferenceType& ref) const {
        return ref.get_inner_value() == value_;
      }

      ValueIterator begin_list() { return ValueIterator({value_.Begin(), &allocator_}); }
      ValueIterator end_list() { return ValueIterator({value_.End(), &allocator_}); }
      auto get_list() { return ListRange<JsonWrapper>{*this}; }

      MemberIterator begin_member() { return MemberIterator({value_.MemberBegin(), &allocator_}); }
      MemberIterator end_member() { return MemberIterator({value_.MemberEnd(), &allocator_}); }
      auto get_object() { return MemberRange<JsonWrapper>{*this}; }

      // list functions.
      void clear() { value_.Clear(); }
      template<typename Callable>
      void push_back_builder(Callable&& cb) {
        ProviderValueType value;
        cb(ReferenceType{value, allocator_});
        value_.PushBack(value, allocator_);
      }
      void push_back() {
        value_.PushBack(ProviderValueType().Move(), allocator_);
      }
      void push_back(const JsonView& value) {
        value_.PushBack(ProviderValueType(value.get_inner_value(), allocator_), allocator_);
      }
      void push_back(ProviderValueType& value) {
        value_.PushBack(value, allocator_);
      }
      void push_back(ProviderValueType&& value) {
        value_.PushBack(value, allocator_);
      }
      void push_back(const char* value) {
        push_back(ProviderValueType().SetString(value, allocator_));
      }
      void push_back(const std::string& value) {
        push_back(ProviderValueType().SetString(value.data(), value.length(), allocator_));
      }
      void push_back(std::string_view value) {
        push_back(ProviderValueType().SetString(value.data(), value.length(), allocator_));
      }
      void push_back(int value) { push_back(ProviderValueType(value)); }
      void push_back(double value) { push_back(ProviderValueType(value)); }
      void push_back(bool value) { push_back(ProviderValueType(value)); }
      void pop_back() { value_.PopBack(); }
      void erase(ConstValueIterator position) { value_.Erase(position.get_inner_iterator()); }
      void erase(ValueIterator position) { value_.Erase(position.get_inner_iterator()); }
      void erase(ValueIterator first, ValueIterator last) {
        value_.Erase(first.get_inner_iterator(), last.get_inner_iterator());
      }
      void erase(const ConstValueIterator first, const ConstValueIterator last) {
        value_.Erase(first.get_inner_iterator(), last.get_inner_iterator());
      }

      // member functions.
      MemberIterator find_member(const char* key) {
        return MemberIterator({value_.FindMember(key), &allocator_});
      }
      MemberIterator find_member(std::string_view key) {
        return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
            return key.compare(item.key.get_cstr()) == 0;
            });
      }
      MemberIterator find_member(const JsonView& value) {
        return MemberIterator({value_.FindMember(value.get_inner_value()), &allocator_});
      }

      void add_member(ProviderValueType&& key, ProviderValueType&& value) {
        value_.AddMember(key, value, allocator_);
      }

      void add_member(ProviderValueType& key, ProviderValueType& value) {
        value_.AddMember(key, value, allocator_);
      }

      void add_member(const char* key, ProviderValueType&& value) {
        value_.AddMember(
            ProviderValueType().SetString(key, allocator_),
            value,
            allocator_);
      }

      void add_member(const char* key, ProviderValueType& value) {
        value_.AddMember(
            ProviderValueType().SetString(key, allocator_),
            value,
            allocator_);
      }

      void add_member(JsonView key, JsonView value) {
        add_member(
            ProviderValueType(key.get_inner_value(), allocator_),
            ProviderValueType(value.get_inner_value(), allocator_)
            );
      }

      void add_member(JsonView key) {
        add_member(
            ProviderValueType(key.get_inner_value(), allocator_),
            ProviderValueType()
            );
      }

      void add_member(const char* key, ValueType& value) {
        add_member(key, value.get_inner_value());
      }

      void add_member(ValueType& key, ValueType& value) {
        add_member(key.get_inner_value(), value.get_inner_value());
      }

      template<typename Callable>
      void add_member_builder(const char* key, Callable&& cb) {
        ProviderValueType value;
        cb(ReferenceType(value, allocator_));
        add_member(key, value);
      }
      void add_member(const char* key) {
        add_member(ProviderValueType().SetString(key, allocator_));
      }
      void add_member(const char* key, const JsonWrapper& value) {
        add_member(key, value.get_inner_value());
      }
      void add_member(const char* key, const char* value) {
        add_member(key, ProviderValueType().SetString(value, allocator_));
      }
      void add_member(const char* key, const std::string& value) {
        add_member(
            key,
            ProviderValueType().SetString(value.data(), value.length(), allocator_));
      }
      void add_member(const char* key, std::string_view value) {
        add_member(
            key,
            ProviderValueType().SetString(value.data(), value.length(), allocator_));
      }
      void add_member(const char* key, double value) {
        add_member(key, ProviderValueType(value));
      }
      void add_member(const char* key, int value) {
        add_member(key, ProviderValueType(value));
      }
      void add_member(const char* key, bool value) {
        add_member(key, ProviderValueType(value));
      }

      void remove_member(const char* key) { value_.RemoveMember(key); }
      void remove_member(const JsonView& key) { value_.RemoveMember(key.get_inner_value()); }
      void erase_member(MemberIterator position) {
        value_.EraseMember(position.get_inner_iterator());
      }
      void erase_member(ConstMemberIterator position) {
        value_.EraseMember(position.get_inner_iterator());
      }

      T& get_inner_value() { return value_; }
      const T& get_inner_value() const { return value_; }
      ReferenceType get_reference() {
        return ReferenceType(value_, allocator_);
      }
      AllocatorType& get_allocator() { return allocator_; }

    private:
      T value_;
      AllocatorType& allocator_;
    };
  }

  using JsonRef = internal::JsonWrapper<::rapidjson::Value&>;
  using JsonValue = internal::JsonWrapper<::rapidjson::Value>;
  using JsonDocument = internal::JsonWrapper<::rapidjson::Document>;

  struct Json {
    static inline JsonDocument load(const char* data) {
      JsonDocument doc;
      doc.get_inner_value().Parse(data);
      return doc;
    }

    template<int BufferSize = 65536>
    static inline JsonDocument load(FILE * file) {
      char read_buffer[BufferSize];
      ::rapidjson::FileReadStream input_stream(file, read_buffer, sizeof(read_buffer));
      JsonDocument doc;
      doc.get_inner_value().ParseStream(input_stream);
      return doc;
    }

    template<typename JsonType, int BufferSize = 65536>
    static inline void dump(FILE * file, JsonType&& source) {
      char write_buffer[BufferSize];
      ::rapidjson::FileWriteStream os(file, write_buffer, sizeof(write_buffer));
      ::rapidjson::Writer<::rapidjson::FileWriteStream> writer(os);
      source.get_inner_value().Accept(writer);
    }

    template<typename JsonType, int BufferSize = 65536>
    static inline void dump_pretty(FILE * file, JsonType&& source) {
      char write_buffer[BufferSize];
      ::rapidjson::FileWriteStream os(file, write_buffer, sizeof(write_buffer));
      ::rapidjson::PrettyWriter<::rapidjson::FileWriteStream> writer(os);
      source.get_inner_value().Accept(writer);
    }

  };

}


#endif
