#ifndef GARLIC_H
#define GARLIC_H

#include <exception>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "basic.h"
#include "json.h"
#include "layer.h"
#include "utility.h"
#include "allocators.h"

namespace garlic
{

  const char * version();

  //class validation_error : std::exception {
  //public:
  //  explicit validation_error(const char* message) : message_(message) {}
  //  const char* what() const noexcept { return message_; }
  //private:
  //  const char* message_;
  //};


  //class field
  //{
  //public:
  //  field (const std::string& name, bool null=false, const std::string& desc="") : name_(name), null_(null), desc_(desc) {}
  //  virtual ~field () = default;
  //  virtual void validate(const value& val) const = 0;

  //  const std::string& name() const noexcept { return name_; }
  //  const std::string& desc() const noexcept { return desc_; }
  //  bool allows_null() const noexcept { return null_; }

  //private:
  //  std::string name_;
  //  std::string desc_;
  //  bool null_;
  //};


  //class model
  //{
  //public:
  //  virtual ~model() = default;
  //  void add(const std::string& name, std::unique_ptr<field> field) {
  //    field_table_.emplace(name, std::move(field));
  //  }

  //  template<typename FieldType, typename... Args>
  //  void add(const std::string& name, Args&&... args) {
  //    field_table_.emplace(name, std::make_unique<FieldType>(std::forward<Args>(args)...));
  //  }

  //  void validate(const value& data) const {
  //    if (!data.is_object()) throw validation_error{"object expected!"};
  //    for(auto& item : field_table_) {
  //      if (const auto& value = data.get(item.first)) {
  //        item.second->validate(*value);
  //      } else if(!item.second->allows_null()) {
  //        auto msg = (char*)malloc(item.first.size() + 100);
  //        sprintf(msg, "field %s cannot be null!", item.first.c_str());
  //        throw validation_error{msg};
  //      }
  //    }
  //  }

  //private:
  //  std::map<std::string, std::unique_ptr<field>> field_table_;
  //};
} /* garlic */

#endif /* GARLIC_H */
