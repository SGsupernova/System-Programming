#include "20141570.h"
#include "tokenizer.h"
#include "linking_loader.h"
#include <stdlib.h>

int linking_loader_main (int num_command, const char * inputStr) {
	static struct bpLink * bpLinkHead = NULL;
	static int progaddr = 0;

	struct reg regSet;

	int temp_int = 0;
	int error_flag = 0;

	switch (num_command) {
		case 1 : // progaddr command
			temp_int = command_progaddr(inputStr, &error_flag);
			if (error_flag) { break; }
			progaddr = temp_int;
			break;

		case 2: // loader command
			error_flag = command_loader(inputStr);
			break;

		case 3 : // run command
			initRegister(&regSet);
			command_run(regSet);
			break;

		case 4 : // break point command
			error_flag = command_bp(&bpLinkHead, inputStr);
			break;
	}
}

int command_progaddr (const char * inputStr, int * error_flag) {
	int progaddr = strtoi (inputStr, error_flag, 16);
	if (*error_flag) {
		SEND_ERROR_MESSAGE("INPUT DOES NOT ADDRESS");
		return 0;
	}
	if (isOverMemoryBoundary(progaddr)) {
		SEND_ERROR_MESSAGE("OVER MEMORY BOUNDARY");
		return 0;
	}

	return progaddr;
}

int command_loader (const char * inputStr) {
	char ** object_filename = NULL,
		 * extension = NULL;
	int argc = 0, i = 0;
	int prog_len = 0;

	tokenizer(inputStr, &argc, &object_filename);

	if (!argc) { // there is no argument
		SEND_ERROR_MESSAGE("THERE IS NO ARGUMENT");
		return 1;
	}

	for (i = 0; i < argc; i++) {
		extension = fetch_filename_extension(object_filename[i]);
		
		if (!extension || strcmp(extension, "obj")) { // there is no extension \ error
			SEND_ERROR_MESSAGE("(A extension must be .obj) EXTENSION");
			return 1;
		}
	}

	// all extensions of filenames are .obj

	linking_loader_print_load_map(prog_len);

	return 0;
}

int command_run (struct reg regSet) {
	

	print_register_set(regSet);
}

// return is_error
int command_bp (struct bpLink ** bpLinkHead_ptr, const char * inputStr) {
	int argc = 0, addr = 0, error_flag = 0;
	char ** argv = NULL;

	tokenizer(inputStr, &argc, &argv);

	// error
	if (argc != 1) {
		SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
		return 1;
	}

	addr = strtoi(argv[0], &error_flag, 16);

	if (error_flag) {
		if (argv[0] && !strcmp(argv[0], "clear")) { // bp clear
			bp_clear(bpLinkHead_ptr);
		}
		else {
			SEND_ERROR_MESSAGE("ARGUMENT");
			return 1;
		}
	}
	else if (isOverMemoryBoundary(addr)) {
		SEND_ERROR_MESSAGE("OVER MEMORY BOUNDARY");
		return 1;
	}
	else { // bp address
		bp_address(bpLinkHead_ptr, addr);
	}

	return 0;
}

void linking_loader_print_load_map(int prog_len) {
	printf("control\t\tsymbol\t\taddress\t\tlength\n");
	printf("section\t\tname\n");
	printf("----------------------------------------------------------\n");




	printf("----------------------------------------------------------\n");
	printf("\t\t\t\ttotal length\t%04X\n", prog_len);
}

void bp_clear(struct bpLink ** bpLinkHead_ptr) {
	struct bpLink * link = *bpLinkHead_ptr,
				  * temp = NULL;

	while (link) {
		temp = link;
		link = link->next;

		free(temp);
	}
	*bpLinkHead_ptr = NULL;

	printf("[ok] clear all breakpoints\n");
}

void bp_address(struct bpLink ** bpLinkHead_ptr, int addr) {
	struct bpLink * link = *bpLinkHead_ptr,
				  * new = NULL;

	new = (struct bpLink *) calloc (1, sizeof(struct bpLink));
	new->bpLineNum = addr;

	if (link) {
		while (link->bpLineNum < addr  && link->next) {
			link = link->next;
		}
		
		// there is a node whose bpLineNum is equal to addr
		if (link->bpLineNum == addr) {
			free(new);
		}
		// link to head
		else if (link->next) {
			new->next = link->next;
			link->next = new;
		}
		else { // link->next == NULL
			link->next = new;
		}
	}
	else { // bpLinkHead_ptr = NULL(= link)
		*bpLinkHead_ptr = new;
	}

	printf("[ok] create breakpoint %04X\n", addr);
}



