#include "yaml.h"
#include <gtest/gtest.h>


TEST(LibYaml, BasicTest) {
  FILE *fh = fopen("data/test.yaml", "r");
  yaml_parser_t parser;

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if(fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  yaml_document_t doc;
  yaml_parser_load(&parser, &doc);

  auto node = yaml_document_get_root_node(&doc);
  std::cout << "tag: " << node->tag << std::endl;
  std::cout << node->type << std::endl;

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);
}
