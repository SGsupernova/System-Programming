#include "20141570.h"
/*****************************/
/* function for help command */
/*****************************/
void command_help(void) {
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\n");
}


/* memory print for dump */
void memory_print(int start_addr, int end_addr) {
	int i = 0, j = 0;

	int st_addr_column = start_addr/16,
		end_addr_column = end_addr/16 + 1, // <= end_addr_column + 1
		front_addr_column = start_addr - start_addr % 16, // point current line(column)
		tmp;

	for (i = st_addr_column; i < end_addr_column; i++, front_addr_column += 16) {
		printf("%05x " , front_addr_column); // left column(ADDRESS)

		/* print each memory */
		for (j = 0; j < 16; j ++) {
			if (start_addr <= front_addr_column + j && front_addr_column + j <= end_addr) {
				printf(" %02X", memory[front_addr_column + j]);
			}
			else {
				printf("   ");
			}
		}

		/* delimiter between memory and ASCII column */
		printf(" ; ");

		/* print ASCII column */
		for (j = 0; j < 16; j++) {
			tmp = front_addr_column + j;
			if (!(start_addr <= tmp && tmp <= end_addr) || 
					(memory[tmp] < 0x20 || 0x7E < memory[tmp])) {
				printf(".");
			}
			else {
				printf("%c", memory[tmp]);
			}
		}
		printf("\n");
	}

}


/**********************************/
/* function for directory command */
/**********************************/
void command_dir(void) {
	DIR *dp = NULL;
	struct dirent *entry = NULL;
	struct stat sb;

	dp = opendir(".");

	while((entry = readdir(dp)) != NULL) {
		// except current directory . and prior directory ..
		if (!strcmp(entry->d_name, ".") ||
				!strcmp(entry->d_name, "..")) {
			continue;
		}
		stat(entry->d_name, &sb);

		// the type of this file is directory
		if (entry->d_type == DT_DIR) {
			printf("%20s/", entry->d_name);
		}
		// the type of this file is executable file
		else if ((sb.st_mode & S_IXUSR) ||
				(sb.st_mode & S_IXGRP) ||
				(sb.st_mode & S_IXOTH)
				) {
			printf("%20s*", entry->d_name);
		}
		// the type of this file is regular file
		else{
			printf("%20s", entry->d_name);
		}
	}
	printf("\n");

	closedir(dp);
}


/********************************/
/* function for history command */
/********************************/
// print history list
void command_history(void) {
	hist_list * link = history_head->next;
	int i = 1;
	while (link) {
		printf("%d\t%s\n", i++, link->command_line);
		link = link->next;
	}
}


/*****************************/
/* function for dump command */
/*****************************/
/* if start or end is -1, it does not exist(is not input) */
void command_dump(int start, int end) {
	static int last_addr = 0;

	if (start == -1) { // case : dump
		memory_print(last_addr, ADDR_BOUNDARY(last_addr+159)); // print 160 memory from last_addr
		last_addr += 160;
	}
	else {
		if (end == -1) { // dump start , otherwise dump start, end
			end = ADDR_BOUNDARY(start + 159);
		}
		memory_print(start, end);
		last_addr = end + 1;
	}

	// exception : We scan the last memory
	if (last_addr > 0xFFFFF) {
		last_addr = 0;
	}
}



/*****************************/
/* function for edit command */
/*****************************/
void command_edit(int addr, int val) {
	memory[addr] = val;
}



/*****************************/
/* function for fill command */
/*****************************/
void command_fill(int start, int end, int val) {
	int i;
	end++;
	for (i = start; i < end; i++) {
		memory[i] = val;
	}
}


/*****************************/
/* function for reset command */
/*****************************/
void command_reset(void) {
	int i = 0;

	for (i = 0; i < 0x100000; i++) {
		memory[i] = 0;
	}
}


/********************************/
/* functions for opcode command */
/********************************/
/*	flag = 0 : opcode mnemonic
 *	flag = 1 : opcodelist
 * */
