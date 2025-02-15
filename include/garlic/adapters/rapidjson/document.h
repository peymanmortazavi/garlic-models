#ifndef GARLIC_RAPIDJSON_DOCUMENT_H
#define GARLIC_RAPIDJSON_DOCUMENT_H

#include "../../layer.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"

#include "reader.h"
#include "writer.h"


namespace garlic::adapters::rapidjson {

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

  template<typename, typename = void>
  struct is_rapidjson_wrapper : std::false_type {};

  template<typename T>
  struct is_rapidjson_wrapper<T, std::void_t<typename std::decay_t<T>::RapidJsonWrapperTraits>> : std::true_type {};

  template<bool View>
  struct WrapperTraits {
    static constexpr bool view = View;
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
    using RapidJsonWrapperTraits = WrapperTraits<true>;

    JsonView (const ProviderValueType& value) : value_(&value) {}
    JsonView (const JsonView& other) : value_(other.value_) {}
    JsonView (JsonView&& other) : value_(other.value_) {};

    bool is_null() const noexcept { return value_->IsNull(); }
    bool is_int() const noexcept { return value_->IsInt(); }
    bool is_string() const noexcept { return value_->IsString(); }
    bool is_double() const noexcept { return value_->IsDouble(); }
    bool is_object() const noexcept { return value_->IsObject(); }
    bool is_list() const noexcept { return value_->IsArray(); }
    bool is_bool() const noexcept { return value_->IsBool(); }

    int get_int() const noexcept { return value_->GetInt(); }
    std::string get_string() const noexcept {
      return std::string(value_->GetString(), value_->GetStringLength());
    }
    std::string_view get_string_view() const noexcept {
      return std::string_view(value_->GetString(), value_->GetStringLength());
    }
    const char* get_cstr() const noexcept { return value_->GetString(); }
    double get_double() const noexcept { return value_->GetDouble(); }
    bool get_bool() const noexcept { return value_->GetBool(); }

    size_t string_length() const noexcept { return value_->GetStringLength(); }

    JsonView& operator = (const JsonView& another) {
      value_ = another.value_;
      return *this;
    };

    ConstValueIterator begin_list() const {
      return ConstValueIterator({value_->Begin()});
    }
    ConstValueIterator end_list() const {
      return ConstValueIterator({value_->End()});
    }
    auto get_list() const { return ConstListRange<JsonView>{*this}; }

    ConstMemberIterator begin_member() const {
      return ConstMemberIterator({value_->MemberBegin()});
    }
    ConstMemberIterator end_member() const {
      return ConstMemberIterator({value_->MemberEnd()});
    }
    ConstMemberIterator find_member(text key) const {
      return std::find_if(this->begin_member(), this->end_member(),
          [&key](const auto& item) {
            return key.compare(item.key.get_cstr()) == 0;
          });
    }
    ConstMemberIterator find_member(const JsonView& value) const {
      return ConstMemberIterator({value_->FindMember(value.get_inner_value())});
    }
    auto get_object() const { return ConstMemberRange<JsonView>{*this}; }

    const ProviderValueType& get_inner_value() const { return *value_; }
    JsonView get_view() const { return JsonView{*value_}; }

    bool operator == (const JsonView& view) const {
      return view.value_ == value_;
    }
  
  private:
    const ProviderValueType* value_;
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
      using RapidJsonWrapperTraits = WrapperTraits<false>;

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

      void set_string(text value) {
        value_.SetString(value.data(), value.size(), allocator_);
      }
      void set_string(string_ref value) {
        value_.SetString(::rapidjson::StringRef(value.data(), value.size()));
      }
      void set_int(int value) { value_.SetInt(value); }
      void set_double(double value) { value_.SetDouble(value); }
      void set_bool(bool value) { value_.SetBool(value); }
      void set_null() { value_.SetNull(); }
      void set_list() { value_.SetArray(); }
      void set_object() { value_.SetObject(); }

