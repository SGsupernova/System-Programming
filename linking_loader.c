#include "20141570.h"
#include "tokenizer.h"
#include "linking_loader.h"
#include <string.h>
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
			error_flag = command_loader(inputStr, progaddr);
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

int command_loader (const char * inputStr, int progaddr) {
	char ** object_filename = NULL,
		 * extension = NULL;
	int argc = 0, i = 0;
	int prog_len = 0;
	int error_flag = 0;
	ESTAB * extern_symbol_table = NULL;

	tokenizer(inputStr, &argc, &object_filename);

	printf("argc : %d\n", argc);
	
	for (i = 0; i < argc; i++) {
		printf("argv[%d] : %s\n", i, object_filename[i]);
	}

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

	extern_symbol_table = (ESTAB*) calloc(argc, sizeof(ESTAB));

	if (error_flag = linking_loader_pass1 (progaddr, argc, object_filename, extern_symbol_table)) {
		return 1;
	}

	linking_loader_print_load_map (argc, extern_symbol_table);

	if (error_flag = linking_loader_pass2 (progaddr, object_filename)) {
		return 1;
	}

	// all extensions of filenames are .obj

	/*
	linking_loader_print_load_map (prog_len, argc, extern_symbol_table);
	*/
	return 0;
}

int command_run (struct reg regSet) {

	print_register_set(regSet);
	printf("\tEnd program.\n");
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

int linking_loader_pass1 (int progaddr, int argc, char *object_filename[], ESTAB extern_symbol_table[]) {
	FILE * object_fp = NULL;
	char fileInputStr[150] = {0,}, 
		 record_type = 0, program_name[7] = {0,}, *extern_symbol = (char *) calloc (7, sizeof(char));


	int CSADDR = progaddr, CSLTH = 0,
		starting_addr = 0, relative_addr = 0,
		**bogus_double_pointer = NULL;
	int iter = 0;
	int define_record_loc = 0, define_record_length = 0;
	int is_found = 0;
	


	while (iter < argc) {
		object_fp = fopen(object_filename[iter], "r");

		if (object_fp == NULL) {
			SEND_ERROR_MESSAGE ("THERE IS NO SUCH FILE");
			return 1;
		}

		// read next input record
		fgets(fileInputStr, 150, object_fp);
		getRecoredType(fileInputStr, record_type);
		// TODO : Record가 H가 아닌 경우에 대해서 Error handling을 해야한다.

		// set CSLTH to control section length
		sscanf(fileInputStr, "%c%s%06X%06X", &record_type, program_name, &starting_addr, &CSLTH);

		is_found = linking_loader_search_control_section_name(program_name, argc, extern_symbol_table);	// search ESTAB for control section name
		if (is_found == 1) {
			SEND_ERROR_MESSAGE("DUPLICATE EXTERNAL SYMBOL");
			return 1; // error
		}
		else {
			extern_symbol_table[iter].control_section_name = strdup(program_name);
			extern_symbol_table[iter].length				= CSLTH;
			extern_symbol_table[iter].address				= CSADDR;
		}
	

		record_type = 0;
		while (record_type != 'E') {
			// read next input record
			fgets(fileInputStr, 150, object_fp);
			getRecoredType(fileInputStr, record_type);

			if (record_type == 'D') {
				define_record_loc = 1;
				define_record_length = strlen(fileInputStr) - 1;

				while (define_record_loc < define_record_length) { // for each symbol in the record_type
					relative_addr = 0x1000000;
					sscanf(fileInputStr + define_record_loc, "%6s%06X", extern_symbol, &relative_addr);

					if (relative_addr == 0x1000000) {
						SEND_ERROR_MESSAGE("(linking_loader_pass1 / define record) object file");
					}

					//search ESTAB for symbol name
					is_found = linking_loader_search_estab_symbol(extern_symbol_table[iter].extern_symbol, extern_symbol);/* XXX : make a function that searchs symbol name in ESTAB */;
					if (is_found) {
						SEND_ERROR_MESSAGE("(linking_loader_pass1 / define record)DUPLICATE EXTERNAL SYMBOL");
						return 1; // error
					}
					else {
						linking_loader_enter_symbol (&extern_symbol_table[iter].extern_symbol, extern_symbol, relative_addr + CSADDR); // XXX : make a function that enters symbol into ESTAB with value (CSADDR + indicated address)
					}

					define_record_loc += 12;
				}
			}
		}

		// add CSLTH to CSADDR
		CSADDR += CSLTH;


		iter += 1;
	}

	return 0;
}

// need
// progaddr / the number of object_file(argc)
int linking_loader_pass2 (int progaddr, int argc) {
	FILE * object_fp = NULL;

	char fileInputStr[150] = {0,};

	int CSADDR = progaddr,
		EXECARRD = progaddr,
		CSLTH = 0;
	int iter = 0;
	

	while (iter < argc) { // not end of input
		object_fp = fopen(object_filename[iter], "r");

		//read next input record
		fgets(fileInputStr, 150, object_fp);
		getRecoredType(fileInputStr, record_type);

		
		// set CSLTH to control section length
		sscanf(fileInputStr, "%c%s%06X%06X", &record_type, program_name, &starting_addr, &CSLTH);

		record_type = 0;
		while (record)



	
		iter ++;
	}



	return 0;
}

int linking_loader_search_estab_control_section_name (const char * str, int argc, ESTAB extern_symbol_table[]) {
	int i = 0;

	if (!str) {
		return -1;
	}

	for (i = 0; i < argc; i++) {
		if (extern_symbol_table[i].control_section_name && 
				!strcmp(str, extern_symbol_table[i].control_section_name)) {
			return 1;
		}
	}
	
	return 0;
}

int linking_loader_search_estab_symbol (struct __extern_symbol *extern_symbol, char * str) {
	struct __extern_symbol * link = extern_symbol;


	while (link) {
		if (link->symbol && !strcmp(link->symbol, str)) {
			return 1; // found
		}
		link = link->next;
	}
	
	return 0; // not found
}

void linking_loader_enter_symbol (struct __extern_symbol **extern_symbol_ptr, char * str, int address) {
	struct __extern_symbol * new = NULL,
						   * link = *extern_symbol_ptr;

	new = (struct __extern_symbol*) calloc (1, sizeof(struct __extern_symbol));
	new->symbol = strdup(str);
	new->address = address;

	if (link) {
		while (link->next) {
			link = link->next;
		}
		link->next = new;
	}
	else {
		*extern_symbol_ptr = new;
	}
}

void linking_loader_print_load_map(int argc, ESTAB * extern_symbol_table) {
	int i = 0, total_length = 0;

	printf("control\t\tsymbol\t\taddress\t\tlength\n");
	printf("section\t\tname\n");
	printf("----------------------------------------------------------\n");

	for (i = 0; i < argc; i++) {
		printf("%-6s\t\t\t\t%4X\t\t%04X\n",
				extern_symbol_table[i].control_section_name,
				extern_symbol_table[i].address,
				extern_symbol_table[i].length
			  );

		struct __extern_symbol * link = extern_symbol_table[i].extern_symbol;

		while (link) {
			printf("\t\t%-6s\t\t%4X\n",
					link->symbol,
					link->address
					);
			link = link->next;
		}
		
		total_length += extern_symbol_table[i].length;

		if (i+1 != argc) {
			printf("\n");
		}
	}


	printf("----------------------------------------------------------\n");
	printf("\t\t\t\ttotal length\t%04X\n", total_length);
}

// TODO : loader를 다시 수행했을 때 어떤 일이 발생하는지 체크
int linking_loader_search_control_section_name(const char * str, int argc, ESTAB extern_symbol_table[]) {
	int i = 0;

	for (i = 0; i < argc; i++) {
		if (!extern_symbol_table[i].control_section_name) {
			return 0;
		}
		if (!strcmp(extern_symbol_table[i].control_section_name, str)) {
			return 1;
		}
	}

	return 0;
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



