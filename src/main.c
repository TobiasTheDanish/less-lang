#include "include/compiler.h"
#include "include/parser.h"
#include <stdlib.h>
#include <string.h>

void call_cmd(const char* command) {
	printf("%s\n", command);
	system(command);
}

void print_usage() {
	printf("Usage of less compiler:\n");
	printf("  -  less <filepath>             : Compiles the '.l' file at the given path.\n");
	printf("  -  less <filepath> -o <output> : Compiles the '.l' file, and creates an assembly file at <output>.asm.\n");
}

int main(int argc, char** argv) {
	if (argc == 1) {
		print_usage();
		exit(1);
	}

	char* filepath = argv[1];
	lexer_T* lexer = lexer_from_file(filepath);
	parser_T* parser = parser_new(lexer, 2);
	ast_node_T* program = parser_parse(parser);
	if (argc == 2) {
		compiler_T* compiler = compiler_new(program, "output.asm");
		compile(compiler);
	} else if (argc == 4) {
		char* output = argv[3];
		char outfile[strlen(output)+5];
		snprintf(outfile, strlen(output)+5, "%s.asm", output);
		compiler_T* compiler = compiler_new(program, outfile);
		compile(compiler);
	} else {
		print_usage();
		exit(1);
	}

	return 0;
}