void command_opcode(const char * input_mnem, int * error_flag) {
	int opcode = opcode_mnem(table_head[hash_func(input_mnem)], input_mnem);

	if (opcode == -1) { // Input string does not any matching mnemonic
		*error_flag = opcode;
		return ;
	}

	printf("opcode is %02X\n", opcode);
}


void command_opcodelist(void) {
	int i = 0;
	op_list * tmp = NULL;

	for (i = 0; i < 20; i++) {
		tmp = table_head[i]->next;
		printf("%d : ", i);

		while (tmp) {
			printf("[%s,%d]", tmp->mnemonic, tmp->opcode);
			if (tmp->next) {
				printf(" -> ");
			}
			tmp = tmp->next;
		}
		printf("\n");
	}
}


// TODO : change hash function
int hash_func(const char * mnemonic) {
	return (mnemonic[0] - 'A') % 20;
}


void make_linking_table(op_list ** table_addr, int opcode, const char * mnemonic) {
	op_list * new_op = *table_addr;

	while (new_op->next) {
		new_op = new_op->next;
	}

	/* allocate a element of op_list-struct, store opcode and mnemonic and link to head */
	new_op->next = (op_list*) calloc(1, sizeof(op_list));
	new_op->next->opcode = opcode;
	strcpy(new_op->next->mnemonic, mnemonic);
	new_op->next->next = NULL;
}


/* description : convert mnemonic to opcode.
 * return : opcode number , (error - there is no matching mnemonic) : -1
 * */
int opcode_mnem(op_list * table, const char *mnemonic) {
	table = table->next;
	while (table) {
		if (!strcmp(table->mnemonic, mnemonic)) {
			break;
		}
		table = table->next;
	}
	if (!table) { // error : there is matching mnemonic
		return -1;
	}
	return table->opcode;
}

/* description : string to integer
 * paramater : string(may be Hex number)
 * return : error_flag = 1 if this string is not integer 
 * */
int strtoi(const char * str, int* error_flag) {
	int i = 0, minus_flag = 1, res = 0;

	// case : negative integer
	if (str[0] == '-') {
		minus_flag = -1;
		i ++;
	}
	// case : 0x123 or 0X123
	else if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		i += 2;
	}
	for (; str[i] != 0; i++) {
		if ('0' <= str[i] && str[i] <= '9') {
			res = res * 16 + str[i] - '0';
		}
		else if ('A' <= str[i] && str[i] <= 'F') {
			res = res * 16 + str[i] - 55;
		}
		// error case
		else {
			*error_flag = 1;
			return -1;
		}
		if (res >= 0x100000) {
			res = 0x100000;
		}
	}
	
	return minus_flag * res;
}


/********************************/
/* functions for error handling */
/********************************/

// return : 0 (normal)
//			1 (error)
int error_check_comma (int i, int comma_flag) {
	if (!i && !comma_flag) { // first argv : no comma , normal case
		return 0;
	}
	else if (i && comma_flag == 1) {
		return 0;
	}
	else {	// error case
		return 1;
	}
}


int error_check_moreargv (const char * input_str, int idx_input_str) {
	while (input_str[idx_input_str] != 0) {
		/* error case : command and argv are overfull*/
		if (!(input_str[idx_input_str++] == ' ')) {
			return 1;
		}
	}
	return 0;
}

/*************************************/
/* functions for deallocating memory */
/*************************************/
void deallocate_history (void) {
	hist_list * hist_trace = NULL,
			  * hist_tmp = NULL;
	hist_trace = history_head;
	while (hist_trace) {
		hist_tmp = hist_trace;
		hist_trace = hist_trace->next;
		free(hist_tmp);
	}
}


void deallocate_opcode (void) {
	int i;
	op_list * op_trace = NULL,
			  * op_tmp = NULL;
	for (i = 0; i < 20; i++) {
		op_trace = table_head[i];
		while (op_trace) {
			op_tmp = op_trace;
			op_trace = op_trace->next;
			free(op_tmp);
		}
	}
}