      JsonWrapper& operator = (double value) { this->set_double(value); return *this; }
      JsonWrapper& operator = (int value) { this->set_int(value); return *this; }
      JsonWrapper& operator = (text value) { this->set_string(value); return *this; }
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
        this->push_back(text(value));
      }
      void push_back(text value) {
        push_back(ProviderValueType().SetString(value.data(), value.size(), allocator_));
      }
      void push_back(string_ref value) {
        push_back(ProviderValueType().SetString(::rapidjson::StringRef(value.data(), value.size())));
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
      MemberIterator find_member(text key) {
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

      void add_member(ProviderValueType&& key, ProviderValueType& value) {
        value_.AddMember(key, value, allocator_);
      }

      void add_member(ProviderValueType& key, ProviderValueType&& value) {
        value_.AddMember(key, value, allocator_);
      }

      void add_member(ProviderValueType& key, ProviderValueType& value) {
        value_.AddMember(key, value, allocator_);
      }

      template<typename Key>
      void add_member(Key key, ProviderValueType&& value) {
        value_.AddMember(this->build_key(key), value, allocator_);
      }

      template<typename Key>
      void add_member(Key key, ProviderValueType& value) {
        value_.AddMember(this->build_key(key), value, allocator_);
      }

      void add_member(JsonView key, JsonView value) {
        this->add_member(
            ProviderValueType(key.get_inner_value(), allocator_),
            ProviderValueType(value.get_inner_value(), allocator_)
            );
      }

      void add_member(JsonView key) {
        this->add_member(
            ProviderValueType(key.get_inner_value(), allocator_),
            ProviderValueType()
            );
      }

      template<typename Key>
      void add_member(Key key, ValueType& value) {
        this->add_member(this->build_key(key), value.get_inner_value());
      }

      void add_member(ValueType& key, ValueType& value) {
        this->add_member(key.get_inner_value(), value.get_inner_value());
      }

      template<typename Key, typename Callable>
      void add_member_builder(Key key, Callable&& cb) {
        ProviderValueType value;
        cb(ReferenceType(value, allocator_));
        this->add_member(this->build_key(key), value);
      }

      template<typename Key>
      void add_member(Key key) {
        this->add_member(this->build_key(key), ProviderValueType());
      }

      template<typename Key>
      void add_member(Key key, const JsonWrapper& value) {
        this->add_member(this->build_key(key), value.get_inner_value());
      }

      template<typename Key>
      void add_member(Key key, const char* value) {
        this->add_member(this->build_key(key), ProviderValueType().SetString(value, allocator_));
      }

      template<typename Key>
      void add_member(Key key, text value) {
        this->add_member(
            this->build_key(key),
            ProviderValueType().SetString(value.data(), value.size(), allocator_));
      }

      template<typename Key>
      void add_member(Key key, string_ref value) {
        this->add_member(
            this->build_key(key),
            ProviderValueType().SetString(::rapidjson::StringRef(value.data(), value.size())));
      }

      template<typename Key>
      void add_member(Key key, double value) {
        add_member(this->build_key(key), ProviderValueType(value));
      }

      template<typename Key>
      void add_member(Key key, int value) {
        add_member(this->build_key(key), ProviderValueType(value));
      }

      template<typename Key>
      void add_member(Key key, bool value) {
        add_member(this->build_key(key), ProviderValueType(value));
      }

      void remove_member(text key) { value_.RemoveMember(key.data()); }
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

      template<typename InputStream>
      ::rapidjson::ParseResult parse(InputStream& stream) {
        auto handler = make_handler(*this);
        ::rapidjson::Reader reader;
        return reader.Parse(stream, handler);
      }

    private:
      T value_;
      AllocatorType& allocator_;

      inline ProviderValueType build_key(text key) const noexcept {
        auto value = ProviderValueType();
        value.SetString(key.data(), key.size(), allocator_);
        return value;
      }

      inline ProviderValueType build_key(string_ref key) const noexcept {
        auto value = ProviderValueType();
        value.SetString(::rapidjson::StringRef(key.data(), key.size()));
        return value;
      }
    };

    template<GARLIC_VIEW Layer>
    static inline std::enable_if_t<is_rapidjson_wrapper<Layer>::value, ::rapidjson::StringBuffer>
    dump(Layer&& source, bool pretty = false) {
      ::rapidjson::StringBuffer buffer;
      if (pretty) {
        auto writer = ::rapidjson::PrettyWriter<::rapidjson::StringBuffer>(buffer);
        source.get_inner_value().Accept(writer);
      } else {
        auto writer = ::rapidjson::Writer<::rapidjson::StringBuffer>(buffer);
        source.get_inner_value().Accept(writer);
      }
      return buffer;
    }

    template<GARLIC_VIEW Layer>
    static inline std::enable_if_t<!is_rapidjson_wrapper<Layer>::value, ::rapidjson::StringBuffer>
    dump(Layer&& source, bool pretty = false) {
      ::rapidjson::StringBuffer buffer;
      if (pretty) {
        auto writer = ::rapidjson::PrettyWriter<::rapidjson::StringBuffer>(buffer);
        dump(writer, source);
      } else {
        auto writer = ::rapidjson::Writer<::rapidjson::StringBuffer>(buffer);
        dump(writer, source);
      }
      return buffer;
    }

    template<GARLIC_VIEW Layer>
    static inline std::enable_if_t<is_rapidjson_wrapper<Layer>::value>
    dump(FILE * file, Layer&& source, char* write_buffer, size_t length, bool pretty = false) {
      ::rapidjson::FileWriteStream os(file, write_buffer, length);
      if (pretty) {
        auto writer = ::rapidjson::PrettyWriter<::rapidjson::FileWriteStream>(os);
        source.get_inner_value().Accept(writer);
      } else {
        auto writer = ::rapidjson::Writer<::rapidjson::FileWriteStream>(os);
        source.get_inner_value().Accept(writer);
      }
    }

    template<GARLIC_VIEW Layer>
    static inline std::enable_if_t<!is_rapidjson_wrapper<Layer>::value>
    dump(FILE * file, Layer&& source, char* write_buffer, size_t length, bool pretty = false) {
      ::rapidjson::FileWriteStream os(file, write_buffer, length);
      if (pretty) {
        auto writer = ::rapidjson::PrettyWriter<::rapidjson::FileWriteStream>(os);
        write(writer, source);
      } else {
        auto writer = ::rapidjson::Writer<::rapidjson::FileWriteStream>(os);
        write(writer, source);
      }
    }


  }

