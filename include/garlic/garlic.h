/******************************************************************************
 * File:             garlic.h
 *
 * Author:           Peyman Mortazavi
 * Created:          07/29/19
 * Description:      This file contains a lot of good stuff.
 *****************************************************************************/

#ifndef GARLIC_H
#define GARLIC_H

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace garlic
{

  class TypeError : public std::exception {};


  // ok so you need to use ref and then have non-virtual methods support const read
  // you also need to add the support for the resolve
  // then make the library and support yaml and json
  // then create python support
  // then make garlicconfig use garlic. import garlic; garlic.loads_json('asd')


  /*! \enum type_flag
   *
   *  value only supports a few types.
   */
  enum type_flag : uint8_t {
    null         = 0x0     ,
    boolean_type = 0x1 << 0,
    string_type  = 0x1 << 1,
    integer_type = 0x1 << 2,
    double_type  = 0x1 << 3,
    object_type  = 0x1 << 4,
    list_type    = 0x1 << 5,
  };


  template <template<typename...> typename ptr>
  class base_value
  {
  public:
    virtual ~base_value() = default;

    using store_type            = ptr<base_value>;
    using const_object_iterator = typename std::map<std::string, store_type>::const_iterator;

    class list_iterator : public std::iterator<std::forward_iterator_tag, base_value> {
    private:
      using it = typename std::vector<store_type>::iterator;
      it iterator_;
    public:
      explicit list_iterator(it&& iterator) : iterator_(std::move(iterator)) {}
      list_iterator& operator ++ () { iterator_++; return *this; }
      list_iterator& operator ++ (int) { list_iterator old_it = *this; ++(*this); return old_it; }
      bool operator == (const list_iterator& other) const { return other.iterator_ == this->iterator_; }
      bool operator != (const list_iterator& other) const { return !(other == *this); }
      base_value& operator * () const { return **iterator_; }
    };

    class const_list_iterator : public std::iterator<std::forward_iterator_tag, base_value> {
    private:
      using it = typename std::vector<store_type>::const_iterator;
      it iterator_;
    public:
      explicit const_list_iterator(it&& iterator) : iterator_(std::move(iterator)) {}
      const_list_iterator& operator ++ () { iterator_++; return *this; }
      const_list_iterator& operator ++ (int) { const_list_iterator old_it = *this; ++(*this); return old_it; }
      bool operator == (const const_list_iterator& other) const { return other.iterator_ == this->iterator_; }
      bool operator != (const const_list_iterator& other) const { return !(other == *this); }
      const base_value& operator * () const { return **iterator_; }
    };

    static const base_value none;

    bool is_null()   const noexcept { return ~(type_ & type_flag::null);      }
    bool is_int()    const noexcept { return type_ & type_flag::integer_type; }
    bool is_string() const noexcept { return type_ & type_flag::string_type;  }
    bool is_double() const noexcept { return type_ & type_flag::double_type;  }
    bool is_object() const noexcept { return type_ & type_flag::object_type;  }
    bool is_list()   const noexcept { return type_ & type_flag::list_type;    }
    bool is_bool()   const noexcept { return type_ & type_flag::boolean_type; }

    // Basic Types
    virtual const int& get_int() const { throw TypeError(); }
    virtual const std::string& get_string() const { throw TypeError(); }
    virtual const double& get_double() const { throw TypeError(); }
    virtual const bool& get_bool() const { throw TypeError(); }

    // list
    virtual list_iterator begin_element() { throw TypeError(); }
    virtual list_iterator end_element()   { throw TypeError(); }
    virtual const_list_iterator cbegin_element() const { throw TypeError(); }
    virtual const_list_iterator cend_element() const   { throw TypeError(); }
    virtual void append(const base_value& value)      { throw TypeError(); }
    virtual void append(base_value&& value)           { throw TypeError(); }
    virtual void remove(size_t index)                 { throw TypeError(); }
    virtual base_value& operator[](size_t index)      { throw TypeError(); }

    // object
    virtual const_object_iterator begin_member() const                { throw TypeError(); }
    virtual const_object_iterator end_member() const                  { throw TypeError(); }
    virtual void set(const std::string& key, const base_value& value) { throw TypeError(); }
    virtual void set(const std::string& key, base_value&& value)      { throw TypeError(); }
    virtual const base_value* get(const std::string& key) const       { throw TypeError(); }

    // cloning.
    virtual store_type clone() const { return store_type(new base_value(*this)); };
    virtual store_type move_clone() { return store_type(new base_value(std::move(*this))); };

  private:
    struct list_range {
      base_value& self;
      inline list_iterator begin() { return self.begin_element(); }
      inline list_iterator end()   { return self.end_element(); }
    };

    struct const_list_range {
      const base_value& self;
      inline const_list_iterator begin() const { return self.cbegin_element(); }
      inline const_list_iterator end() const { return self.cend_element(); }
    };

    struct object_range {
      const base_value& self;
      inline const_object_iterator begin() { return self.begin_member(); }
      inline const_object_iterator end()   { return self.end_member(); }
    };

  protected:
    base_value(const type_flag& type) : type_(type) {}
    type_flag type_;

  public:
    inline list_range get_list() { return list_range   { *this }; };
    inline const_list_range get_list() const { return const_list_range   { *this }; };
    inline object_range get_object() const { return object_range { *this }; };
  };

  using value = base_value<std::unique_ptr>;
  //using shareable_value = base_value<std::unique_ptr>;

  template<> const value value::none{type_flag::null};
  //template<> const shareable_value shareable_value::none{type_flag::null};

  class string : public value
  {
  public:
    virtual ~string() = default;
    string(const std::string& text="") : value_(text), value(type_flag::string_type) {}
    string(std::string&& text) : value_(std::move(text)), value(type_flag::string_type) {}
    string(const char* text) : value_(text), value(type_flag::string_type) {}
    const std::string& get_string() const override { return value_; }

    store_type clone() const override { return store_type(new string(value_)); }
    store_type move_clone() override { return store_type(new string(std::move(*this))); }

    operator const char* () const { return value_.c_str(); }
    operator const std::string& () const { return value_; }

  private:
    std::string value_;
  };


  class integer : public value
  {
  public:
    virtual ~integer() = default;
    integer(int num=0) : value_(num), value(type_flag::integer_type) {}
    const int& get_int() const override { return value_; }

    store_type clone() const override { return store_type(new integer(value_)); }
    store_type move_clone() override { return store_type(new integer(std::move(*this))); }

    operator int() const { return value_; }

  private:
    int value_;
  };


  class float64 : public value
  {
  public:
    virtual ~float64() = default;
    float64(double num=0) : value_(num), value(type_flag::double_type) {}
    const double& get_double() const override { return value_; }

    store_type clone() const override { return store_type(new float64(value_)); }
    store_type move_clone() override { return store_type(new float64(std::move(*this))); }

    operator const double& () const { return value_; }

  private:
    double value_;
  };


  class boolean : public value
  {
  public:
    virtual ~boolean() = default;
    boolean(bool val=false) : value_(val), value(type_flag::boolean_type) {}
    const bool& get_bool() const override { return value_; }

    store_type clone() const override { return store_type(new boolean(value_)); }
    store_type move_clone() override { return store_type(new boolean(std::move(*this))); }

    operator bool () const { return value_; }

  private:
    bool value_;
  };


  class object : public value
  {
  public:
    virtual ~object() = default;
    object() : value(type_flag::object_type) {}
    object(const object& other) : value(type_flag::object_type) {
      for(auto& pair : other.table_) {
        table_.emplace(pair.first, pair.second->clone());
      }
    }
    object(object&& other) : table_(std::move(other.table_)), value(type_flag::object_type) {}

    const_object_iterator begin_member() const override { return table_.cbegin(); }
    const_object_iterator end_member() const override { return table_.cend(); }
    void set(const std::string& key, const value& val) override { table_.emplace(key, val.clone()); }
    void set(const std::string& key, value&& val) override { table_.emplace(key, val.move_clone()); }
    const value* get(const std::string& key) const override {
      const auto& it = table_.find(key);
      if (it != end(table_)) {
        return it->second.get();
      }
      return nullptr;
    }

    store_type clone() const override { return store_type(new object(*this)); }
    store_type move_clone() override { return store_type(new object(std::move(*this))); }

  private:
    std::map<std::string, store_type> table_;
  };


  class list : public value
  {
  public:
    virtual ~list() = default;
    list() : value(type_flag::list_type) {}
    list(size_t capacity) : value(type_flag::list_type) {
      items_.reserve(capacity);
    }
    list(const list& other) : value(type_flag::list_type) {
      items_.reserve(other.items_.size());
      for (auto& item : other.items_) {
        items_.push_back(item->clone());
      }
    }
    list(list&& other) : items_(std::move(other.items_)), value(type_flag::list_type) {}

    list_iterator begin_element() override { return list_iterator(items_.begin()); }
    list_iterator end_element() override { return list_iterator(items_.end()); }
    const_list_iterator cbegin_element() const override { return const_list_iterator(items_.begin()); }
    const_list_iterator cend_element() const override { return const_list_iterator(items_.end()); }
    void append(const value& val) override { items_.push_back(val.clone()); }
    void append(value&& val) override { items_.push_back(val.move_clone()); }
    void remove(size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      items_.erase(begin(items_) + 1);
    }
    value& operator[](size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      return **(items_.begin() + index);
    }

    store_type clone() const override { return store_type(new list(*this)); }
    store_type move_clone() override { return store_type(new list(std::move(*this))); }

  private:
    std::vector<store_type> items_;
  };


  class validation_error : std::exception {
  public:
    explicit validation_error(const char* message) : message_(message) {}
    const char* what() const noexcept { return message_; }
  private:
    const char* message_;
  };


  class field
  {
  public:
    field (const std::string& name, bool null=false, const std::string& desc="") : name_(name), null_(null), desc_(desc) {}
    virtual ~field () = default;
    virtual void validate(const value& val) const = 0;

    const std::string& name() const noexcept { return name_; }
    const std::string& desc() const noexcept { return desc_; }
    bool allows_null() const noexcept { return null_; }

  private:
    std::string name_;
    std::string desc_;
    bool null_;
  };


  class model
  {
  public:
    virtual ~model() = default;
    void add(const std::string& name, std::unique_ptr<field> field) {
      field_table_.emplace(name, std::move(field));
    }

    template<typename FieldType, typename... Args>
    void add(const std::string& name, Args&&... args) {
      field_table_.emplace(name, std::make_unique<FieldType>(std::forward<Args>(args)...));
    }

    void validate(const value& data) const {
      if (!data.is_object()) throw validation_error{"object expected!"};
      for(auto& item : field_table_) {
        if (const auto& value = data.get(item.first)) {
          item.second->validate(*value);
        } else if(!item.second->allows_null()) {
          auto msg = (char*)malloc(item.first.size() + 100);
          sprintf(msg, "field %s cannot be null!", item.first.c_str());
          throw validation_error{msg};
        }
      }
    }

  private:
    std::map<std::string, std::unique_ptr<field>> field_table_;
  };
} /* garlic */

#endif /* GARLIC_H */
