#include "include/compiler.h"
#include "include/logger.h"
#include "include/parser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void call_cmd(const char* command) {
	printf("[CMD]: %s\n", command);
	int code = system(command);
	if (code) {
		log_error(NULL, code, "Command: '%s' exited with errorcode %d\n", command, code);
	}
}

void print_usage() {
	printf("Usage of lessl compiler:\n");
	printf("  -  lessl -h                     : Prints this message.\n");
	printf("  -  lessl <filepath>             : Compiles the '.l' file at the given path.\n");
	printf("  -  lessl <filepath> -o <output> : Compiles the '.l' file, and creates an assembly file at <output>.asm.\n");
}

typedef struct{
	char* filepath;
	char* out_path;
} args_t ;

args_t* parse_args(int argc, char** argv) {
	if (argc == 1) {
		print_usage();
		exit(1);
	}

	args_t* args = malloc(sizeof(args_t));

	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-h") == 0) {
			print_usage();
			exit(1);
		}

		if (i == 1) {
			args->filepath = argv[i];
		} else if (strcmp(argv[i], "-o") == 0) {
			args->out_path = argv[++i];
		} 

		i++;
	}

	if (args->out_path == (void*)0) {
		strncpy(args->out_path, args->filepath, strlen(args->filepath)-2);
	}

	return args;
}

int main(int argc, char** argv) {
	args_t* args = parse_args(argc, argv);

	log_info("Compiling %s\n", args->filepath);

	lexer_T* lexer = lexer_from_file(args->filepath);

	parser_T* parser = parser_new(lexer, 2);

	ast_node_T* program = parser_parse(parser);

	char asmpath[strlen(args->out_path)+5];
	snprintf(asmpath, strlen(args->out_path)+5, "%s.asm", args->out_path);
	compiler_T* compiler = compiler_new(program, parser->s_table, parser->data_table, asmpath);
	compile(compiler);

	char cmd[220];
	snprintf(cmd, 220, "nasm -felf64 -g %s.asm", args->out_path);
	call_cmd(cmd);

	snprintf(cmd, 220, "ld -g -o %s %s.o", args->out_path, args->out_path);
	call_cmd(cmd);

	return 0;
}