  using JsonRef = internal::JsonWrapper<::rapidjson::Value&>;
  using JsonValue = internal::JsonWrapper<::rapidjson::Value>;
  using JsonDocument = internal::JsonWrapper<::rapidjson::Document>;

  //! Load a JsonDocument from a string.
  //! \param data the string containing valid JSON.
  static inline JsonDocument load(const char* data) {
    JsonDocument doc;
    doc.get_inner_value().Parse(data);
    return doc;
  }

  //! Load a JsonDocument from a file.
  //! \param file an open and readable file.
  //! \param read_buffer the read buffer.
  //! \param length the length of the read buffer.
  static inline JsonDocument load(FILE * file, char* read_buffer, size_t length) {
    ::rapidjson::FileReadStream input_stream(file, read_buffer, length);
    JsonDocument doc;
    doc.get_inner_value().ParseStream(input_stream);
    return doc;
  }

  //! Load a JsonDocument from a file.
  //! \param file an open and readable file.
  //! \tparam BufferSize the internal buffersize to use.
  template<int BufferSize = 65536>
  static inline JsonDocument load(FILE * file) {
    char read_buffer[BufferSize];
    return load(file, read_buffer, sizeof(read_buffer));
  }

  //! Write JSON string representing the given layer to the given file.
  //! \tparam Layer any type conforming to garlic::ViewLayer concept.
  //! \param file the file to write to. It must be open and writable.
  //! \param source the readable layer to dump its content.
  //! \param buffer custom string buffer to use.
  //! \param length size of the custom string buffer.
  //! \param pretty if enabled, would output more readable JSON string.
  template<GARLIC_VIEW Layer>
  static inline void dump(FILE* file, Layer&& source, char* buffer, size_t length, bool pretty = false) {
    internal::dump(file, source, buffer, length, pretty);
  }

  //! Write JSON string representing the given layer to the given file.
  //! \tparam Layer any type conforming to garlic::ViewLayer concept.
  //! \tparam BufferSize optional buffer size.
  //! \param file the file to write to. It must be open and writable.
  //! \param source the readable layer to dump its content.
  //! \param pretty if enabled, would output more readable JSON string.
  template<GARLIC_VIEW Layer, unsigned BufferSize = 65536>
  static inline void dump(FILE* file, Layer&& source, bool pretty = false) {
    char buffer[BufferSize];
    internal::dump(file, source, buffer, sizeof(buffer), pretty);
  }

  //! Get a JSON string representing the given layer.
  //! \tparam Layer any type conforming to garlic::ViewLayer concept.
  template<GARLIC_VIEW Layer>
  static inline ::rapidjson::StringBuffer
  dump(Layer&& layer, bool pretty = false) {
    return internal::dump(layer, pretty);
  }

}

#endif
