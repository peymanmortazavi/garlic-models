#ifndef BASIC_H
#define BASIC_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include "allocators.h"
#include "json.h"

namespace garlic {

  enum BasicType : uint8_t {
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
    const char* str;
  };

  template<typename T>
  struct Array {
    size_t capacity;
    size_t length;
    T* data;
  };

  template<typename T>
  class ArrayIterator {
  public:
    using difference_type = int;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using iterator_category = std::forward_iterator_tag;

    explicit ArrayIterator() {}
    explicit ArrayIterator(T* ptr) : ptr_(ptr) {}

    ArrayIterator& operator ++ () { ptr_++; return *this; }
    ArrayIterator operator ++ (int) { auto it = *this; ptr_++; return it; }
    bool operator == (const ArrayIterator& it) const { return it.ptr_ == this->ptr_; }
    bool operator != (const ArrayIterator& it) const { return it.ptr_ != this->ptr_; }
    
    reference operator * () const { return *ptr_; }
    pointer get_pointer() { return ptr_; }
 
  private:
    T* ptr_;
  };

  template<garlic::Allocator AllocatorType>
  class BasicValue {
  public:

    struct Member {
      BasicValue key;
      BasicValue value;
    };

    using List = Array<BasicValue>;
    using Object = Array<Member>;
    using ValueIterator = ArrayIterator<BasicValue>;
    using ConstValueIterator = ArrayIterator<const BasicValue>;
    using MemberIterator = ArrayIterator<Member>;
    using ConstMemberIterator = ArrayIterator<const Member>;

    explicit BasicValue() {}
    explicit BasicValue(const BasicValue&) = delete;
    ~BasicValue() { this->clean(); }

    bool is_null() const noexcept { return data_.type & BasicType::Null; }
    bool is_int() const noexcept { return data_.type & BasicType::Integer; }
    bool is_string() const noexcept { return data_.type & BasicType::String; }
    bool is_double() const noexcept { return data_.type & BasicType::Double; }
    bool is_object() const noexcept { return data_.type & BasicType::Object; }
    bool is_list() const noexcept { return data_.type & BasicType::List; }
    bool is_bool() const noexcept { return data_.type & BasicType::Boolean; }

    int get_int() const { return data_.i; }
    double get_double() const { return data_.d; }
    bool get_bool() const { return data_.b; }
    const char* get_cstr() const { return data_.s.str; }
    std::string get_string() const { return std::string{data_.s.str}; }
    std::string_view get_string_view() const { return std::string_view{data_.s.str}; }

    void set_string(const char* str) {
      this->clean();
      this->data_.type = BasicType::String;
      this->data_.s.length = strlen(str);
      this->data_.s.str = reinterpret_cast<char*>(allocator_.allocate(sizeof(char) * this->data_.s.length));
      strcpy(this->data_.s.str, str);
    }
    void set_string(const std::string& str) { this->set_string(str.data()); }
    void set_string(const std::string_view& str) { this->set_string(str.data()); }
    void set_double(double value) {
      this->clean();
      this->data_.type = BasicType::Double;
      this->data_.d = value;
    }
    void set_int(int value) {
      this->clean();
      this->data_.type = BasicType::Integer;
      this->data_.i = value;
    }
    void set_bool(bool value) {
      this->clean();
      this->data_.type = BasicType::Boolean;
      this->data_.b = value;
    }
    void set_null() {
      this->clean();
      this->data_.type = BasicType::Null;
    }
    void set_list() {
      if (this->is_list()) return;
      this->clean();
      this->data_.type = BasicType::List;
      this->data_.l.data = reinterpret_cast<BasicValue*>(allocator_.allocate(256 * sizeof(BasicValue)));
      this->data_.l.length = 0;
    }
    void set_object() {
      if (this->is_object()) return;
      this->clean();
      this->data_.type = BasicType::Object;
      this->data_.o.data = reinterpret_cast<Member*>(allocator_.allocate(256 * sizeof(Member)));
      this->data_.o.length = 0;
    }

    BasicValue& operator = (double value) { this->set_double(value); return *this; }
    BasicValue& operator = (int value) { this->set_int(value); return *this; }
    BasicValue& operator = (bool value) { this->set_bool(value); return *this; }
    BasicValue& operator = (const std::string& value) { this->set_string(value); return *this; }
    BasicValue& operator = (const std::string_view& value) { this->set_string(value); return *this; }
    BasicValue& operator = (const char* value) { this->set_string(value); return *this; }

    ConstValueIterator begin_list() const { return ConstValueIterator(data_.l.data); }
    ConstValueIterator end_list() const { return ConstValueIterator(data_.l.data + data_.l.length); }
    ConstListRange<BasicValue> get_list() const { return ConstListRange<BasicValue>{*this}; }

