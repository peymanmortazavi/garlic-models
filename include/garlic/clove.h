#ifndef BASIC_H
#define BASIC_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include "allocators.h"
#include "layer.h"

namespace garlic {

  struct String {
    size_t length;
    char* data;
  };

  template<typename T>
  struct Array {
    using Container = T*;

    size_t capacity;
    size_t length;
    Container data;
  };

  template<typename T>
  struct Member {
    T key;
    T value;
  };
 
  template<Allocator Allocator>
  struct GenericData {  // todo: use pointers to the array in order to reduce 4 size_t types.
    using List = Array<GenericData>;
    using Object = Array<Member<GenericData>>;
    using AllocatorType = Allocator;

    TypeFlag type = TypeFlag::Null;
    union {
      double dvalue;
      int integer;
      bool boolean;
      struct String string;
      List list;
      Object object;
    };
  };

  template<typename ValueType, typename Iterator>
  class CloveMemberIterator {
  public:
    struct MemberWrapper {
      ValueType key;
      ValueType value;
    };

    using difference_type = ptrdiff_t;
    using value_type = MemberWrapper;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    explicit CloveMemberIterator() {}
    explicit CloveMemberIterator(Iterator&& iterator) : iterator_(std::move(iterator)) {}
    explicit CloveMemberIterator(const Iterator& iterator) : iterator_(iterator) {}

    CloveMemberIterator& operator ++ () { iterator_++; return *this; }
    CloveMemberIterator operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const CloveMemberIterator& other) const { return other.iterator_ == iterator_; }
    bool operator != (const CloveMemberIterator& other) const { return !(other == *this); }

    const Iterator& get_inner_iterator() const { return iterator_; }
    Iterator& get_inner_iterator() { return iterator_; }

    MemberWrapper operator * () const {
      return MemberWrapper{
        ValueType{this->iterator_->key},
        ValueType{this->iterator_->value}
      };
    }

