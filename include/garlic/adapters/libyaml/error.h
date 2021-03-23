#ifndef GARLIC_LIBYAML_ERROR_H
#define GARLIC_LIBYAML_ERROR_H

#include "yaml.h"

namespace garlic::adapters::libyaml {

  //! Describes a libyaml parser error.
  struct ParserProblem {
    yaml_mark_t mark;  //!< problem mark.
    const char* message;  //!< message.
    size_t offset;  //!< problem offset.
    int value;  //!< problem value.

    ParserProblem(const yaml_parser_t& parser
        ) : mark(parser.problem_mark), message(parser.problem),
            offset(parser.problem_offset), value(parser.problem_value) {}
  };

}

#endif /* end of include guard: GARLIC_LIBYAML_ERROR_H */
