#ifndef GARLIC_CLOVE_H
#define GARLIC_CLOVE_H

/*! \file clove.h
 *  \brief Built-in type that provides readable and writable layers.
 *  \code
 *  CloveDocument doc;  // contains an allocator for the entire object. (conforms to garlic::RefLayer)
 *  CloveView view = doc.get_view();  // a view to the doc. (conforms to garlic::ViewLayer)
 *  CloveRef ref = doc.get_reference();  // get a reference to the doc. (conforms to garlic::RefLayer)
 *  CloveValue value(doc);  // use the allocator of the document root. (conforms to garlic::RefLayer)
 *  \endcode
 */

#include "garlic.h"
#include "allocators.h"
#include "layer.h"

namespace garlic {

  template<typename SizeType = unsigned>
  struct StringData {
    SizeType length;
    char* data;
  };

  template<typename Type, typename SizeType = unsigned>
  struct Array {
    using Container = Type*;

    SizeType capacity;
    SizeType length;
    Container data;
  };

  template<GARLIC_ALLOCATOR Allocator, typename SizeType = unsigned>
  struct GenericData {
    using List = Array<GenericData, SizeType>;
    using Object = Array<MemberPair<GenericData>, SizeType>;
    using AllocatorType = Allocator;

    TypeFlag type = TypeFlag::Null;
    union {
      double dvalue;
      int integer;
      bool boolean;
      StringData<SizeType> string;
      List list;
      Object object;
    };
  };

  template<typename Layer, typename Iterator>
  struct ConstMemberIteratorWrapper {
    using output_type = MemberPair<Layer>;
    using iterator_type = Iterator;

    iterator_type iterator;

    inline output_type wrap() const {
      return output_type {
        Layer { iterator->key },
        Layer { iterator->value }
      };
    }
  };

  template<typename Layer, typename Iterator, typename Allocator>
  struct MemberIteratorWrapper {
    using output_type = MemberPair<Layer>;
    using iterator_type = Iterator;
    using allocator_type = Allocator;

    iterator_type iterator;
    allocator_type* allocator;

