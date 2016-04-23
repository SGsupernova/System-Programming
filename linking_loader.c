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
	
	return error_flag;
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

	if (error_flag = linking_loader_pass2 (progaddr, argc, object_filename, extern_symbol_table)) {
		return 1;
	}

	linking_loader_deallocate_ESTAB (extern_symbol_table, argc);

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
	int iter = 0, is_found = 0;
	int define_record_loc = 0, define_record_length = 0;
	int bogus_int;


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

		if (starting_addr != 0) {
			CSADDR -= starting_addr;
		}

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
					is_found = linking_loader_search_estab_symbol(extern_symbol_table, argc, extern_symbol, &bogus_int);/* XXX : make a function that searchs symbol name in ESTAB */;
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

		if (starting_addr != 0) {
			CSADDR += starting_addr;
		}

		iter += 1;
	}

	free(extern_symbol);

	return 0;
}


// need
// progaddr / the number of object_file(argc)
int linking_loader_pass2 (int progaddr, int argc, char * object_filename[], ESTAB extern_symbol_table[]) {
	FILE * object_fp = NULL;

	char record_type = 0, 
		 reference_name[7] = {0,},
		 program_name[7] = {0,};
	char temp_char, operator = 0;
	char fileInputStr[150] = {0,};

	int CSADDR = progaddr,
		EXECADDR = progaddr,
		CSLTH = 0,
		iter_addr = 0;
	int iter = 0, idx = 0, refer_idx = 0, is_found = 0,
		temp_hex = 0, temp_address;
	int length_objcode = 0, length_str = 0, limit_text = 0;
	int starting_addr = 0, num_half_byte = 0;

	while (iter < argc) { // not end of input
		object_fp = fopen(object_filename[iter], "r");

		int reference_table[257] = {0,}; // 16*16 + 1
		refer_idx = 2;
		reference_table[1] = extern_symbol_table[iter].address;


		//read next input record
		fgets(fileInputStr, 150, object_fp);
		getRecoredType(fileInputStr, record_type);


		// set CSLTH to control section length
		sscanf(fileInputStr, "%c%s%06X%06X", &record_type, program_name, &starting_addr, &CSLTH);


		record_type = 0;
		while (record_type != 'E') {
			// clear fileInputStr
			for (idx = 0; idx < 150; idx ++) {
				fileInputStr[idx] = 0;
			}

			// read next input record
			fgets(fileInputStr, 150, object_fp);
			getRecoredType(fileInputStr, record_type);

			if (record_type == 'T') {
				// CSADDR + specified address
				sscanf(fileInputStr, "T%06X%02X", &iter_addr, &length_objcode);
				iter_addr += CSADDR;
				limit_text = 2 * length_objcode + 9;

				// move object code from record to location
				idx = 9;
				for (; idx < limit_text; iter_addr ++, idx += 2) {
					if (idx > 150) {
						SEND_ERROR_MESSAGE("(linking_loader pass2) length_objcode");
						return 1;
					}
					if (iter_addr > MEMORY_SIZE || iter_addr < 0) {
						SEND_ERROR_MESSAGE("(linking_loader pass2) Memory overflow");
						return 1;
					}

					sscanf(fileInputStr + idx, "%02X", &temp_hex);
					memory[iter_addr] = temp_hex;
				}
			}
			else if (record_type == 'M') {
				sscanf(fileInputStr, "M%06X%02X%c%02X",
						&starting_addr,
						&num_half_byte,
						&operator,
						&idx);
				// add or subtract symbol value at location
				// CSADDR + specified address
				starting_addr += CSADDR;
				temp_address = linking_loader_fetch_objcode_from_memory (starting_addr, num_half_byte);

				if (operator == '+') {
					temp_address += reference_table[idx];
				}
				else if (operator == '-') {
					temp_address -= reference_table[idx];
				}

				linking_loader_load_memory (starting_addr, num_half_byte, temp_address);
			}
			else if (record_type == 'R') {
				length_str = strlen(fileInputStr);
				fileInputStr[length_str--] = 0;

				for(idx = 1; idx < length_str; idx += 8) {
					sscanf(fileInputStr + idx, "%02X%6s", &refer_idx, reference_name);
					is_found = linking_loader_search_estab_symbol(extern_symbol_table, argc, reference_name, &temp_address);

					if (is_found) {
						reference_table[refer_idx] = temp_address;
					}
					else {
						SEND_ERROR_MESSAGE("(linking_loader pass2) Reference record");
						return 1;
					}
				}
			}
		}
		// if an address is specified (in End record) then
		if (record_type == 'E') {
			// set EXECADDR to (CSADDR + specified address)
			sscanf(fileInputStr, "E%06X", &EXECADDR);
			EXECADDR += CSADDR;
		}

		// add CSLTH to CSADDR
		CSADDR += CSLTH;

		iter ++;
		fclose(object_fp);
	}

	return 0;
}

int linking_loader_search_estab_symbol (ESTAB extern_symbol_table[], int argc, char * str, int * address) {
	struct __extern_symbol * link = NULL;
	int i = 0;

	for (i = 0; i < argc; i++) {
		link = extern_symbol_table[i].extern_symbol;
		while (link) {
			if (link->symbol && !strcmp(link->symbol, str)) {
				*address = link->address;
				return 1; // found
			}
			link = link->next;
		}
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


int linking_loader_fetch_objcode_from_memory (int addr, int num_half_byte) {
	int objcode = 0;
	int i = 0, sign = 1;

	// the number of half bytes is odd / special case
	if (num_half_byte % 2 == 1) {
		objcode = memory[addr] % 16;
		num_half_byte --;
		addr ++;
	}

	for (i = 0; i < num_half_byte; i += 2) {
		objcode *= 256;
		objcode += memory[addr];
		addr ++;
	}

	return objcode;
}

void linking_loader_load_memory (int addr, int num_half_byte, int objcode) {
	int i = 0;
	int pow = 1;
	unsigned int unsigned_objcode = objcode;


	for (i = 0; i < num_half_byte; i++) {
		pow *= 16;
	}

	if (num_half_byte % 2 == 1) {
		memory[addr] &= 0x10;
		memory[addr] |= ((unsigned_objcode % pow) / (pow/16));
		pow /= 16;
		num_half_byte --;
		addr ++;
	}


	for (i = 0; i < num_half_byte; i += 2, addr ++, pow /= 256) {
		memory[addr] = ((unsigned_objcode % pow) / (pow / 256));
	}
}

void linking_loader_deallocate_ESTAB (ESTAB extern_symbol_table[], int argc) {
	struct __extern_symbol * symbol_link = NULL,
						   * temp_symbol;
	int i;

	for (i = 0; i < argc; i++) {
		free(extern_symbol_table[i].control_section_name);

		symbol_link = extern_symbol_table[i].extern_symbol;
		
		while (symbol_link) {
			temp_symbol = symbol_link;
			symbol_link = symbol_link->next;

			free(temp_symbol->symbol);
			free (temp_symbol);
		}
	}

	free(extern_symbol_table);
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

