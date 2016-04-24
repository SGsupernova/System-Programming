#include "20141570.h"
#include "tokenizer.h"
#include "linking_loader.h"
#include <string.h>
#include <stdlib.h>

int linking_loader_main (int num_command, const char * inputStr) {
	static unsigned char *break_point_addr = NULL;
	static struct RUN_PARAM run_param_set;
	static int progaddr = 0;
	static int EXECADDR = 0;


	int temp_int = 0;
	int error_flag = 0;
	int i = 0;

	if (!break_point_addr) {
		break_point_addr = (unsigned char *) calloc (MEMORY_SIZE, sizeof(unsigned char));
	}

	switch (num_command) {
		case 1 : // progaddr command
			temp_int = command_progaddr(inputStr, &error_flag);
			if (error_flag) { break; }
			progaddr = temp_int;
			break;

		case 2: // loader command
			error_flag = command_loader(inputStr, progaddr, &EXECADDR);
			if (!error_flag) {
				run_param_set.break_flag = 0;
				run_param_set.progaddr = progaddr;
				run_param_set.EXECADDR = EXECADDR;

				for (i = 0; i < 10; i++) {
					run_param_set.reg[i] = 0;
				}
				run_param_set.reg[2] = 0xFFFFFF;
			}
			break;

		case 3 : // run command
			command_run(&run_param_set, break_point_addr);
			break;

		case 4 : // break point command
			error_flag = command_bp(break_point_addr, inputStr);
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

int command_loader (const char * inputStr, int progaddr, int * EXECADDR) {
	char ** object_filename = NULL,
		 * extension = NULL;
	int argc = 0, i = 0;
	int prog_len = 0;
	int error_flag = 0;
	ESTAB * extern_symbol_table = NULL;


	// get object filenames
	tokenizer(inputStr, &argc, &object_filename);

	if (!argc) { // there is no argument
		SEND_ERROR_MESSAGE("THERE IS NO ARGUMENT");
		return 1;
	}

	// error check : whether this object filename's extension is .obj
	for (i = 0; i < argc; i++) {
		extension = fetch_filename_extension(object_filename[i]);

		if (!extension || strcmp(extension, "obj")) { // there is no extension \ error
			SEND_ERROR_MESSAGE("(A extension must be .obj) EXTENSION");
			return 1;
		}
	}


	extern_symbol_table = (ESTAB*) calloc(argc, sizeof(ESTAB)); // make extern_symbol_table


	// linking loader pass1, pass2
	if (error_flag = linking_loader_pass1 (progaddr, argc, object_filename, extern_symbol_table)) {
		return 1;
	}
	if (error_flag = linking_loader_pass2 (progaddr, argc, object_filename, extern_symbol_table, EXECADDR)) {
		return 1;
	}

	linking_loader_print_load_map (argc, extern_symbol_table);

	// deallocate
	linking_loader_deallocate_ESTAB (extern_symbol_table, argc);
	tokenize_deallocate_argvs(&object_filename, argc);

	return 0;
}

int command_run (struct RUN_PARAM* run_param_set, const unsigned char break_point_addr[]) {
	static int current_addr = 0, break_addr = 0x100000;
	unsigned int objcode = 0;
	unsigned char opcode = 0;
	int opcode_flag = 0, opcode_format = 0;
	int ni = 0, addr = 0, temp_int = 0, i;
	int r1, r2;
	int num_half_byte, indirect_addr = 0;
	unsigned int L_stack[100] = {0,}, top = -1;

	const int opcode_set[USE_NUM_OPCODE] = {
		0xB4, 0x00, 0xB8, 0x0C, // CLEAR,	LDA,	TIXR,	STA
		0x54, 0x38, 0xDC, 0x30, // STCH,	JLT,	WD,		JEQ
		0x4C, 0x68, 0x3C, 0x10, // RSUB,	LDB,	J,		STX
		0x28, 0xE0, 0xA0, 0xD8, // COMP,	TD,		COMPR,	RD
		0x50, 0x48, 0x14, 0x74	// LDCH,	JSUB,	STL,	LDT
	};
	const int opcode_set_format[USE_NUM_OPCODE] = {
		2, 3, 2, 3,
		3, 3, 3, 3,
		3, 3, 3, 3,
		3, 3, 2, 3,
		3, 3, 3, 3
	};

	if (!(run_param_set->break_flag)) {
		run_param_set->reg[2] = 0xFFFFFF;
		run_param_set->reg[8] = current_addr = run_param_set->EXECADDR;
		L_stack[++top] = 0xFFFFFF;
	}

	while (1) {
		objcode = 0;
//		printf("\n\n-------------------------------------------------------\n");
//		printf("-------------------------------------------------------\n");
		if (run_param_set->reg[2] == run_param_set->reg[2]) {
			print_register_set(*run_param_set);
			return 0;
		}

		if (break_point_addr[current_addr]) {
			if (break_addr != current_addr) {
				break_addr = current_addr;
				run_param_set->break_flag = 0;
				print_register_set(*run_param_set);
//				printf("Stop at checkpoint[%04X]\n", current_addr);
				return 0;
			}
			else {
				break_addr = 0x1000000; // break address initialize
			}
		}

		// fetch1
//		printf("memory[%06X] : %02X\n", current_addr, memory[current_addr]);
		opcode = memory[current_addr] - memory[current_addr] % 4;
		ni = memory[current_addr] % 4;

		opcode_flag = 0;
		for (i = 0; i < 20; i++) {
			if (opcode_set[i] == opcode) {
				opcode_flag = 1;
				opcode_format = opcode_set_format[i];
				break;
			}
		}

//		printf("opcode : %04X\n", opcode);
		if (!opcode_flag) { // invalid opcode
			SEND_ERROR_MESSAGE("(command_run) invalid opcode");
			return 1;
		}

		// make objcode
		objcode = memory[current_addr] * 0x100 + memory[current_addr + 1];

//		printf("objcode : %04X\n", objcode);

		if (opcode_format == 2) {
			num_half_byte = 4;
		}
		else if (opcode_format == 3) {
			objcode = objcode * 0x100 + memory[current_addr + 2];

			num_half_byte = 6;
			if (objcode & 0x001000) { // extension -> format 4
				opcode_format = 4;
				objcode = objcode * 0x100 + memory[current_addr + 3];
				num_half_byte = 8;
			}

		}

		// increasing register PC
		current_addr = run_param_set->reg[8] += opcode_format;

//		printf("later objcode : %06X\n", objcode);

		if (opcode_format == 3 || opcode_format == 4) {
			addr = run_get_addr(objcode, opcode_format, run_param_set) + run_param_set->progaddr;

			objcode = linking_loader_fetch_objcode_from_memory(addr, num_half_byte);
			/***************/
			// address setting
			if (ni == 2) { // indirect
				addr = objcode;
			}
			/***************/
		}

		switch (opcode) {
			// format 2
			case 0xB4 : // CLEAR
				temp_int = objcode & 0x00F0;
				temp_int = temp_int >> 4;
				if (temp_int < 0 || temp_int > 9) {
					SEND_ERROR_MESSAGE ("(run) CLEAR object code(r1)");
					return 1;
				}
				run_param_set->reg[temp_int] = 0;
				break;

			case 0xA0 : // COMPR
				r1 = objcode & 0x00F0;
				r2 = objcode & 0x000F;

				r1 = r1 >> 4;

				if (r1 < 0 || r1 > 9) {
					SEND_ERROR_MESSAGE ("(run) COMPR object code(r1)");
					return 1;
				}
				if (r2 < 0 || r2 > 9) {
					SEND_ERROR_MESSAGE ("(run) COMPR object code(r2)");
					return 1;
				}

				if (r1 < r2) {
					run_param_set->reg[9] = -1;
				}
				else if (r1 == r2) {
					run_param_set->reg[9] = 0;
				}
				else {
					run_param_set->reg[9] = 1;
				}
				break;
			case 0xB8 : // TIXR
				run_param_set->reg[1]++;

				if (r1 < r2) {
					run_param_set->reg[9] = -1;
				}
				else if (r1 == r2) {
					run_param_set->reg[9] = 0;
				}
				else {
					run_param_set->reg[9] = 1;
				}
				break;


				// format 3/4
				// load
			case 0x00 : // LDA
				run_param_set->reg[0] = run_get_addr (objcode, opcode_format, run_param_set);
				break;

			case 0x68 : // LDB
				run_param_set->reg[3] = run_get_addr(objcode, opcode_format, run_param_set);
				break;

			case 0x50 : // LDCH
				run_param_set->reg[0] = run_param_set->reg[0] & 0xFFFF00;
				run_param_set->reg[0] += run_get_addr(objcode, opcode_format, run_param_set) & 0x0000FF;
				break;
			case 0x74 : // LDT
				run_param_set->reg[5] = run_get_addr(objcode, opcode_format, run_param_set);
				break;

				// store
			case 0x0C : // STA
				linking_loader_load_memory(addr, num_half_byte, run_param_set->reg[0]);
				break;
			case 0x54 : // STCH
				linking_loader_load_memory(addr, num_half_byte, run_param_set->reg[0] & 0x0000FF);
				break;
			case 0x10 : // STX
				linking_loader_load_memory(addr, num_half_byte, run_param_set->reg[1]);
				break;
			case 0x14 : // STL
				linking_loader_load_memory(addr, num_half_byte, run_param_set->reg[2]);
				break;



			case 0x38 : // JLT
				if (run_param_set->reg[9] < 0) {
					run_param_set->reg[8] = addr;
				}
				break;
			case 0x30 : // JEQ
				if (run_param_set->reg[9] == 0) {
					run_param_set->reg[8] = addr;
				}
				break;

			case 0x3C : // J
				run_param_set->reg[8] = addr;
				break;
			case 0x28 : // COMP
				if (run_param_set->reg[0] < addr) {
					run_param_set->reg[9] = -1;
				}
				else if (run_param_set->reg[0] == addr) {
					run_param_set->reg[9] = 0;
				}
				else if (run_param_set->reg[0] > addr) {
					run_param_set->reg[9] = 1;
				}
				break;

			case 0xE0 : // TD
				run_param_set->reg[9] = -1;
				break;
			case 0xD8 : // RD
				run_param_set->reg[0] = 0;
				break;
			case 0xDC : // WD
				break;

			case 0x48 : // JSUB
				L_stack[++top] = current_addr;
				run_param_set->reg[2] = run_param_set->reg[8];
				current_addr = run_param_set->reg[8] = addr;
				break;
			case 0x4C : // RSUB
				current_addr = run_param_set->reg[8] = run_param_set->reg[2];
				run_param_set->reg[2] = L_stack[--top];
				break;
			default :
				SEND_ERROR_MESSAGE("(run) opcode error");
				break;
		}
/*
		if (run_param_set->reg[8] == run_param_set->reg[2]) {
			break;
		}
*/		
//		run_param_set->reg[8] = current_addr;
	}

	return 0;
}

int command_bp (unsigned char break_point_addr[], const char * inputStr) {
	int argc = 0, addr = 0, error_flag = 0;
	char ** argv = NULL;
	int idx = 0;

	tokenizer(inputStr, &argc, &argv);

	// error
	if (!argc) {
		printf("breakpoint\n");
		printf("----------\n");

		for (idx = 0; idx < MEMORY_SIZE; idx++) {
			if (break_point_addr[idx]) {
				printf("%04X\n", idx);
			}
		}

		return 0;
	}
	else if (argc != 1){
		SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
		return 1;
	}

	// break point address
	addr = strtoi(argv[0], &error_flag, 16);

	if (error_flag) {
		if (argv[0] && !strcmp(argv[0], "clear")) { // bp clear
			bp_clear(break_point_addr);
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
		bp_address(break_point_addr, addr);
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
int linking_loader_pass2 (int progaddr, int argc, char * object_filename[], ESTAB extern_symbol_table[], int * EXECADDR) {
	FILE * object_fp = NULL;

	char record_type = 0, 
		 reference_name[7] = {0,},
		 program_name[7] = {0,};
	char temp_char, operator = 0;
	char fileInputStr[150] = {0,};

	int CSADDR = progaddr,
		CSLTH = 0,
		iter_addr = 0;
	int iter = 0, idx = 0, refer_idx = 0, is_found = 0,
		temp_hex = 0, temp_address;
	int length_objcode = 0, length_str = 0, limit_text = 0;
	int starting_addr = 0, num_half_byte = 0;

	*EXECADDR = progaddr;

	while (iter < argc) { // not end of input
		object_fp = fopen(object_filename[iter], "r");

		// referemce table : to mapping reference numbers and its address
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
				// make reference number table and store a address
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
			sscanf(fileInputStr, "E%06X", EXECADDR);
			(*EXECADDR) += CSADDR;
		}

		// add CSLTH to CSADDR
		CSADDR += CSLTH;

		iter ++;
		fclose(object_fp);
	}

	return 0;
}

// search symbol in external symbol table
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

void bp_clear(unsigned char break_point_addr[]) {
	int i = 0;

	for (i = 0; i < MEMORY_SIZE; i++) {
		break_point_addr[i] = 0;
	}

	printf("[ok] clear all breakpoints\n");
}

void bp_address(unsigned char break_point_addr[], int addr) {
	break_point_addr[addr] = 1;

	printf("[ok] create breakpoint %04X\n", addr);
}

int run_get_addr (int objcode, int opcode_format, struct RUN_PARAM * run_param_set) {
	int disp = 0, addr = 0;
	int x = 0, b = 0, p = 0;

	if (opcode_format == 3) {
		x = objcode & 0x008000;
		b = objcode & 0x004000;
		p = objcode & 0x002000;

		disp = objcode & 0xFFF;

		if (p) {
			addr = disp + run_param_set->reg[8];
		}
		else if (b) {
			addr = disp + run_param_set->reg[3];
		}

		if (x) {
			addr += run_param_set->reg[1];
		}
	}
	else if (opcode_format == 4){
		x = objcode & 0x00800000;
		b = objcode & 0x00400000;
		p = objcode & 0x00200000;

		disp = objcode & 0xFFFF;
		addr = disp;
		if (x) {
			addr += run_param_set->reg[1];
		}
	}

	return addr;
}

void print_register_set (struct RUN_PARAM run_param_set) {
	printf("\tA : %06X X : %06X\n", 
			run_param_set.reg[0], 
			run_param_set.reg[1]
		  );
	printf("\tL : %06X PC : %06X\n", 
			run_param_set.reg[2], 
			run_param_set.reg[8]
		  );
	printf("\tB : %06X S : %06X\n",
			run_param_set.reg[3],
			run_param_set.reg[4]
		  );
	printf("\tT : %06X\n", run_param_set.reg[5]);
	printf("End program.\n");
}