  private:
    Iterator iterator_;
  };


  template<typename ValueType, typename Iterator, typename AllocatorType>
  class RefCloveMemberIterator {
  public:
    struct MemberWrapper {
      ValueType key;
      ValueType value;
    };

    using difference_type = ptrdiff_t;
    using value_type = MemberWrapper;
    using reference = ValueType&;
    using pointer = ValueType*;
    using iterator_category = std::forward_iterator_tag;

    explicit RefCloveMemberIterator() {}
    explicit RefCloveMemberIterator(
        Iterator&& iterator,
        AllocatorType& allocator
    ) : allocator_(&allocator), iterator_(std::move(iterator)) {}

    explicit RefCloveMemberIterator(
        const Iterator& iterator,
        AllocatorType& allocator
    ) : allocator_(&allocator), iterator_(iterator) {}

    RefCloveMemberIterator& operator ++ () { iterator_++; return *this; }
    RefCloveMemberIterator operator ++ (int) { auto it = *this; iterator_++; return it; }
    bool operator == (const RefCloveMemberIterator& other) const { return other.iterator_ == iterator_; }
    bool operator != (const RefCloveMemberIterator& other) const { return !(other == *this); }

    const Iterator& get_inner_iterator() const { return iterator_; }
    Iterator& get_inner_iterator() { return iterator_; }

    MemberWrapper operator * () const {
      return MemberWrapper{
        ValueType{this->iterator_->key, *allocator_},
        ValueType{this->iterator_->value, *allocator_}
      };
    }

  private:
    Iterator iterator_;
    AllocatorType* allocator_;
  };


  template<Allocator Allocator>
  class GenericCloveView {
  public:
    using DataType = GenericData<Allocator>;
    using ConstValueIterator = ValueIteratorWrapper<GenericCloveView, typename DataType::List::Container>;
    using ConstMemberIterator = CloveMemberIterator<GenericCloveView, typename DataType::Object::Container>;

    GenericCloveView (const DataType& data) : data_(data) {}
    GenericCloveView (const GenericCloveView& another) = delete;

    bool is_null() const noexcept { return data_.type == TypeFlag::Null; }
    bool is_int() const noexcept { return data_.type & TypeFlag::Integer; }
    bool is_string() const noexcept { return data_.type & TypeFlag::String; }
    bool is_double() const noexcept { return data_.type & TypeFlag::Double; }
    bool is_object() const noexcept { return data_.type & TypeFlag::Object; }
    bool is_list() const noexcept { return data_.type & TypeFlag::List; }
    bool is_bool() const noexcept { return data_.type & TypeFlag::Boolean; }

    int get_int() const { return data_.integer; }
    double get_double() const { return data_.dvalue; }
    bool get_bool() const { return data_.boolean; }
    const char* get_cstr() const { return data_.string.data; }
    std::string get_string() const { return std::string{data_.string.data}; }
    std::string_view get_string_view() const { return std::string_view{data_.string.data}; }

    ConstValueIterator begin_list() const { return ConstValueIterator(data_.list.data); }
    ConstValueIterator end_list() const { return ConstValueIterator(data_.list.data + data_.list.length); }
    auto get_list() const { return ConstListRange<GenericCloveView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(data_.object.data); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(data_.object.data + data_.object.length); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return strcmp(item.key.get_cstr(), key) == 0;
      });
    }
    ConstMemberIterator find_member(const GenericCloveView& value) const {
      return this->find_member(value.get_cstr());
    }
    auto get_object() const { return ConstMemberRange<GenericCloveView>{*this}; }

    GenericCloveView get_view() const { return GenericCloveView{data_}; }

  private:
    const DataType& data_;
  };


  template<Allocator Allocator>
  class GenericCloveRef : public GenericCloveView<Allocator> {
  public:
    using ViewType = GenericCloveView<Allocator>;
    using DataType = typename ViewType::DataType;
    using AllocatorType = Allocator;
    using ValueIterator = RefValueIteratorWrapper<
      GenericCloveRef,
      typename DataType::List::Container,
      AllocatorType
    >;
    using MemberIterator = RefCloveMemberIterator<
      GenericCloveRef,
      typename DataType::Object::Container,
      AllocatorType
    >;
    using ViewType::begin_list;
    using ViewType::end_list;
    using ViewType::get_list;
    using ViewType::begin_member;
    using ViewType::end_member;
    using ViewType::get_object;
    using ViewType::find_member;

    GenericCloveRef (
        DataType& data, AllocatorType& allocator
    ) : data_(data), allocator_(allocator), ViewType(data) {}

    GenericCloveRef (const GenericCloveRef& another) = delete;

    void set_string(const char* str) {
      this->clean();
      this->data_.type = TypeFlag::String;
      this->data_.string.length = strlen(str) + 1;
      this->data_.string.data = reinterpret_cast<char*>(allocator_.allocate(sizeof(char) * this->data_.string.length));
      strcpy(this->data_.string.data, str);
    }
    void set_string(const std::string& str) { this->set_string(str.data()); }
    void set_string(const std::string_view& str) { this->set_string(str.data()); }
    void set_double(double value) {
      this->clean();
      this->data_.type = TypeFlag::Double;
      this->data_.dvalue = value;
    }
    void set_int(int value) {
      this->clean();
      this->data_.type = TypeFlag::Integer;
      this->data_.integer = value;
    }
    void set_bool(bool value) {
      this->clean();
      this->data_.type = TypeFlag::Boolean;
      this->data_.boolean = value;
    }
    void set_null() {
      this->clean();
      this->data_.type = TypeFlag::Null;
    }
    void set_list() {
      if (this->is_list()) return;
      this->clean();
      this->data_.type = TypeFlag::List;
      this->data_.list.data = reinterpret_cast<typename DataType::List::Container>(
          allocator_.allocate(256 * sizeof(DataType))
      );
      this->data_.list.length = 0;
      this->data_.list.capacity = 256;
    }
    void set_object() {
      if (this->is_object()) return;
      this->clean();
      this->data_.type = TypeFlag::Object;
      this->data_.object.data = reinterpret_cast<typename DataType::Object::Container>(
          allocator_.allocate(128 * sizeof(Member<DataType>))
      );
      this->data_.object.length = 0;
      this->data_.object.capacity = 128;
    }

    GenericCloveRef& operator = (double value) { this->set_double(value); return *this; }
    GenericCloveRef& operator = (int value) { this->set_int(value); return *this; }
    GenericCloveRef& operator = (bool value) { this->set_bool(value); return *this; }
    GenericCloveRef& operator = (const std::string& value) { this->set_string(value); return *this; }
    GenericCloveRef& operator = (const std::string_view& value) { this->set_string(value); return *this; }
    GenericCloveRef& operator = (const char* value) { this->set_string(value); return *this; }

    ValueIterator begin_list() { return ValueIterator(data_.list.data, allocator_); }
    ValueIterator end_list() { return ValueIterator(data_.list.data + data_.list.length, allocator_); }
    ListRange<GenericCloveRef> get_list() { return ListRange<GenericCloveRef>{*this}; }

    MemberIterator begin_member() { return MemberIterator(data_.object.data, allocator_); }
    MemberIterator end_member() {
      return MemberIterator(
        data_.object.data + data_.object.length,
        allocator_
      );
    }
    MemberRange<GenericCloveRef> get_object() { return MemberRange<GenericCloveRef>{*this}; }

    // list functions
    void clear() {
      // destruct all the elements but keep the pointers as is.
      std::for_each(this->begin_list(), this->end_list(), [](auto item){ item.clean(); });
      this->data_.list.length = 0;
    }
    void push_back(DataType&& value) {
      this->check_list();
      this->data_.list.data[this->data_.list.length] = std::move(value);
      this->data_.list.length++;
    }
    void push_back() { this->push_back(DataType{}); }
    void push_back(const std::string& value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_string(value);
      this->push_back(std::move(data));
    }
    void push_back(const std::string_view& value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_string(value);
      this->push_back(std::move(data));
    }
    void push_back(const char* value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_string(value);
      this->push_back(std::move(data));
    }
    void push_back(double value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_double(value);
      this->push_back(std::move(data));
    }
    void push_back(int value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_int(value);
      this->push_back(std::move(data));
    }
    void push_back(bool value) {
      DataType data;
      GenericCloveRef(data, allocator_).set_bool(value);
      this->push_back(std::move(data));
    }
    void pop_back() {
      (*ValueIterator{this->data_.list.data + this->data_.list.length - 1, allocator_}).clean();
      this->data_.list.length--;
    }
    void erase(const ValueIterator& first, const ValueIterator& last) {
      std::for_each(first, last, [](auto item) { item.clean(); });
      auto count = last.get_inner_iterator() - first.get_inner_iterator();
      std::memmove(
          static_cast<void*>(first.get_inner_iterator()),
          static_cast<void*>(last.get_inner_iterator()),
          static_cast<size_t>(this->end_list().get_inner_iterator() - last.get_inner_iterator()) * sizeof(DataType)
      );
      data_.list.length -= count;
    }
    void erase(const ValueIterator& position) { this->erase(position, std::next(position)); }

    // member functions
    MemberIterator find_member(const char* key) {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return strcmp(item.key.get_cstr(), key) == 0;
      });
    }

    void add_member(DataType&& key, DataType&& value) {
      this->check_members();
      this->data_.object.data[this->data_.object.length] = Member<DataType>{std::move(key), std::move(value)};
      this->data_.object.length++;
    }
    void add_member(const char* key, DataType&& value) {
      DataType data; GenericCloveRef(data, allocator_).set_string(key);
      this->add_member(std::move(data), std::move(value));
    }
    void add_member(const char* key) {
      this->add_member(key, DataType{});
    }
    void add_member(const char* key, const char* value) {
      DataType data; GenericCloveRef(data, allocator_).set_string(value);
      this->add_member(key, std::move(data));
    }
    void add_member(const char* key, bool value) {
      DataType data; GenericCloveRef(data, allocator_).set_bool(value);
      this->add_member(key, std::move(data));
    }
    void add_member(const char* key, double value) {
      DataType data; GenericCloveRef(data, allocator_).set_double(value);
      this->add_member(key, std::move(data));
    }
    void add_member(const char* key, int value) {
      DataType data; GenericCloveRef(data, allocator_).set_int(value);
      this->add_member(key, std::move(data));
    }
    void remove_member(const char* key) {
      auto it = this->find_member(key);
      if (it != this->end_member()) this->erase_member(it);
    }
    void erase_member(const MemberIterator& position) {
      position->key.clean();
      position->value.clean();
      memmove(
          static_cast<void*>(position.get_pointer()),
          static_cast<void*>(position.get_pointer() + 1),
          static_cast<size_t>(this->end_member().get_inner_iterator() - position.get_inner_iterator() - 1) * sizeof(Member<DataType>)
      );
    }

    GenericCloveRef get_reference() { return GenericCloveRef(data_, allocator_); }

  private:
    DataType& data_;
    AllocatorType& allocator_;

    void check_list() {
      // make sure we have enough space for another item.
      if (this->data_.list.length >= this->data_.list.capacity) {
        this->data_.list.data = reinterpret_cast<typename DataType::List::Container>(
          allocator_.reallocate(this->data_.list.data, this->data_.list.capacity, this->data_.list.capacity * 2)
        );
        this->data_.list.capacity *= 2;
      }
    }

    void check_members() {
      // make sure we have enough space for another member.
      if (this->data_.object.length >= this->data_.object.capacity) {
        this->data_.object.data = reinterpret_cast<typename DataType::Object::Container>(
          allocator_.reallocate(this->data_.object.data, this->data_.object.capacity, this->data_.object.capacity * 2)
        );
        this->data_.object.capacity *= 2;
      }
    }

    void clean() {
      if (!AllocatorType::needs_free) return;
      switch (data_.type) {
      case TypeFlag::String:
        {
          allocator_.free(const_cast<char*>(data_.string.data));
        }
        break;
      case TypeFlag::Object:
        {
          std::for_each(this->begin_member(), this->end_member(), [](auto item) {
            item.key.clean();
            item.value.clean();
          });
          allocator_.free(data_.object.data);
        }
        break;
      case TypeFlag::List:
        {
          std::for_each(this->begin_list(), this->end_list(), [](auto item) { item.clean(); });
          allocator_.free(data_.list.data);
        }
        break;
      default:
        break;
      }
    }
  };


  template<Allocator Allocator>
  class GenericCloveDocument {
  public:
    using DataType = GenericData<Allocator>;
    using ViewType = GenericCloveView<Allocator>;
    using ReferenceType = GenericCloveRef<Allocator>;

    explicit GenericCloveDocument(std::shared_ptr<Allocator> allocator) : allocator_(allocator) {}
    GenericCloveDocument() { allocator_ = std::make_shared<Allocator>(); }
    ~GenericCloveDocument() { this->get_reference().set_null(); }

    ViewType get_view() { return ViewType{data_}; }
    ReferenceType get_reference() { return ReferenceType{data_, *allocator_}; }
    Allocator& get_allocator() { return *allocator_; }

  private:
    DataType data_;
    std::shared_ptr<Allocator> allocator_;
  };


  template<Allocator Allocator>
  class GenericCloveValue {
  public:
    using DataType = GenericData<Allocator>;
    using ViewType = GenericCloveView<Allocator>;
    using ReferenceType = GenericCloveRef<Allocator>;
    using DocumentType = GenericCloveDocument<Allocator>;

    explicit GenericCloveValue(DocumentType root) : allocator_(root.get_allocator()) {}
    ~GenericCloveValue() { this->get_reference().set_null(); }

    ViewType get_view() { return ViewType{data_}; }
    ReferenceType get_reference() { return ReferenceType{data_, allocator_}; }
    DataType&& move_data() { return std::move(data_); }

  private:
    DataType data_;
    Allocator& allocator_;
  };


  using CloveData = GenericData<CAllocator>;
  using CloveView = GenericCloveView<CAllocator>;
  using CloveRef = GenericCloveRef<CAllocator>;
  using CloveValue = GenericCloveValue<CAllocator>;
  using CloveDocument = GenericCloveDocument<CAllocator>;

}

#endif /* end of include guard: BASIC_H */