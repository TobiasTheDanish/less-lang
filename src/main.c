#include "include/compiler.h"
#include "include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void call_cmd(const char* command) {
	printf("[CMD]: %s\n", command);
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
	char output[100];
	char outfile[strlen(output)+5];
	if (argc == 2) {
		strncpy(output, filepath, strlen(filepath)-2);
		snprintf(outfile, strlen(output)+5, "%s.asm", output);
	} else if (argc == 4) {
		strncpy(output, argv[3], 100);
		snprintf(outfile, strlen(output)+5, "%s.asm", output);
	} else {
		print_usage();
		exit(1);
	}

	compiler_T* compiler = compiler_new(program, parser->s_table, parser->data_table, outfile);
	compile(compiler);

	char cmd[220];
	snprintf(cmd, 220, "nasm -felf64 -g %s", outfile);
	call_cmd(cmd);

	snprintf(cmd, 220, "ld -g -o %s %s.o", output, output);
	call_cmd(cmd);

	return 0;
}
