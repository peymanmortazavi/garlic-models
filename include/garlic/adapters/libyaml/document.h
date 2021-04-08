#ifndef GARLIC_LIBYAML_DOCUMENT_H
#define GARLIC_LIBYAML_DOCUMENT_H

/*! \file garlic/adapters/libyaml/document.h
 *  \brief Contains adapters for yaml_document_t
 */

#include "../../parsing/numbers.h"
#include "../../layer.h"

#include "yaml.h"

#include "error.h"


namespace garlic::adapters::libyaml {

  //! An adapter for yaml_document_t conforming to garlic::ViewLayer
  class YamlView {

    struct ValueIteratorWrapper {
      using iterator_type = yaml_node_item_t*;
      using output_type = YamlView;

      iterator_type iterator;
      yaml_document_t* doc;

      inline YamlView wrap() const {
        return YamlView(doc, yaml_document_get_node(doc, *iterator));
      }
    };

    struct MemberIteratorWrapper {
      using iterator_type = yaml_node_pair_t*;
      using output_type = MemberPair<YamlView>;

      iterator_type iterator;
      yaml_document_t* doc;

      inline output_type wrap() const {
        return output_type {
          YamlView{doc, yaml_document_get_node(doc, iterator->key)},
          YamlView{doc, yaml_document_get_node(doc, iterator->value)},
        };
      }
    };

  public:
    using ConstValueIterator = RandomAccessIterator<ValueIteratorWrapper>;
    using ConstMemberIterator = RandomAccessIterator<MemberIteratorWrapper>;

    YamlView (yaml_document_t* doc, yaml_node_t* node) : doc_(doc), node_(node) {}
    YamlView (yaml_document_t* doc) : doc_(doc) { node_ = yaml_document_get_root_node(doc); }

