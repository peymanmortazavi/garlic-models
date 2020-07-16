#ifndef BASIC_H
#define BASIC_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include "allocators.h"
#include "json.h"
#include "layer.h"

namespace garlic {

  enum DataType : uint8_t {
    Null    = 0x0     ,
    Boolean = 0x1 << 0,
    String  = 0x1 << 1,
    Integer = 0x1 << 2,
    Double  = 0x1 << 3,
    Object  = 0x1 << 4,
    List    = 0x1 << 5,
  };

  struct String {
    size_t length;
    const char* data;
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
 
  struct Data {
    using List = Array<Data>;
    using Object = Array<Member<Data>>;

    DataType type = DataType::Null;
    union {
      double dvalue;
      int integer;
      bool boolean;
      struct String string;
      List list;
      Object object;
    };
  };

  //template<typename ValueType, typename DataType>
  //class ListIterator {
  //public:
  //  using difference_type = int;
  //  using value_type = ValueType;
  //  using reference = ValueType&;
  //  using pointer = ValueType*;
  //  using iterator_category = std::forward_iterator_tag;

  //  explicit ListIterator() {}
  //  explicit ListIterator(DataType* ptr) : ptr_(ptr) {}

  //  ListIterator& operator ++ () { ptr_++; return *this; }
  //  ListIterator operator ++ (int) { auto it = *this; ptr_++; return it; }
  //  bool operator == (const ListIterator& it) const { return it.ptr_ == this->ptr_; }
  //  bool operator != (const ListIterator& it) const { return it.ptr_ != this->ptr_; }
    
  //  ValueType operator * () const { return ValueType{*ptr_}; }
  //  pointer get_pointer() { return ptr_; }
 
  //private:
  //  DataType* ptr_;
  //};
  template<typename ValueType, typename Iterator>
  class CloveMemberIterator {
  public:
    struct MemberWrapper {
      ValueType key;
      ValueType value;
    };

    using difference_type = int;
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

    MemberWrapper operator * () const { return MemberWrapper{ValueType{this->iterator_->key}, ValueType{this->iterator_->value}}; }

  private:
    Iterator iterator_;
  };


  class CloveView {
  public:
    using ConstValueIterator = ValueIteratorWrapper<CloveView, Data::List::Container>;
    using ConstMemberIterator = CloveMemberIterator<CloveView, Data::Object::Container>;

    CloveView (const Data& data) : data_(data) {}
    CloveView (const CloveView& another) = delete;

    bool is_null() const noexcept { return data_.type & DataType::Null; }
    bool is_int() const noexcept { return data_.type & DataType::Integer; }
    bool is_string() const noexcept { return data_.type & DataType::String; }
    bool is_double() const noexcept { return data_.type & DataType::Double; }
    bool is_object() const noexcept { return data_.type & DataType::Object; }
    bool is_list() const noexcept { return data_.type & DataType::List; }
    bool is_bool() const noexcept { return data_.type & DataType::Boolean; }

    int get_int() const { return data_.integer; }
    double get_double() const { return data_.dvalue; }
    bool get_bool() const { return data_.boolean; }
    const char* get_cstr() const { return data_.string.data; }
    std::string get_string() const { return std::string{data_.string.data}; }
    std::string_view get_string_view() const { return std::string_view{data_.string.data}; }