    inline output_type wrap() const {
      return output_type {
        Layer { iterator->key, *allocator },
        Layer { iterator->value, *allocator }
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

  template<GARLIC_ALLOCATOR Allocator, typename SizeType = unsigned>
  class GenericCloveView {
  public:
    using DataType = GenericData<Allocator, SizeType>;
    using ProviderValueIterator = typename DataType::List::Container;
    using ProviderMemberIterator = typename DataType::Object::Container;
    using ConstValueIterator = BasicRandomAccessIterator<GenericCloveView, ProviderValueIterator>;
    using ConstMemberIterator = RandomAccessIterator<
      ConstMemberIteratorWrapper<GenericCloveView, ProviderMemberIterator>>;

    GenericCloveView (const DataType& data) : data_(data) {}

    bool is_null() const noexcept { return data_.type & TypeFlag::Null; }
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
    std::string get_string() const {
      return std::string{data_.string.data, data_.string.length};
    }
    std::string_view get_string_view() const {
      return std::string_view{data_.string.data, data_.string.length};
    }

    ConstValueIterator begin_list() const { return ConstValueIterator({data_.list.data}); }
    ConstValueIterator end_list() const {
      return ConstValueIterator({data_.list.data + data_.list.length});
    }
    auto get_list() const { return ConstListRange<GenericCloveView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator({data_.object.data}); }
    ConstMemberIterator end_member() const {
      return ConstMemberIterator({data_.object.data + data_.object.length});
    }
    ConstMemberIterator find_member(const char* key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return strcmp(item.key.get_cstr(), key) == 0;
      });
    }
    ConstMemberIterator find_member(std::string_view key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return key.compare(item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(const GenericCloveView& value) const {
      return this->find_member(value.get_cstr());
    }
    auto get_object() const { return ConstMemberRange<GenericCloveView>{*this}; }

    GenericCloveView get_view() const { return GenericCloveView{data_}; }
    const DataType& get_inner_value() const { return data_; }

  private:
    const DataType& data_;
  };


  template<GARLIC_ALLOCATOR Allocator, typename SizeType = unsigned>
  class GenericCloveRef : public GenericCloveView<Allocator> {
  public:
    using ViewType = GenericCloveView<Allocator, SizeType>;
    using DataType = typename ViewType::DataType;
    using AllocatorType = Allocator;
    using ProviderValueIterator = typename ViewType::ProviderValueIterator;
    using ProviderMemberIterator = typename ViewType::ProviderMemberIterator;
    using ValueIterator = RandomAccessIterator<
      ValueIteratorWrapper<GenericCloveRef, ProviderValueIterator, AllocatorType>>;
    using MemberIterator = RandomAccessIterator<
      MemberIteratorWrapper<GenericCloveRef, ProviderMemberIterator, AllocatorType>>;
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

    void set_string(const char* str) {
      this->prepare_string(strlen(str));
      strcpy(this->data_.string.data, str);
    }
    void set_string(const std::string& str) {
      this->prepare_string(str.size());
      strncpy(this->data_.string.data, str.c_str(), str.size());
    }
    void set_string(std::string_view str) {
      this->prepare_string(str.size());
      strncpy(this->data_.string.data, str.data(), str.size());
    }
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
      this->data_.list.capacity = 16;
    }
    void set_object() {
      if (this->is_object()) return;
      this->clean();
      this->data_.type = TypeFlag::Object;
      this->data_.object.data = reinterpret_cast<typename DataType::Object::Container>(
          allocator_.allocate(16 * sizeof(MemberPair<DataType>))
      );
      this->data_.object.length = 0;
      this->data_.object.capacity = 16;
    }

    GenericCloveRef& operator = (double value) { this->set_double(value); return *this; }
    GenericCloveRef& operator = (int value) { this->set_int(value); return *this; }
    GenericCloveRef& operator = (bool value) { this->set_bool(value); return *this; }
    GenericCloveRef& operator = (const std::string& value) {
      this->set_string(value); return *this;
    }
    GenericCloveRef& operator = (const std::string_view& value) {
      this->set_string(value); return *this;
    }
    GenericCloveRef& operator = (const char* value) { this->set_string(value); return *this; }

    ValueIterator begin_list() {
      return ValueIterator({data_.list.data, &allocator_});
    }
    ValueIterator end_list() {
      return ValueIterator({data_.list.data + data_.list.length, &allocator_});
    }
    ListRange<GenericCloveRef> get_list() { return ListRange<GenericCloveRef>{*this}; }

    MemberIterator begin_member() { return MemberIterator({data_.object.data, &allocator_}); }
    MemberIterator end_member() {
      return MemberIterator({
        data_.object.data + data_.object.length,
        &allocator_
        });
    }
    MemberRange<GenericCloveRef> get_object() { return MemberRange<GenericCloveRef>{*this}; }

    // list functions
    void clear() {
      // destruct all the elements but keep the pointers as is.
      std::for_each(this->begin_list(), this->end_list(), [](auto item){ item.clean(); });
      this->data_.list.length = 0;
    }

    template<typename Callable>
    void push_back_builder(Callable&& cb) {
      this->check_list();
      DataType value;
      cb(GenericCloveRef(value, allocator_));
      this->push_back(std::move(value));
    }
    void push_back(DataType&& value) {
      this->check_list();
      this->data_.list.data[this->data_.list.length++] = std::move(value);
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
      (*ValueIterator({this->data_.list.data + this->data_.list.length - 1, &allocator_})).clean();
      this->data_.list.length--;
    }
    void erase(const ValueIterator& first, const ValueIterator& last) {
      std::for_each(first, last, [](auto item) { item.clean(); });
      auto count = last.get_inner_iterator() - first.get_inner_iterator();
      std::memmove(
          static_cast<void*>(first.get_inner_iterator()),
          static_cast<void*>(last.get_inner_iterator()),
          static_cast<SizeType>(this->end_list().get_inner_iterator() - last.get_inner_iterator()) * sizeof(DataType)
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
    MemberIterator find_member(std::string_view key) {
      return std::find_if(this->begin_member(), this->end_member(), [&key](auto item) {
        return key.compare(item.key.get_cstr()) == 0;
      });
    }

    template<typename Callable>
    void add_member_builder(const char* key, Callable&& cb) {
      DataType value;
      cb(GenericCloveRef(value, allocator_));
      this->add_member(key, std::move(value));
    }

    void add_member(DataType&& key, DataType&& value) {
      this->check_members();
      this->data_.object.data[this->data_.object.length] = MemberPair<DataType>{std::move(key), std::move(value)};
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
      (*position).key.clean();
      (*position).value.clean();
      memmove(
          static_cast<void*>(position.get_inner_iterator()),
          static_cast<void*>(position.get_inner_iterator() + 1),
          static_cast<SizeType>(this->end_member().get_inner_iterator() - position.get_inner_iterator() - 1) * sizeof(MemberPair<DataType>)
      );
    }

    GenericCloveRef get_reference() { return GenericCloveRef(data_, allocator_); }
    DataType& get_inner_value() { return data_; }

  private:
    DataType& data_;
    AllocatorType& allocator_;

    void check_list() {
      // make sure we have enough space for another item.
      if (this->data_.list.length >= this->data_.list.capacity) {
        auto new_capacity = this->data_.list.capacity + (this->data_.list.capacity + 1) / 2;
        this->data_.list.data = reinterpret_cast<typename DataType::List::Container>(
          allocator_.reallocate(
            this->data_.list.data,
            this->data_.list.capacity * sizeof(DataType),
            new_capacity * sizeof(DataType))
        );
        this->data_.list.capacity = new_capacity;
      }
    }

    void check_members() {
      // make sure we have enough space for another member.
      if (this->data_.object.length >= this->data_.object.capacity) {
        auto new_capacity = this->data_.object.capacity + (this->data_.object.capacity + 1) / 2;
        this->data_.object.data = reinterpret_cast<typename DataType::Object::Container>(
          allocator_.reallocate(
            this->data_.object.data,
            this->data_.object.capacity * sizeof(DataType),
            new_capacity * sizeof(DataType))
        );
        this->data_.object.capacity = new_capacity;
      }
    }

    inline void prepare_string(SizeType length) {
      this->clean();
      this->data_.type = TypeFlag::String;
      this->data_.string.length = length;
      this->data_.string.data = reinterpret_cast<char*>(
          allocator_.allocate(sizeof(char) * (length + 1)));
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


  template<GARLIC_ALLOCATOR Allocator, typename SizeType = unsigned>
  class GenericCloveDocument : public GenericCloveRef<Allocator, SizeType> {
  public:
    using DataType = GenericData<Allocator, SizeType>;
    using ViewType = GenericCloveView<Allocator, SizeType>;
    using ReferenceType = GenericCloveRef<Allocator, SizeType>;
    using DocumentType = GenericCloveDocument<Allocator, SizeType>;

    explicit GenericCloveDocument(
        std::shared_ptr<Allocator> allocator
        ) : allocator_(allocator), ReferenceType(data_, *allocator) {}
    GenericCloveDocument(
        ) : allocator_(std::make_shared<Allocator>()), ReferenceType(data_, *allocator_) {}
    ~GenericCloveDocument() { this->get_reference().set_null(); }

    Allocator& get_allocator() { return *allocator_; }

  private:
    DataType data_;
    std::shared_ptr<Allocator> allocator_;
  };


  template<GARLIC_ALLOCATOR Allocator, typename SizeType = unsigned>
  class GenericCloveValue : public GenericCloveRef<Allocator, SizeType> {
  public:
    using DataType = GenericData<Allocator>;
    using ViewType = GenericCloveView<Allocator>;
    using ReferenceType = GenericCloveRef<Allocator>;
    using DocumentType = GenericCloveDocument<Allocator>;

    explicit GenericCloveValue(
        DocumentType& root) : ReferenceType(data_, root.get_allocator()) {}

    explicit GenericCloveValue(
        Allocator& allocator) : ReferenceType(data_, allocator) {}
    ~GenericCloveValue() { this->get_reference().set_null(); }

    DataType&& move_data() { return std::move(data_); }

  private:
    DataType data_;
  };


  //! A view to the a clove value/document conforming to garlic::ViewLayer.
  using CloveView = GenericCloveView<CAllocator>;

  //! A reference to the a clove value/document conforming to garlic::RefLayer.
  using CloveRef = GenericCloveRef<CAllocator>;

  //! A clove value conforming to garlic::RefLayer.
  using CloveValue = GenericCloveValue<CAllocator>;

  //! A clove document (value) conforming to garlic::RefLayer.
  using CloveDocument = GenericCloveDocument<CAllocator>;

}

#endif /* end of include guard: GARLIC_CLOVE_H */