    bool is_null() const {
      return node_->type == yaml_node_type_t::YAML_NO_NODE || (
          is_string() && strcmp("null", get_cstr()) == 0
          );
    }
    bool is_int() const noexcept {
      int holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseInt(scalar_data(), holder)
      );
    }
    bool is_string() const noexcept { return node_->type == yaml_node_type_t::YAML_SCALAR_NODE; }
    bool is_double() const noexcept {
      double holder;
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        parsing::ParseDouble(scalar_data(), holder)
      );
    }
    bool is_object() const noexcept { return node_->type == yaml_node_type_t::YAML_MAPPING_NODE; }
    bool is_list() const noexcept { return node_->type == yaml_node_type_t::YAML_SEQUENCE_NODE; }
    bool is_bool() const noexcept {
      return (
        node_->type == yaml_node_type_t::YAML_SCALAR_NODE &&
        node_->data.scalar.style == yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE &&
        check_bool(scalar_data(), [](auto){})
      );
    }

    char* scalar_data() const noexcept { return (char*)node_->data.scalar.value; }
    int get_int() const noexcept {
      int result = 0;
      parsing::ParseInt(scalar_data(), result);
      return result;
    }
    std::string get_string() const noexcept { return std::string{scalar_data()}; }
    std::string_view get_string_view() const noexcept { return std::string_view{scalar_data()}; }
    const char* get_cstr() const noexcept { return scalar_data(); }
    double get_double() const noexcept {
      double result;
      parsing::ParseDouble(scalar_data(), result);
      return result;
    }
    bool get_bool() const noexcept {
      bool result = false;
      check_bool(scalar_data(), [&result](auto r) { result = r; });
      return result;
    }

    ConstValueIterator begin_list() const { return ConstValueIterator({node_->data.sequence.items.start, doc_}); }
    ConstValueIterator end_list() const { return ConstValueIterator({node_->data.sequence.items.top, doc_}); }
    auto get_list() const { return ConstListRange<YamlView>{*this}; }

    ConstMemberIterator begin_member() const { return ConstMemberIterator({node_->data.mapping.pairs.start, doc_}); }
    ConstMemberIterator end_member() const { return ConstMemberIterator({node_->data.mapping.pairs.top, doc_}); }
    ConstMemberIterator find_member(text key) const {
      return std::find_if(this->begin_member(), this->end_member(), [&key](const auto& item) {
        return key.compare(item.key.get_cstr()) == 0;
      });
    }
    ConstMemberIterator find_member(const YamlView& value) const { return this->find_member(value.get_cstr()); }
    auto get_object() const { return ConstMemberRange<YamlView>{*this}; }

    const yaml_node_t& get_inner_value() const { return *node_; }
    const yaml_document_t& get_inner_document() const { return *doc_; }
    YamlView get_view() const { return YamlView(*this); }

  private:
    yaml_document_t* doc_;
    yaml_node_t* node_;

    template<typename Callable>
    bool check_bool(const char* input, const Callable& cb) const {
      struct pair {
        const char* name;
        bool value;
      };
      static const pair pairs[8] = {
        {"y", true}, {"n", false},
        {"true", true}, {"false", false},
        {"on", true}, {"off", false},
        {"yes", true}, {"no", false},
      };
      for(auto it = std::begin(pairs); it != std::end(pairs); it++) {
        if (strcmp(it->name, input) == 0) {
          cb(it->value);
          return true;
        }
      }
      return false;
    }
  };


  //! An object that wraps around a yaml_document_t and deallocates it upon deletion.
  //! \note This object does **NOT** conform to garlic::ViewLayer concept.
  class YamlDocument {
  protected:
    yaml_document_t doc_;
    bool ready_ = false;

  public:
    YamlDocument() = default;
    YamlDocument(const YamlDocument&) = delete;
    YamlDocument(YamlDocument&& doc) : doc_(std::move(doc.doc_)), ready_(true) {
      doc.ready_ = false;
    }

    ~YamlDocument() {
      if (this->ready_)
        yaml_document_delete(&doc_);
    }

    YamlView get_view() {
      return YamlView{&doc_};
    }

    yaml_document_t* get_inner_document() { return &doc_; }

    //! \return whether or not the yaml_document_t life is managed by this instance.
    bool ready() const { return this->ready_; }

    //! Set if the yaml_document_t is ready.
    void set_ready(bool value) { this->ready_ = value; }

    YamlDocument move() {
      return std::move(*this);
    }
  };

  template<typename Initializer>
  static tl::expected<YamlDocument, ParserProblem>
  load(Initializer&& initializer) {
    YamlDocument doc;
    yaml_parser_t parser;

    if (!yaml_parser_initialize(&parser))
      return tl::make_unexpected(ParserProblem(parser));

    initializer(&parser);

    doc.set_ready(true);
    if (!yaml_parser_load(&parser, doc.get_inner_document())) {
      ParserProblem problem(parser);
      yaml_parser_delete(&parser);  // Done with the parser, delete it.
      return tl::make_unexpected(problem);
    }

    yaml_parser_delete(&parser);  // Everything is good, delete the parser!
    return doc;
  }

  //! Uses an initialized and ready libyaml_parser_t to load a yaml_document_t and wraps it in YamlDocument.
  //! \param parser an initialized libyaml parser.
  static YamlDocument load(yaml_parser_t* parser) {
    YamlDocument doc;
    yaml_parser_load(parser, doc.get_inner_document());
    return doc;
  }


  //! Uses libyaml parser to create a yaml_document_t and wraps it in YamlDocument.
  //! \param data valid YAML string.
  //! \param length the length of the string to load.
  static inline tl::expected<YamlDocument, ParserProblem>
  load(const char* data, size_t length) {
    return load([&](yaml_parser_t* parser) {
        yaml_parser_set_input_string(
            parser,
            reinterpret_cast<const unsigned char*>(data),
            length);
        });
  }

  //! Uses libyaml parser to create a yaml_document_t and wraps it in YamlDocument.
  //! \param data valid YAML string.
  static inline tl::expected<YamlDocument, ParserProblem>
  load(const char * data) {
    return load(data, strlen((char*)data));
  }

  //! Uses libyaml parser to create a yaml_document_t and wraps it in YamlDocument.
  //! \param file An open and readable file.
  //! \note This function does not close the file afterward.
  static tl::expected<YamlDocument, ParserProblem>
  load(FILE * file) {
    return load([&](yaml_parser_t* parser) {
        yaml_parser_set_input_file(parser, file);
        });
  }

}

#endif /* end of include guard: GARLIC_LIBYAML_DOCUMENT_H */