    ValueIterator begin_list() { return ValueIterator(data_.l.data); }
    ValueIterator end_list() { return ValueIterator(data_.l.data + data_.l.length); }
    ListRange<BasicValue> get_list() { return ConstListRange<BasicValue>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator(data_.o.data); }
    ConstMemberIterator end_member() const { return ConstMemberIterator(data_.o.data + data_.o.length); }
    ConstMemberRange<BasicValue> get_object() const { return ConstMemberRange<BasicValue>{*this}; }

    MemberIterator begin_member() { return MemberIterator(data_.o.data); }
    MemberIterator end_member() { return MemberIterator(data_.o.data + data_.o.length); }
    MemberRange<BasicValue> get_object() { return MemberRange<BasicValue>{*this}; }

    // list functions
    void clear() {
      // destruct all the elements but keep the pointers as is.
      std::for_each(this->begin_list, this->end_list(), [](auto& item){ item.clean(); });
      this->data_.l.length = 0;
    }
    void push_back(BasicValue&& value) {
      this->check_list();
      this->data_.l.data[this->data_.l.length] = std::move(value);
      this->data_.l.length++;
    }
    void push_back() { this->push_back(BasicValue{}); }
    void push_back(const std::string& value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void push_back(const std::string_view& value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void push_back(const char* value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void push_back(double value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void push_back(int value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void push_back(bool value) { BasicValue v; v = value; this->push_back(std::move(v)); }
    void pop_back() { this->data_.l.data[this->data_.l.length - 1].clean(); this->data_.l.length--; }
    void erase(const ValueIterator& first, const ValueIterator& last) {
      std::for_each(first, last, [](auto& item) { item.clean(); });
      memmove(
          static_cast<void*>(first.get_pointer()),
          static_cast<void*>(last.get_pointer()),
          static_cast<size_t>(this->end_list() - last) * sizeof(BasicValue)
      );
    }
    void erase(const ValueIterator& position) { this->erase(position, position + 1); }

    // member functions
    MemberIterator find_member(const char* key) {
      return MemberIterator(this->find_member(key).get_pointer());
    }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(
        this->begin_member(),
        this->end_member(),
        [&](auto& item) { return std::strcmp(item.key.get_cstr(), key) == 0; }
      );
    }
    void add_member(BasicValue&& key, BasicValue&& value) {
      this->check_members();
      this->data_.o.data[this->data_.o.length] = Member{std::move(key), std::move(value)};
      this->data_.o.length++;
    }
    void add_member(const char* key, BasicValue&& value) {
      BasicValue k; k = key;
      this->add_member(std::move(k), std::move(value));
    }
    void add_member(const char* key) {
      this->add_member(key, BasicValue{});
    }
    void add_member(const char* key, const char* value) {
      BasicValue v; v = value;
      this->add_member(key, v);
    }
    void add_member(const char* key, bool value) {
      BasicValue v; v = value;
      this->add_member(key, v);
    }
    void add_member(const char* key, double value) {
      BasicValue v; v = value;
      this->add_member(key, v);
    }
    void add_member(const char* key, int value) {
      BasicValue v; v = value;
      this->add_member(key, v);
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
          static_cast<size_t>(this->end_member() - position - 1) * sizeof(Member)
      );
    }
 
    struct Data {
      BasicType type = BasicType::Null;
      union {
        double d;
        int i;
        bool b;
        struct String s;
        List l;
        Object o;
      };
    };

  private:
    Data data_;
    AllocatorType allocator_;

    void check_list() {
      // make sure we have enough space for another item.
      if (this->data_.l.length >= this->data_.l.capacity) {
        this->data_.l.data = reinterpret_cast<BasicValue*>(
          allocator_.reallocate(this->data_.l.data, this->data_.l.capacity, this->data_.l.capacity * 2)
        );
      }
    }

    void check_members() {
      // make sure we have enough space for another member.
      if (this->data_.o.length >= this->data_.o.capacity) {
        this->data_.o.data = reinterpret_cast<Member*>(
          allocator_.reallocate(this->data_.o.data, this->data_.o.capacity, this->data_.o.capacity * 2)
        );
      }
    }

    void clean() {
      if (AllocatorType::needs_free) return;
      switch (data_.type) {
      case BasicType::String:
        {
          allocator_.free(const_cast<char*>(data_.s.str));
        }
        break;
      case BasicType::Object:
        {
          std::for_each(this->begin_member(), this->end_member(), [](auto& item) {
            item.key.clean();
            item.value.clean();
          });
          allocator_.free(data_.o.data);
        }
        break;
      case BasicType::List:
        {
          std::for_each(this->begin_list(), this->end_list(), [](auto& item) { item.clean(); });
          allocator_.free(data_.l.data);
        }
        break;
      default:
        break;
      }
    }
  };

}

#endif /* end of include guard: BASIC_H */
