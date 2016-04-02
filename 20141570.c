#include "20141570.h"

int main () {
	FILE * fp = fopen("opcode.txt","r");
	char input_str[MAX_LEN_INPUT],
		 command[MAX_LEN_COMMAND],
		 argv[3][MAX_LEN_INPUT],
		 *mnemonic = (char *) calloc(LEN_MNEMONIC, sizeof(char));
	int i, j,
		error_flag, comma_flag,
		idx_input_str, idx_argv,
		opcode, format;
	hist_list * new_hist_elem = NULL;
	history_head = (hist_list *) calloc(1, sizeof(hist_list));

	
	/* make opcode table */
	for (i = 0; i < 20; i++) {
		table_head[i] = (op_list *) calloc(1, sizeof(op_list));
	}

	// opcode에 관한 정보를 받고 linked list에 저장한다.
	i = 0;
	while (i < NUM_OF_OPCODES) {
		fscanf(fp, "%02X %s %d/%*s\n", &opcode, mnemonic, &format);
		// TODO : opcode table에서 linking 시켜 op_list 를 굳이 새로 만들지 않게 할 것 && 이 부분도 함수로 moduling 할 것 && trash -> format && mnemonic 바로 받는 것도 생각해 보기
		make_linking_table(&table_head[hash_func(mnemonic)], opcode, mnemonic, format);
		opcode_table[i].opcode = opcode;
		opcode_table[i].format = format;
		strcpy(opcode_table[i].mnemonic, mnemonic);
		i ++;
	}


	while (1) {
		/**************************************/
		/********** initialization ************/
		/**************************************/
		error_flag = 0;
		comma_flag = 0;
		idx_input_str = 0;
		for (i = 0; i < MAX_LEN_COMMAND; i++) { command[i] = 0; }
		for (i = 0; i < 3; i++) { for (j = 0; j < MAX_LEN_INPUT; j++) { argv[i][j] = 0; } }
		for (i = 0; i < MAX_LEN_INPUT; i++) { input_str[i] = 0; }
		/********** initialization end ********/

		printf("sicsim> ");

		/**************************************/
		/****** input string handling *********/
		/**************************************/
		gets(input_str);


		/** get command name **/
		while (input_str[idx_input_str] == ' ') { idx_input_str ++; } /* ignore front space */
		
		// input consists of space, enter
		if (!input_str[idx_input_str]) {
			continue;
		}

		idx_argv = 0;
		while ((input_str[idx_input_str] != ' ') && (input_str[idx_input_str] != 0)) {
			command[idx_argv++] = input_str[idx_input_str++];
		}
		command[idx_argv] = 0;


		while (input_str[idx_input_str] == ' ' || 
				input_str[idx_input_str] == '\t' ||
				input_str[idx_input_str] == '\n') {
			idx_input_str++;
		}

		/* ---------- linking loader check and execute command ---------- */
		if (!strcmp(command, "progaddr")) {
			linking_loader_main(1 input_str + idx_input_str);
		}
		else if (!strcmp(command, "loader")) {
			linking_loader_main(2 input_str + idx_input_str);
		}
		else if (!strcmp(command, "run")) {
			linking_loader_main(3, input_str + idx_input_str);
		}
		else if (!strcmp(command, "bp")) {
			linking_loader_main(4, input_str + idx_input_str);
		}
		/* ---------- linking loader check end --------- */

		/* get parameter name */
		for (i = 0; i < 3 ;i++) {
			comma_flag = 0;
			/** i th(second, third, fourth) argument value **/
			if (input_str[idx_input_str]) { // not end
				/* ignore front space and counting comma */
				while (input_str[idx_input_str] == ' ' || input_str[idx_input_str] == ',') {
					if (input_str[idx_input_str] == ',') {
						comma_flag++;
					}
					idx_input_str ++;
				}
				error_flag = error_check_comma(i, comma_flag);

				idx_argv = 0;
				while (input_str[idx_input_str] != ' ' && (input_str[idx_input_str] != 0) && (input_str[idx_input_str] != ',') && input_str[idx_input_str] != '\n') { // except 0(end of string), ",", space
					argv[i][idx_argv++] = input_str[idx_input_str++];
				}
				argv[i][idx_argv] = 0;
			}
		}

		/* error checking */
		if (error_flag) {
			SEND_ERROR_MESSAGE("COMMA");
			continue;
		}
		else if(error_check_moreargv(input_str, idx_input_str)) {
			SEND_ERROR_MESSAGE("OVERFULL ARGV");
			continue;
		}

		/****** input string handling end ********/


		/*****************************************/
		/*********** execute command *************/
		/*****************************************/
		if (!strcmp(command, "help") || !strcmp(command, "h")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			command_help();
		}
		else if (!strcmp(command, "dir") || !strcmp(command, "d")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			command_dir();
		}
		else if (!strcmp(command, "quit") || !strcmp(command, "q")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			break;
		}
		else if (!strcmp(command, "history") || !strcmp(command, "hi")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			// function check
		}

		else if (!strcmp(command, "dump") || !strcmp(command, "du")) {
			// exception handling - boundary error and existing thrid argv
			int start = strtoi(argv[0], &error_flag, 16),
				end = strtoi(argv[1], &error_flag, 16);

			/* error handling */
			if (error_flag) { // does not integer
				SEND_ERROR_MESSAGE("NOT INTEGER");
				continue;
			}
			if ((start < 0) || (end < 0) || (start > 0xFFFFF) || (argv[1][0] && (start > end)) || start > 0xFFFFF || end > 0xFFFFF) { // boundary error
				SEND_ERROR_MESSAGE("BOUNDARY ERROR");
				continue;
			}
			if (argv[2][0]) { // argv[2] exists
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}

			// params don't exist
			if (!argv[0][0]) {
				start = -1;
			}
			else if (!argv[1][0]) {
				end = -1;
			}

			command_dump(start, end);
		}

		else if (!strcmp(command, "edit") || !strcmp(command, "e")) {
			int addr = strtoi(argv[0], &error_flag, 16),
				val = strtoi(argv[1], &error_flag, 16);

			/* error handling */
			if (error_flag) { // does not integer
				SEND_ERROR_MESSAGE("NOT INTEGER");
				continue;
			}
			if (addr < 0x0 || addr > 0xFFFFF || val < 0 || val > 0xFF) { // boundary error
				SEND_ERROR_MESSAGE("BOUNDARY ERROR");
				continue;
			}
			if (!argv[0][0] || !argv[1][0] || argv[2][0]) { // argv[0] doesn't exist || argv[1] doesn't exist || argv[2] exist
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}

			command_edit(addr, val);
		}
		else if (!strcmp(command, "fill") || !strcmp(command, "f")) {
			int start = strtoi(argv[0], &error_flag, 16),
				end = strtoi(argv[1], &error_flag, 16),
				val = strtoi(argv[2], &error_flag, 16);
			
			/* error handling */
			if (error_flag) { // does not integer
				SEND_ERROR_MESSAGE("NOT INTEGER");
				continue;
			}
			if ((start < 0) || (end < 0) || (start > 0xFFFFF)|| (argv[1][0] && (start > end))  || end > 0xFFFFF || val < 0 || val > 0xFF) { // boundary error
				SEND_ERROR_MESSAGE("BOUNDARY ERROR");
				continue;
			}
			if (!argv[0][0] || !argv[1][0] || !argv[2][0]) { // argv[0] doesn't exist || argv[1] doesn't exist || argv[2] exist
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
		
			command_fill(start, end, val);
		}
		else if (!strcmp(command, "reset")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			command_reset();
		}
		else if (!strcmp(command, "opcode")) {
			char input_mnem[LEN_MNEMONIC];
			// argv[0] exist - error
			if (!argv[0][0] || argv[1][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}

			strcpy(input_mnem, argv[0]);
			command_opcode(input_mnem, &error_flag);
			if (error_flag == -1) {
				SEND_ERROR_MESSAGE("THERE IS NO MATCHING MNEMONIC");
				continue;
			}
		}
		else if (!strcmp(command, "opcodelist")) {
			// argv[0] exist - error
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			command_opcodelist();
		}
		else if (!strcmp(command, "assemble")) {
			if (argv[1][0] || !argv[0][0]) { // argv[1][0] != 0 implies that second argument exists and argv[1][0] == 0 implies that first argument does not exist.
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			// TODO : Make a function deallocating a symbol_table
			if (!symbol_table) {
				// free symbol_table
				// FATAL it needs to be fixed.
				symbol_table = NULL;
			}

			if (strcmp(argv[0] + strlen(argv[0]) - 4, ".asm")) {
				SEND_ERROR_MESSAGE("THIS IS NOT .asm file");
			}

			command_assemble(argv[0], &error_flag);

			if (error_flag) {
				continue;
			}
		}
		else if (!strcmp(command, "type")) {
			if (argv[1][0] || !argv[0][0]) { // argv[1][0] != 0 implies that second argument exists and argv[1][0] == 0 implies that first argument does not exist.
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}

			command_type(argv[0], &error_flag);

			if (error_flag) {
				// error is occured
				continue;
			}
		}
		else if (!strcmp(command, "symbol")) {
			// XXX : error check!
			if (argv[0][0]) {
				SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH THIS COMMAND");
				continue;
			}
			if (!complete_table) {
				SEND_ERROR_MESSAGE("THERE IS NO SYMBOL TABLE");
				continue;
			}
			command_symbol();
		}

		else {
			SEND_ERROR_MESSAGE("\"THIS COMMAND DOES NOT EXIST \"");
			continue;
		}

		hist_list * tmp_hist;

		// store in hist_list(linked list for history has command lines of history
		new_hist_elem = (hist_list *) calloc(1, sizeof(hist_list));
		strcpy(new_hist_elem->command_line, input_str);
		tmp_hist = history_head;
		while (tmp_hist->next) { tmp_hist = tmp_hist->next; }
		tmp_hist->next = new_hist_elem;
		if (!strcmp(command, "history") || !strcmp(command, "hi")) {
			command_history();
		}
	}

	fclose(fp);
	// deallocating memory
	deallocate_opcode();
	deallocate_history();

	return 0;
}
