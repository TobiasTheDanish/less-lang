#include "include/compiler.h"
#include "include/logger.h"
#include "include/parser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void call_cmd(const char *command) {
  printf("[CMD]: %s\n", command);
  int code = system(command);
  if (code) {
    log_error(NULL, code, "Command: '%s' exited with errorcode %d\n", command,
              code);
  }
}

void print_usage() {
  printf("Usage of lessl compiler:\n");
  printf("  *  lessl <filepath> <flags>     : Compiles the '.l' file at the "
         "given path.\n");
  printf("Flags:\n");
  printf("  *  -h                           : Prints this message.\n");
  printf("  *  -o <output>                  : Specifies the output path for "
         "the compiled assembly and executable.\n");
  printf("  *  -dbg                         : Prints debug information to std "
         "out during compilation.\n");
}

typedef struct {
  char *filepath;
  char *out_path;
  unsigned char debug;
} args_t;

args_t *parse_args(int argc, char **argv) {
  if (argc == 1) {
    print_usage();
    exit(1);
  }

  args_t *args = malloc(sizeof(args_t));
  args->filepath = malloc(sizeof(char));
  args->out_path = malloc(sizeof(char));
  args->debug = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0) {
      print_usage();
      exit(1);
    }

    if (i == 1) {
      log_debug(0, "Arg is src-filepath\n");
      args->filepath = realloc(args->filepath, strlen(argv[i]));
      args->filepath = argv[i];
    } else if (strcmp(argv[i], "-o") == 0) {
      log_debug(0, "Arg is src-filepath\n");
      args->out_path = realloc(args->out_path, strlen(argv[i]));
      args->out_path = argv[++i];
    } else if (strcmp(argv[i], "-dbg") == 0) {
      args->debug = 1;
    } else {
      printf("%s is not a valid flag.\n", argv[i]);
      print_usage();
      exit(1);
    }
  }

  if (args->filepath && strlen(args->out_path) == 0) {
    args->out_path = realloc(args->out_path, strlen(args->filepath) - 2);
    strncpy(args->out_path, args->filepath, strlen(args->filepath) - 2);
    log_debug(0, "outpath: %s\n", args->out_path);
  }

  return args;
}

int main(int argc, char **argv) {
  args_t *args = parse_args(argc, argv);

  log_info("Compiling %s\n", args->filepath);

  lexer_T *lexer = lexer_from_file(args->filepath);

  parser_T *parser = parser_new(lexer, 4, args->debug);

  ast_node_T *program = parser_parse(parser);

  char asmpath[strlen(args->out_path) + 5];
  snprintf(asmpath, strlen(args->out_path) + 5, "%s.asm", args->out_path);
  compiler_T *compiler = compiler_new(program, parser->s_table,
                                      parser->data_table, asmpath, args->debug);
  compile(compiler);

  log_info("Asembling\n");
  char cmd[220];
  snprintf(cmd, 220, "nasm -felf64 -g %s.asm", args->out_path);
  call_cmd(cmd);
  log_info("Asembling finished\n");

  log_info("Linking\n");
  snprintf(cmd, 220, "ld -g -o %s %s.o -Llib -lgc -lc", args->out_path,
           args->out_path);
  call_cmd(cmd);
  log_info("Linking finished\n");

  return 0;
}
