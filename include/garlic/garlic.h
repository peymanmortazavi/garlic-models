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


  class value
  {
  public:
    virtual ~value() = default;

    using const_list_iterator = typename std::vector<std::shared_ptr<value>>::const_iterator;
    using const_object_iterator = typename std::map<std::string, std::shared_ptr<value>>::const_iterator;

    bool is_null()   const noexcept { return type_ & type_flag::null;         }
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
    virtual const_list_iterator begin_element() const        { throw TypeError(); }
    virtual const_list_iterator end_element() const          { throw TypeError(); }
    virtual void append(const std::shared_ptr<value>& value) { throw TypeError(); }
    virtual void append(std::shared_ptr<value>&& value)      { throw TypeError(); }
    virtual void remove(size_t index)                        { throw TypeError(); }
    virtual std::shared_ptr<value>& operator[](size_t index) { throw TypeError(); }

    // object
    virtual const_object_iterator begin_member() const                            { throw TypeError(); }
    virtual const_object_iterator end_member() const                              { throw TypeError(); }
    virtual void set(const std::string& key, const std::shared_ptr<value>& value) { throw TypeError(); }
    virtual void set(const std::string& key, std::shared_ptr<value>&& value)      { throw TypeError(); }
    virtual const std::shared_ptr<value>& get(const std::string& key) const       { throw TypeError(); }

  private:
    struct list_range {
      const value& self;
      inline const_list_iterator begin() { return self.begin_element(); }
      inline const_list_iterator end()   { return self.end_element(); }
    };

    struct object_range {
      const value& self;
      inline const_object_iterator begin() { return self.begin_member(); }
      inline const_object_iterator end()   { return self.end_member(); }
    };

  protected:
    value(const type_flag& type) : type_(type) {}
    type_flag type_;

  public:
    inline list_range get_list() const     { return list_range   { *this}; };
    inline object_range get_object() const { return object_range { *this}; };
  };


  const static std::shared_ptr<value> NoResult = nullptr;


  class string : public value
  {
  public:
    virtual ~string() = default;
    string(std::string&& text) : value_(std::move(text)), value(type_flag::string_type) {}
    const std::string& get_string() const override { return value_; }

  private:
    std::string value_;
  };


  class integer : public value
  {
  public:
    virtual ~integer() = default;
    integer(int num) : value_(num), value(type_flag::integer_type) {}
    const int& get_int() const override { return value_; }

  private:
    int value_;
  };


  class float64 : public value
  {
  public:
    virtual ~float64() = default;
    float64(double num) : value_(num), value(type_flag::double_type) {}
    const double& get_double() const override { return value_; }

  private:
    double value_;
  };


  class boolean : public value
  {
  public:
    virtual ~boolean() = default;
    boolean(bool val) : value_(val), value(type_flag::boolean_type) {}
    const bool& get_bool() const override { return value_; }

  private:
    bool value_;
  };


  class object : public value
  {
  public:
    virtual ~object() = default;
    object() : value(type_flag::object_type) {}

    const_object_iterator begin_member() const override { return table_.cbegin(); }
    const_object_iterator end_member() const override { return table_.cend(); }
    void set(const std::string& key, const std::shared_ptr<value>& value) override { table_.emplace(key, value); }
    void set(const std::string& key, std::shared_ptr<value>&& value) override { table_.emplace(key, std::move(value)); }
    const std::shared_ptr<value>& get(const std::string& key) const override {
      const auto& it = table_.find(key);
      if (it != end(table_)) {
        return it->second;
      }
      return NoResult;
    }

  private:
    std::map<std::string, std::shared_ptr<value>> table_;
  };


  class list : public value
  {
  public:
    virtual ~list() = default;
    list() : value(type_flag::list_type) {}
    list(size_t capacity) : value(type_flag::list_type) {
      items_.reserve(capacity);
    }

    const_list_iterator begin_element() const override { return items_.cbegin(); }
    const_list_iterator end_element() const override { return items_.cend(); }
    void append(const std::shared_ptr<value>& value) override { items_.push_back(value); }
    void append(std::shared_ptr<value>&& value) override { items_.push_back(value); }
    void remove(size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      items_.erase(begin(items_) + 1);
    }
    std::shared_ptr<value>& operator[](size_t index) override {
      if (index >= items_.size()) throw std::out_of_range("requested index is out of range.");
      return *(items_.begin() + index);
    }

  private:
    std::vector<std::shared_ptr<value>> items_;
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