    ConstValueIterator begin_list() const { return ConstValueIterator(data_.list.data); }
    ConstValueIterator end_list() const { return ConstValueIterator(data_.list.data + data_.list.length); }
    auto get_list() const { return ConstListRange<CloveView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(data_.object.data); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(data_.object.data + data_.object.length); }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return strcmp(item.key.get_cstr(), key) == 0;
      });
    }
    ConstMemberIterator find_member(const CloveView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<CloveView>{*this}; }

    CloveView get_view() const { return CloveView{data_}; }

  private:
    const Data& data_;
  };


  //template<garlic::Allocator AllocatorType>
  //class CloveView {
  //public:

  //  using List = Array<CloveView>;
  //  using Object = Array<Member>;
  //  using ValueIterator = ArrayIterator<CloveView>;
  //  using ConstValueIterator = ArrayIterator<const CloveView>;
  //  using MemberIterator = ArrayIterator<Member>;
  //  using ConstMemberIterator = ArrayIterator<const Member>;

  //  explicit CloveView() {}
  //  explicit CloveView(const CloveView&) = delete;
  //  ~CloveView() { this->clean(); }

  //  void set_string(const char* str) {
  //    this->clean();
  //    this->data_.type = DataType::String;
  //    this->data_.s.length = strlen(str);
  //    this->data_.s.str = reinterpret_cast<char*>(allocator_.allocate(sizeof(char) * this->data_.s.length));
  //    strcpy(this->data_.s.str, str);
  //  }
  //  void set_string(const std::string& str) { this->set_string(str.data()); }
  //  void set_string(const std::string_view& str) { this->set_string(str.data()); }
  //  void set_double(double value) {
  //    this->clean();
  //    this->data_.type = DataType::Double;
  //    this->data_.d = value;
  //  }
  //  void set_int(int value) {
  //    this->clean();
  //    this->data_.type = DataType::Integer;
  //    this->data_.i = value;
  //  }
  //  void set_bool(bool value) {
  //    this->clean();
  //    this->data_.type = DataType::Boolean;
  //    this->data_.b = value;
  //  }
  //  void set_null() {
  //    this->clean();
  //    this->data_.type = DataType::Null;
  //  }
  //  void set_list() {
  //    if (this->is_list()) return;
  //    this->clean();
  //    this->data_.type = DataType::List;
  //    this->data_.l.data = reinterpret_cast<CloveView*>(allocator_.allocate(256 * sizeof(CloveView)));
  //    this->data_.l.length = 0;
  //  }
  //  void set_object() {
  //    if (this->is_object()) return;
  //    this->clean();
  //    this->data_.type = DataType::Object;
  //    this->data_.o.data = reinterpret_cast<Member*>(allocator_.allocate(256 * sizeof(Member)));
  //    this->data_.o.length = 0;
  //  }

  //  CloveView& operator = (double value) { this->set_double(value); return *this; }
  //  CloveView& operator = (int value) { this->set_int(value); return *this; }
  //  CloveView& operator = (bool value) { this->set_bool(value); return *this; }
  //  CloveView& operator = (const std::string& value) { this->set_string(value); return *this; }
  //  CloveView& operator = (const std::string_view& value) { this->set_string(value); return *this; }
  //  CloveView& operator = (const char* value) { this->set_string(value); return *this; }

  //  ConstValueIterator begin_list() const { return ConstValueIterator(data_.l.data); }
  //  ConstValueIterator end_list() const { return ConstValueIterator(data_.l.data + data_.l.length); }
  //  ConstListRange<CloveView> get_list() const { return ConstListRange<CloveView>{*this}; }

  //  ValueIterator begin_list() { return ValueIterator(data_.l.data); }
  //  ValueIterator end_list() { return ValueIterator(data_.l.data + data_.l.length); }
  //  ListRange<CloveView> get_list() { return ConstListRange<CloveView>{*this}; }

  //  ConstMemberIterator begin_member() const { return ConstMemberIterator(data_.o.data); }
  //  ConstMemberIterator end_member() const { return ConstMemberIterator(data_.o.data + data_.o.length); }
  //  ConstMemberRange<CloveView> get_object() const { return ConstMemberRange<CloveView>{*this}; }

  //  MemberIterator begin_member() { return MemberIterator(data_.o.data); }
  //  MemberIterator end_member() { return MemberIterator(data_.o.data + data_.o.length); }
  //  MemberRange<CloveView> get_object() { return MemberRange<CloveView>{*this}; }

  //  // list functions
  //  void clear() {
  //    // destruct all the elements but keep the pointers as is.
  //    std::for_each(this->begin_list, this->end_list(), [](auto& item){ item.clean(); });
  //    this->data_.l.length = 0;
  //  }
  //  void push_back(CloveView&& value) {
  //    this->check_list();
  //    this->data_.l.data[this->data_.l.length] = std::move(value);
  //    this->data_.l.length++;
  //  }
  //  void push_back() { this->push_back(CloveView{}); }
  //  void push_back(const std::string& value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void push_back(const std::string_view& value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void push_back(const char* value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void push_back(double value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void push_back(int value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void push_back(bool value) { CloveView v; v = value; this->push_back(std::move(v)); }
  //  void pop_back() { this->data_.l.data[this->data_.l.length - 1].clean(); this->data_.l.length--; }
  //  void erase(const ValueIterator& first, const ValueIterator& last) {
  //    std::for_each(first, last, [](auto& item) { item.clean(); });
  //    memmove(
  //        static_cast<void*>(first.get_pointer()),
  //        static_cast<void*>(last.get_pointer()),
  //        static_cast<size_t>(this->end_list() - last) * sizeof(CloveView)
  //    );
  //  }
  //  void erase(const ValueIterator& position) { this->erase(position, position + 1); }

  //  // member functions
  //  MemberIterator find_member(const char* key) {
  //    return MemberIterator(this->find_member(key).get_pointer());
  //  }
  //  ConstMemberIterator find_member(const char* key) const {
  //    return std::find_if(
  //      this->begin_member(),
  //      this->end_member(),
  //      [&](auto& item) { return std::strcmp(item.key.get_cstr(), key) == 0; }
  //    );
  //  }
  //  void add_member(CloveView&& key, CloveView&& value) {
  //    this->check_members();
  //    this->data_.o.data[this->data_.o.length] = Member{std::move(key), std::move(value)};
  //    this->data_.o.length++;
  //  }
  //  void add_member(const char* key, CloveView&& value) {
  //    CloveView k; k = key;
  //    this->add_member(std::move(k), std::move(value));
  //  }
  //  void add_member(const char* key) {
  //    this->add_member(key, CloveView{});
  //  }
  //  void add_member(const char* key, const char* value) {
  //    CloveView v; v = value;
  //    this->add_member(key, v);
  //  }
  //  void add_member(const char* key, bool value) {
  //    CloveView v; v = value;
  //    this->add_member(key, v);
  //  }
  //  void add_member(const char* key, double value) {
  //    CloveView v; v = value;
  //    this->add_member(key, v);
  //  }
  //  void add_member(const char* key, int value) {
  //    CloveView v; v = value;
  //    this->add_member(key, v);
  //  }
  //  void remove_member(const char* key) {
  //    auto it = this->find_member(key);
  //    if (it != this->end_member()) this->erase_member(it);
  //  }
  //  void erase_member(const MemberIterator& position) {
  //    position->key.clean();
  //    position->value.clean();
  //    memmove(
  //        static_cast<void*>(position.get_pointer()),
  //        static_cast<void*>(position.get_pointer() + 1),
  //        static_cast<size_t>(this->end_member() - position - 1) * sizeof(Member)
  //    );
  //  }

  //private:
  //  Data data_;
  //  AllocatorType allocator_;

  //  void check_list() {
  //    // make sure we have enough space for another item.
  //    if (this->data_.l.length >= this->data_.l.capacity) {
  //      this->data_.l.data = reinterpret_cast<CloveView*>(
  //        allocator_.reallocate(this->data_.l.data, this->data_.l.capacity, this->data_.l.capacity * 2)
  //      );
  //    }
  //  }

  //  void check_members() {
  //    // make sure we have enough space for another member.
  //    if (this->data_.o.length >= this->data_.o.capacity) {
  //      this->data_.o.data = reinterpret_cast<Member*>(
  //        allocator_.reallocate(this->data_.o.data, this->data_.o.capacity, this->data_.o.capacity * 2)
  //      );
  //    }
  //  }

  //  void clean() {
  //    if (AllocatorType::needs_free) return;
  //    switch (data_.type) {
  //    case DataType::String:
  //      {
  //        allocator_.free(const_cast<char*>(data_.s.str));
  //      }
  //      break;
  //    case DataType::Object:
  //      {
  //        std::for_each(this->begin_member(), this->end_member(), [](auto& item) {
  //          item.key.clean();
  //          item.value.clean();
  //        });
  //        allocator_.free(data_.o.data);
  //      }
  //      break;
  //    case DataType::List:
  //      {
  //        std::for_each(this->begin_list(), this->end_list(), [](auto& item) { item.clean(); });
  //        allocator_.free(data_.l.data);
  //      }
  //      break;
  //    default:
  //      break;
  //    }
  //  }
  //};

}

#endif /* end of include guard: BASIC_H */
