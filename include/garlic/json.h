#ifndef JSON_H
#define JSON_H

#include <string>

#include "rapidjson/document.h"


namespace garlic {

  class rapidjson_wrapper {
  public:
    explicit rapidjson_wrapper(rapidjson::Document&& doc) : document_(std::move(doc)) {};
    //explicit rapidjson_wrapper(const rapidjson::Value& doc) : document_(doc) {};

    bool is_null() const noexcept { return document_.IsNull(); }
    bool is_int() const noexcept { return document_.IsInt(); }
    bool is_string() const noexcept { return document_.IsString(); }
    bool is_double() const noexcept { return document_.IsDouble(); }
    bool is_object() const noexcept { return document_.IsObject(); }
    bool is_list() const noexcept { return document_.IsArray(); }
    bool is_bool() const noexcept { return document_.IsBool(); }

    int get_int() const { return document_.GetInt(); }
    std::string get_string() const { return document_.GetString(); }
    double get_double() const { return document_.GetDouble(); }
    bool get_bool() const { return document_.GetBool(); }

    template<typename T, typename IT>
    class list_iterator_base : public std::iterator<std::forward_iterator_tag, rapidjson_wrapper> {
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

    using list_iterator = list_iterator_base<rapidjson_wrapper, typename rapidjson::Value::ValueIterator>;
    using const_list_iterator = list_iterator_base<const rapidjson_wrapper, typename rapidjson::Value::ConstValueIterator>;

    list_iterator begin_element() { return list_iterator{document_.Begin()}; }
    list_iterator end_element() { return list_iterator{document_.End()}; }
    const_list_iterator cbegin_element() const { return const_list_iterator{document_.Begin()}; }
    const_list_iterator cend_element() const { return const_list_iterator{document_.End()}; }
    //void append(const std::string& value) { document_.PushBack(value, document_.GetAllocator()); }

    struct list_range {
      rapidjson_wrapper& self;
      list_iterator begin() const { return self.begin_element(); }
      list_iterator end() const   { return self.end_element(); }
    };

    struct const_list_range {
      const rapidjson_wrapper& self;
      const_list_iterator begin() const { return self.cbegin_element(); }
      const_list_iterator end() const { return self.cend_element(); }
    };

    list_range get_list() { return list_range{*this}; }
    const_list_range get_list() const { return const_list_range{*this}; }

  private:
    rapidjson::Document document_;
  };

}


#endif /* JSON_H */
