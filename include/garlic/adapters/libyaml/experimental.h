/*
 *    class iterative_parser {
 *    public:
 *
 *      iterative_parser() {
 *        if (!yaml_parser_initialize(&parser_))
 *          error_ = libyaml_error::parser_init_failure;
 *      }
 *
 *      ~iterative_parser() {
 *        yaml_event_delete(&event_);
 *        yaml_parser_delete(&parser_);
 *      }
 *
 *      inline void set_input(FILE* file) {
 *        if (!file)
 *          error_ = libyaml_error::null_file;
 *        yaml_parser_set_input_file(&parser_, file);
 *      }
 *
 *      inline void set_input(const char* input, size_t length) {
 *        yaml_parser_set_input_string(
 *            &parser_,
 *            reinterpret_cast<const unsigned char*>(input),
 *            length);
 *      }
 *
 *      inline void set_input(yaml_read_handler_t* handler, void* data) {
 *        yaml_parser_set_input(&parser_, handler, data);
 *      }
 *      
 *      template<garlic::RefLayer LayerType>
 *      libyaml_error parse(LayerType&& layer) {
 *        if (error_)
 *          return error_;
 *
 *        enum class state_type : uint8_t {
 *          set, push, read_key, read_value
 *        };
 *
 *        struct node {
 *          using reference_type = decltype(layer.get_reference());
 *
 *          reference_type value;
 *          state_type state = state_type::set;
 *        };
 *
 *        std::deque<node> stack {
 *          node {layer.get_reference()}
 *        };
 *
 *        bool delete_event = false;
 *
 *        do {
 *          if (delete_event)
 *            yaml_event_delete(&event_);
 *          delete_event = true;
 *          if (!yaml_parser_parse(&parser_, &event_))
 *            return libyaml_error::parser_error;
 *          auto& item = stack.front();
 *          switch (item.state) {
 *            case state_type::push:
 *              {
 *                if (event_.type == yaml_event_type_t::YAML_SEQUENCE_END_EVENT) {
 *                  stack.pop_front();
 *                  continue;
 *                }
 *                item.value.push_back();
 *                stack.emplace_front(node { *--item.value.end_list() });
 *                // next section will update the layer.
 *              }
 *              break;
 *            case state_type::read_key:
 *              {
 *                if (event_.type == yaml_event_type_t::YAML_MAPPING_END_EVENT) {
 *                  stack.pop_front();
 *                  continue;
 *                }
 *                if (event_.type != yaml_event_type_t::YAML_SCALAR_EVENT)
 *                  return libyaml_error::parser_error;  // unexpected event.
 *                item.value.add_member(this->data());
 *                stack.emplace_front(node { (*--item.value.end_member()).value });
 *                continue;  // do not update the layer in this iteration.
 *              }
 *            case state_type::read_value:
 *              item.state = state_type::read_key;
 *            default:
 *              break;
 *          }
 *          // now update the layer.
 *          switch (event_.type) {
 *            case yaml_event_type_t::YAML_ALIAS_EVENT:
 *              return libyaml_error::parser_error;
 *            case yaml_event_type_t::YAML_SEQUENCE_START_EVENT:
 *              {
 *                stack.front().value.set_list();
 *                stack.front().state = state_type::push;
 *              }
 *              continue;
 *            case yaml_event_type_t::YAML_MAPPING_START_EVENT:
 *              {
 *                stack.front().value.set_object();
 *                stack.front().state = state_type::read_key;
 *              }
 *              continue;
 *            case yaml_event_type_t::YAML_SCALAR_EVENT:
 *              {
 *                if (event_.data.scalar.style != yaml_scalar_style_t::YAML_PLAIN_SCALAR_STYLE) {
 *                  stack.front().value.set_string(this->data());
 *                } else {
 *                  set_plain_scalar_value(stack.front().value, this->data());
 *                }
 *                stack.pop_front();
 *              }
 *            default:
 *              continue;
 *          }
 *        } while(event_.type != yaml_event_type_t::YAML_STREAM_END_EVENT);
 *
 *        return libyaml_error::no_error;
 *      }
 *
 *    private:
 *      const char* data() const {
 *        return reinterpret_cast<const char*>(event_.data.scalar.value);
 *      }
 *
 *      yaml_parser_t parser_;
 *      yaml_event_t event_;
 *      libyaml_error error_;
 *    };
 */
