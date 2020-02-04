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
    using list_container        = std::vector<store_type>;
    using object_container      = std::map<std::string, store_type>;

    template<typename T, typename IT>
    class list_iterator_base : public std::iterator<std::forward_iterator_tag, base_value> {
    private:
      IT iterator_;
    public:
      explicit list_iterator_base(IT&& iterator) : iterator_(std::move(iterator)) {}
      list_iterator_base& operator ++ () { iterator_++; return *this; }
      list_iterator_base& operator ++ (int) { auto old_it = *this; ++(*this); return old_it; }
      bool operator == (const list_iterator_base& other) const { return other.iterator_ == iterator_; }
      bool operator != (const list_iterator_base& other) const { return !(other == *this); }
      T& operator * () const { return **iterator_; }
    };

    using list_iterator = list_iterator_base<base_value, typename list_container::iterator>;
    using const_list_iterator = list_iterator_base<
      const base_value, typename list_container::const_iterator
    >;

    struct object_element {
      const std::string& first;
      base_value& second;
    };

    template<typename T, typename IT>
    class object_iterator_base : public std::iterator<std::forward_iterator_tag, object_element> {
    private:
      IT iterator_;
    public:
      explicit object_iterator_base(IT&& iterator) : iterator_(std::move(iterator)) {}
      object_iterator_base& operator ++ () { iterator_++; return *this; }
      object_iterator_base& operator ++ (int) { auto old_it = *this; ++(*this); old_it; }
      bool operator == (const object_iterator_base& other) const { return other.iterator_ == iterator_; }
      bool operator != (const object_iterator_base& other) const { return !(*this == other); }
      T operator * () const { return object_element{iterator_->first, *iterator_->second}; }
    };

    using object_iterator = object_iterator_base<
      object_element, typename object_container::iterator
    >;
    using const_object_iterator = object_iterator_base<
      const object_element, typename object_container::const_iterator
    >;

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
    virtual object_iterator begin_member() { throw TypeError(); }
    virtual object_iterator end_member() { throw TypeError(); }
    virtual const_object_iterator cbegin_member() const { throw TypeError(); }
    virtual const_object_iterator cend_member() const { throw TypeError(); }
    virtual void set(const std::string& key, const base_value& value) { throw TypeError(); }
    virtual void set(const std::string& key, base_value&& value)      { throw TypeError(); }
    virtual const base_value* get(const std::string& key) const       { throw TypeError(); }
    virtual base_value& operator[](const std::string& key)      { throw TypeError(); }

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
      base_value& self;
      inline object_iterator begin() { return self.begin_member(); }
      inline object_iterator end()   { return self.end_member(); }
    };

    struct const_object_range {
      const base_value& self;
      inline const_object_iterator begin() { return self.cbegin_member(); }
      inline const_object_iterator end()   { return self.cend_member(); }
    };

  protected:
    base_value(const type_flag& type) : type_(type) {}
    type_flag type_;

  public:
    inline list_range get_list() { return list_range   { *this }; };
    inline const_list_range get_list() const { return const_list_range   { *this }; };
    inline object_range get_object() { return object_range { *this }; };
    inline const_object_range get_object() const { return const_object_range { *this }; };
  };

  using value = base_value<std::unique_ptr>;
  using s_value = base_value<std::shared_ptr>;

  template<> const value value::none{type_flag::null};
  template<> const s_value s_value::none{type_flag::null};

  template<typename V>
  class string_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~string_base() = default;
    string_base(const std::string& text="") : value_(text), V(type_flag::string_type) {}
    string_base(std::string&& text) : value_(std::move(text)), V(type_flag::string_type) {}
    string_base(const char* text) : value_(text), V(type_flag::string_type) {}
    const std::string& get_string() const override { return value_; }

    store_type clone() const override { return store_type(new string_base(value_)); }
    store_type move_clone() override { return store_type(new string_base(std::move(*this))); }

    operator const char* () const { return value_.c_str(); }
    operator const std::string& () const { return value_; }

  private:
    std::string value_;
  };

  using string = string_base<value>;
  using s_string = string_base<s_value>;

  template<typename V>
  class integer_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~integer_base() = default;
    integer_base(int num=0) : value_(num), V(type_flag::integer_type) {}
    const int& get_int() const override { return value_; }

    store_type clone() const override { return store_type(new integer_base(value_)); }
    store_type move_clone() override { return store_type(new integer_base(std::move(*this))); }

    operator int() const { return value_; }

  private:
    int value_;
  };

  using integer = integer_base<value>;
  using s_integer = integer_base<s_value>;


  template<typename V>
  class float64_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~float64_base() = default;
    float64_base(double num=0) : value_(num), V(type_flag::double_type) {}
    const double& get_double() const override { return value_; }

    store_type clone() const override { return store_type(new float64_base(value_)); }
    store_type move_clone() override { return store_type(new float64_base(std::move(*this))); }

    operator const double& () const { return value_; }

  private:
    double value_;
  };

  using float64 = float64_base<value>;
  using s_float64 = float64_base<s_value>;


  template<typename V>
  class boolean_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~boolean_base() = default;
    boolean_base(bool val=false) : value_(val), V(type_flag::boolean_type) {}
    const bool& get_bool() const override { return value_; }

    store_type clone() const override { return store_type(new boolean_base(value_)); }
    store_type move_clone() override { return store_type(new boolean_base(std::move(*this))); }

    operator bool () const { return value_; }

  private:
    bool value_;
  };

  using boolean = boolean_base<value>;
  using s_boolean = boolean_base<s_value>;


  template<typename V>
  class object_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~object_base() = default;
    object_base() : V(type_flag::object_type) {}
    object_base(const object_base& other) : V(type_flag::object_type) {
      for(auto& pair : other.table_) {
        table_.emplace(pair.first, pair.second->clone());
      }
    }
    object_base(object_base&& other) : table_(std::move(other.table_)), V(type_flag::object_type) {}

    typename V::const_object_iterator cbegin_member() const override {
      return typename V::const_object_iterator{table_.cbegin()};
    }
    typename V::const_object_iterator cend_member() const override {
      return typename V::const_object_iterator{table_.cend()};
    }
    typename V::object_iterator begin_member() override {
      return typename V::object_iterator{table_.begin()};
    }
    typename V::object_iterator end_member() override {
      return typename V::object_iterator{table_.end()};
    }
    void set(const std::string& key, const V& val) override { table_.emplace(key, val.clone()); }
    void set(const std::string& key, V&& val) override { table_.emplace(key, val.move_clone()); }
    const V* get(const std::string& key) const override {
      auto it = table_.find(key);
      if (it != end(table_)) {
        return it->second.get();
      }
      return nullptr;
    }

    store_type clone() const override { return store_type(new object_base(*this)); }
    store_type move_clone() override { return store_type(new object_base(std::move(*this))); }

  private:
    std::map<std::string, store_type> table_;
  };

  using object = object_base<value>;
  using s_object = object_base<s_value>;


  template<typename V>
  class list_base : public V
  {
  public:
    using store_type = typename V::store_type;

    virtual ~list_base() = default;
    list_base() : V(type_flag::list_type) {}
    list_base(size_t capacity) : V(type_flag::list_type) {
      items_.reserve(capacity);
    }
    list_base(const list_base& other) : V(type_flag::list_type) {
      items_.reserve(other.items_.size());
      for (auto& item : other.items_) {
        items_.push_back(item->clone());
      }
    }
    list_base(list_base&& other) : items_(std::move(other.items_)), V(type_flag::list_type) {}

    typename V::list_iterator begin_element() override {
      return typename V::list_iterator(items_.begin());
    }
    typename V::list_iterator end_element() override {
      return typename V::list_iterator(items_.end());
    }
    typename V::const_list_iterator cbegin_element() const override {
      return typename V::const_list_iterator(items_.begin());
    }
    typename V::const_list_iterator cend_element() const override {
      return typename V::const_list_iterator(items_.end());
    }
    void append(const V& val) override { items_.push_back(val.clone()); }
    void append(V&& val) override { items_.push_back(val.move_clone()); }
    void remove(size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      items_.erase(begin(items_) + 1);
    }
    V& operator[](size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      return **(items_.begin() + index);
    }

    store_type clone() const override { return store_type(new list_base(*this)); }
    store_type move_clone() override { return store_type(new list_base(std::move(*this))); }

  private:
    std::vector<store_type> items_;
  };

  using list = list_base<value>;
  using s_list = list_base<s_value>;


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
