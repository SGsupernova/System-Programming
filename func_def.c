#include "20141570.h"
/*****************************/
/* function for help command */
/*****************************/
// TODO : 각 한 줄씩 printf할 것
void command_help(void) {
	printf("h[elp]\nd[ir]\nq[uit]\nhi[story]\ndu[mp] [start, end]\ne[dit] address, value\nf[ill] start, end, value\nreset\nopcode mnemonic\nopcodelist\nassemble filename\ntype filename\nsymbol\n");
}


/* memory print for dump */
void memory_print(int start_addr, int end_addr) {
	int i = 0, j = 0;

	int st_addr_column = start_addr/16,
		end_addr_column = end_addr/16 + 1, // <= end_addr_column + 1
		current_addr_column = start_addr - start_addr % 16, // point current line(column)
		tmp;

	for (i = st_addr_column; i < end_addr_column; i++, current_addr_column += 16) {
		printf("%05x " , current_addr_column); // left column(ADDRESS)

		/* print each memory */
		for (j = 0; j < 16; j ++) {
			if (start_addr <= current_addr_column + j && current_addr_column + j <= end_addr) {
				printf(" %02X", memory[current_addr_column + j]);
			}
			else {
				printf("   ");
			}
		}

		/* delimiter between memory and ASCII column */
		printf(" ; ");

		/* print ASCII column */
		for (j = 0; j < 16; j++) {
			tmp = current_addr_column + j;
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

void command_assemble(const char * filename, int * error_flag) {
	FILE * fpOrigin = fopen(filename, "r");
	int prog_len = 0;

	// TODO : check filename is .asm and we turn over a parameter whose .asm is removed 

	if (!fpOrigin) { // there is no file whose filename is same as the parameter filename.
		SEND_ERROR_MESSAGE("THERE IS NO SUCH FILE");
		*error_flag = 1;
		return ;
	}

	// get intermediate file through Algorithm for Pass1 of Assembler
	*error_flag = assemblePass1(fpOrigin, &prog_len);
	fclose(fpOrigin);
	if (*error_flag) {
		return ;
	}

	// get list file, object file, and 
	if (*error_flag = assemblePass2(filename, prog_len)) {
		return ;
	}
}

/* return error_flag */
void command_type(const char * filename, int * error_flag) {
	int find_flag = 0;
	DIR * dp = NULL;
	struct dirent *entry = NULL;
	struct stat sb;

	dp = opendir(".");

	while ((entry = readdir(dp)) != NULL) {
		if (!strcmp(entry->d_name, filename)) {
			find_flag = 1;
			break;
		}
	}

	if (!find_flag) { // there is no file whose filename is same as the parameter filename.
		SEND_ERROR_MESSAGE("THERE IS NO SUCH FILE");
		*error_flag = 1;
		return ;
	}

	FILE * fp = fopen(filename, "r");
	char str[50];
	while (fgets(str, 50, fp)) {
		fputs(str, stdout);
	}

	fclose(fp);
	closedir(dp);
}

void command_symbol(void) {

	return ;
}

// return 1 means that a error occurs
int assemblePass1(FILE * fpOrigin, int * prog_len) {
	FILE * fpInter = fopen(INTERMEDIATE_FILENAME, "w");
	int start_addr, LOCCTR = 0, line_num = 5, bogus = 0,
		operand = 0, format = 0, error_flag = 0, cur_line_error_flag = 0;
	char fileInputStr[150],
		 *mnemonic = NULL, *symbol = NULL,
		 exceptCommentStr[150] = {0,}; // TODO : mnemonic , symbol 등이 필요 없을 가능성이 높음, 추후에 보고 고칠 것.
	symbMnemOper infoSetFromStr;

	initFetchedInfoFromStr(&infoSetFromStr);

	/* begin pass1 */
	while (1) {
		fgets(fileInputStr, 150, fpOrigin); // read input lines
		if (!isCommentLine(fileInputStr)) { // this line is not a comment line
			break;
		}

		fprintf(fpInter, "%04X\t%s\n", LOCCTR, fileInputStr); // write listing line
		line_num += 5;
	}
	fetch_info_from_str(fileInputStr, &infoSetFromStr);

	// if OPCODE = 'START'
	if (!strcmp(infoSetFromStr.mnemonic, "START")) {
		// XXX : 이 부분 fetch 함수를 이용함(수정 완료)
		fetch_info_from_str(fileInputStr, &infoSetFromStr);
		LOCCTR = start_addr = strtoi(infoSetFromStr.operand, &error_flag, 16);
		if (start_addr < 0 || error_flag == 1) { // ERROR : start_addr can not be negative
			SEND_ERROR_MESSAGE("OPERAND FORMAT IS A ERROR");
			SEND_ERROR_LINE(line_num);
			error_flag = 1;
		}
		fprintf(fpInter, "%04X\t%s\n", LOCCTR, fileInputStr); // write listing line
		fgets(fileInputStr, 150, fpOrigin);	// read next input line
		line_num += 5;
	}
	else {
		LOCCTR = start_addr = 0;
	}

	while (infoSetFromStr.mnemonic == NULL || strcmp(infoSetFromStr.mnemonic, "END")) {
		if (!isCommentLine(fileInputStr)) { // This is not a comment line.
			fetch_info_from_str(fileInputStr, &infoSetFromStr);

			// XXX : make a function searching SYMTAB(Complete)
			// search SYMTAB for LABEL & there is same LABEL
			if (searchSYMTAB(infoSetFromStr.symbol, &bogus)) {
				SEND_ERROR_MESSAGE("DUPLICATE SYMBOLY"); // duplicate symbol ERROR  
				cur_line_error_flag = 1; // error(FOUND same symbol) // XXX : return 1 에서 error_flag setting
				// TODO : =1 보다 | 1(bit operation)을 하는 것이 속도 향상 면에서 더 좋아 보임
			}
			else {
				insert2SYMTAB(symbol, LOCCTR);
			}
			// TODO : 나중에 다시 sp 짤 때 operand 안들어오는 경우 제외 시키기
			// search OPTAB for OPCODE
			if ((format = format_mnem(table_head[hash_func(mnemonic)], mnemonic)) != -1) { // OPCODE FOUND
				LOCCTR += format; // add {instruction length} to LOCCTR
				if (mnemonic[0] == '+') {
					LOCCTR ++;
				}
			}
			else if (!strcmp(mnemonic, "WORD")) {
				LOCCTR += 3;
			}
			else if (!strcmp(mnemonic, "RESW")) {
				operand = strtoi(infoSetFromStr.operand, &error_flag, 10);
				if (error_flag) {
					SEND_ERROR_MESSAGE("RESW'S OPERAND");
					cur_line_error_flag = 1;
				}
				LOCCTR += 3 * operand;
			}
			else if (!strcmp(mnemonic, "RESB")) {
				operand = strtoi(infoSetFromStr.operand, &error_flag, 10);

				if (error_flag) {
					SEND_ERROR_MESSAGE("RESB'S OPERAND");
					cur_line_error_flag = 1;
				}
				LOCCTR += operand;
			}
			else if (!strcmp(mnemonic, "BYTE")) {
				operand = analyseByte(infoSetFromStr.operand);
				if (operand == -1) {
					SEND_ERROR_MESSAGE("BYTE'S OPERAND NOT MATCHING");
					cur_line_error_flag = 1;
				}
				LOCCTR += operand;
			}
			else if (!strcmp(mnemonic, "BASE")) {}
			else { // invalid operation code
				SEND_ERROR_MESSAGE("INVALID OPERATION CODE");
				cur_line_error_flag = 1;
			}

			error_flag = error_flag || cur_line_error_flag;
			cur_line_error_flag = 0;

			if (cur_line_error_flag) {
				SEND_ERROR_LINE(line_num);
			}

			// write line to intermediate file
			if (!isCommentLine(fileInputStr)) {
				fprintf(fpInter,"%04X\t%s", LOCCTR, fileInputStr);
			}
			else {
				fprintf(fpInter,"\t\t%s", fileInputStr);
			}

			fgets(fileInputStr, 150, fpOrigin); // read next input line
			line_num += 5;

			initFetchedInfoFromStr(&infoSetFromStr);
			fetch_info_from_str(fileInputStr, &infoSetFromStr);
		}
	}

	// write last line to intermediate file
	fprintf(fpInter,"%04X\t%s", LOCCTR, fileInputStr);

	// save (LOCCTR - starting address) as program length
	*prog_len = LOCCTR - start_addr;

	fclose(fpInter);

	return error_flag;
}

int assemblePass2(const char * filename, int prog_len) {
	FILE * fpInter = fopen(INTERMEDIATE_FILENAME, "r"),
		 * fpLst, * fpObj;
	int operand_addr = 0, LOCCTR = 0, disp = 0, cur_line_LOCCTR = 0,
		line_num = 5, opcodeOfMnemonic = 0, len_mnem = 0, i = 0,
		foramt = 0, error_flag = 0, cur_line_error_flag = 0, start_LOCCTR = 0, first_LOCCTR = 0;
	int op_temp, temp,hex;
	char fileInputStr[150], 
		 * mnemonic = NULL, *symbol = NULL,
		 exceptCommentStr[150] = {0,}; // TODO : mnemonic , symbol 등이 필요 없을 가능성이 높음, 추후에 보고 고칠 것.
	char * filenameLst = NULL, *filenameObj = NULL;
	char * textRecord = NULL,
		 objectcode[LEN_OBJCODE] = {0,},
		 operand[2][LEN_OPERAND] = {0,};
	symbMnemOper infoSetFromStr;
	struct reg registerSet;

	// initialization
	initRegister(&registerSet);
	textRecord = (char *) calloc (LEN_TEXT_RECORD, sizeof(char));
	initFetchedInfoFromStr(&infoSetFromStr);

	filenameLst = strdup(filename);
	filenameObj = strdup(filename);
	strcat(filenameLst, ".lst");
	strcat(filenameObj, ".obj");

	fpLst = fopen(filenameLst, "w");
	fpObj = fopen(filenameObj, "w");

	free(filenameLst);
	free(filenameObj);

	/* begin pass2 */
	while (1) {
		fgets(fileInputStr, 150, fpInter); // read input lines
		if (!isCommentLine(fileInputStr)) { // this line is not a comment line
			break;
		}

		fprintf(fpLst, "%03d%04X\t%s\n", line_num, LOCCTR, fileInputStr); // write listing line
		line_num += 5;
	}

	// NOTE : In process "PASS2", we must use form like below
	fetch_info_from_str(fileInputStr + START_RAW_STR_PASS2, &infoSetFromStr);

	// if OPCODE = 'START'
	if (!strcmp(infoSetFromStr.mnemonic, "START")) {
		fprintf(fpLst, "%03d\t%s", line_num, fileInputStr); // write listing line
		fgets(fileInputStr + START_RAW_STR_PASS2, 150, fpInter);	// read next input line
		line_num += 5;
	}

	// first_LOCCTR is address of first executable instruction in object program(hex)
	first_LOCCTR = start_LOCCTR = LOCCTR = getLOCCTR(fileInputStr);
	// write Header record to object program
	fprintf(fpObj, "H%-6s%6X%6X\n", infoSetFromStr.symbol, LOCCTR, prog_len);

	// initialize first Text record
	initObjectCode(objectcode);
	initTextRecord(textRecord);

	while (infoSetFromStr.mnemonic == NULL || strcmp(infoSetFromStr.mnemonic, "END")) {
		if (!isCommentLine(fileInputStr)) { // This is not a comment line.
			fetch_info_from_str(fileInputStr + START_RAW_STR_PASS2, &infoSetFromStr);

			if (opcode_mnem(table_head[hash_func(infoSetFromStr.mnemonic)], infoSetFromStr.mnemonic) != -1) {
				// there is a symbol in OPERAND field 
				if (searchSYMTAB(infoSetFromStr.operand, &LOCCTR)) {
					// store symbol value as operand address
					operand_addr = LOCCTR;
				}
				else {
					// store 0 as operand address
					operand_addr = 0;
				}
				// assemble the object code instruction
				opcodeOfMnemonic = opcode_mnem(table_head[hash_func(infoSetFromStr.mnemonic)], infoSetFromStr.mnemonic);

				/*

				   if (infoSetFromStr.operand[0] == '@') { // indirect addressing
				   registerSet.n = 1;
				   }
				   else if (infoSetFromStr.operand[0] == '#') { // immediate addressing
				   registerSet.i = 1;
				   }

				   if ()
				// pc relative
				fscanf(fpInter, "%04X", &cur_line_LOCCTR); // pc : cur_line_LOCCTR + 3
				disp = operand_addr - (cur_line_LOCCTR + 3);
				if (-2048 <= operand_addr && operand_addr <= 2047) {


				}
				else if () {


				}
				*/
			}
			// OPCODE = 'BYTE' or 'WORD'
			else if (!strcmp(infoSetFromStr.mnemonic, "BYTE")) {
				// convert constant to object code
				len_mnem = strlen(infoSetFromStr.mnemonic);

				for (i = 0; i < len_mnem; i += 2) {
					temp = infoSetFromStr.mnemonic[i / 2] / 16;
					if (0 <= temp && temp <= 9) {
						objectcode[i] = temp + '0';
					}
					else { // 10 ~ 16 (A ~ F)
						objectcode[i] = temp - 10 + 'A';
					}


					temp = infoSetFromStr.mnemonic[i / 2] % 16;
					if (0 <= temp && temp <= 9) {
						objectcode[i] = temp + '0';
					}
					else { // 10 ~ 16 (A ~ F)
						objectcode[i] = temp - 10 + 'A';
					}
				}
			}
			else if (!strcmp(infoSetFromStr.mnemonic, "WORD")) {			
				op_temp = a2dec(infoSetFromStr.operand, &error_flag);
				if (error_flag) {
					SEND_ERROR_MESSAGE("WORD OPERAND");
					SEND_ERROR_LINE(line_num);
					cur_line_error_flag = 1;
				}
				objectcode[0] = objectcode[1] = '0';

				// make objectcode
				hex = 4096;
				for (i = 2; i < 6; i++) {
					temp = (op_temp / (hex/16)) % hex;

					if (0 <= temp && temp <= 9) {
						objectcode[i] = temp + '0';
					}
					else { // 10 ~ 16 (A ~ F)
						objectcode[i] = temp - 10 + 'A';
					}
				}
			}
			// object code will not fit into current Text record 
			if (strlen(objectcode) + strlen(textRecord) >= LEN_TEXT_RECORD) {
				// write Text record to object program
				fprintf(fpObj, "T%6X%2X%s\n", start_LOCCTR, (unsigned int)strlen(textRecord), textRecord);

				// initialize new Text record
				initTextRecord(textRecord);
				start_LOCCTR = LOCCTR;
			}
			// add object code to Text record
			strcat(textRecord, objectcode);
		}
		// write listing line
		fprintf(fpLst, "%3d\t%s\t%s\n", line_num, fileInputStr, objectcode);

		// read next input line
		fgets(fileInputStr, 150, fpInter); // read input lines
		line_num += 5;

		//initialization
		initRegister(&registerSet);
		initObjectCode(objectcode);
		for (i = 0; i < LEN_OPERAND; i++) {  // initialize operands
			operand[0][i] = operand[1][i] = 0;
		}
	}

	// write last Text record to object program
	fprintf(fpObj, "T%6X %2X %s \n", start_LOCCTR, (unsigned int)strlen(textRecord), textRecord);

	// write End record to object program
	fprintf(fpObj, "E%6X", first_LOCCTR);

	// write last listing line
	fprintf(fpLst, "%3d\t%s", line_num, fileInputStr);






	// TODO : DEALLOCATE
	fclose(fpLst);
	fclose(fpObj);
	fclose(fpInter);

	return error_flag;
}

// Returning : -1 implies a error occurs
//				0 implies BYTE C' '
//				otherwise return the number X' '
int analyseByte (const char * strBYTE, int * byteLength) {
	int i = 0, j = 0, length = 0, num = 0;

	if (strBYTE[1] != '\'') {
		return -1;
	}
	if (strBYTE[0] == 'X') {
		for (i = 2; ; i += 2, length ++) {
			for (j = 0; j < 2; j++) {
				if ('a' <= strBYTE[i + j] && strBYTE[i + j] <= 'z') {
					num *= 16;
					num += strBYTE[i + j] - 'a';
				}
				else if ('A' <= strBYTE[i + j] && strBYTE[i + j] <= 'Z') {					
					num *= 16;
					num += strBYTE[i + j] - 'A';
				}
				else if ('0' <= strBYTE[i + j] && strBYTE[i + j] <= '9') {
					num *= 16;
					num += strBYTE[i + j] - '0';
				}
				else if (strBYTE[i + j] == '\'') {
					if (j) { // ERROR : UNMATCHED HEX PAIR. EX) X'1FF'
						SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH BYTE DIRECTIVE");
						*byteLength = -1;
						return -1;
					}
					else {
						*byteLength = length;
						return num;
					}
				}
				else { // ERROR : FORMAT UNMATCHED. ex) X'1234 ,that is, there is no final '\''
					SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH BYTE DIRECTIVE");
					return -1;
				}
			}
		}
	}
	else if (strBYTE[0] == 'C') {
		for (i = 2; (strBYTE[i] != 0) && (strBYTE[i] != '\''); i++, length++);
		if (strBYTE[i] == '\'') {
			for (; (strBYTE[i] == ' ') || (strBYTE[i] == '\t') || (strBYTE[i] == '\n'); i++);
			if (!strBYTE[i]) {
				*byteLength = length;
				return 0;
			}
			// otherwise, ERROR : there is a another character after '\''
		}
		// otherwise ERROR : NOT MATCHED TO FORMAT that C'characters'
	}

	SEND_ERROR_MESSAGE("FORMAT DOES NOT MATCH BYTE DIRECTIVE");
	*byteLength = -1;


	return -1;
}

// return : If there is the same symbol in symbol table(SYMTAB), return 1.
//			otherwise - 0
int searchSYMTAB(const char * symbol, int * LOCCTR) {
	SYMTAB * search_sym = symbol_table;

	while (search_sym) {
		if (!strcmp(search_sym->symbol, symbol)) { // FOUND
			*LOCCTR = search_sym->LOCCTR;
			return 1;
		}
		search_sym = search_sym->next;
	}
	// NOT FOUND
	return 0;
}

void insert2SYMTAB (char * symbol, int LOCCTR) {
	SYMTAB * new_sym = symbol_table; // TODO : change a global variable to parameter(structure)
	if (new_sym) {
		while (new_sym->next) {
			new_sym = new_sym->next;
		}
		new_sym->next = (SYMTAB *) calloc(1, sizeof(SYMTAB));
		new_sym->next->LOCCTR = LOCCTR;
		new_sym->next->symbol = symbol;
	}
	else {
		new_sym = (SYMTAB *) calloc(1, sizeof(SYMTAB));
		new_sym->LOCCTR = LOCCTR;
		new_sym->symbol = symbol;
	}
}

// XXX : 이름 바꾸기 fetch info form str, 전달 인자는 struct로 관리
void fetch_info_from_str(const char * str, symbMnemOper *infoSetFromStr) {
	char * symbol, * mnemonic, * operand;
	int i = 0;

	// XXX : set symbol length and operand length
	symbol = (char *) calloc(LEN_SYMBOL, sizeof(char));
	mnemonic = (char *) calloc(LEN_MNEMONIC, sizeof(char));
	operand = (char *) calloc(LEN_OPERAND, sizeof(char));

	// XXX : 정규표현식으로 param2는 ., \n나오기 바로 전까지 받기
	sscanf(str, "%s %s %[^.\n]", symbol, mnemonic, operand);

	if (opcode_mnem(table_head[hash_func(mnemonic)], mnemonic) != -1) { // success to seach mnemonic & symbol also exists
		for (i = 0; operand[i] != 0 
				&& operand[i] != ' ' 
				&& operand[i] != '\t' 
				&& operand[i] != '\n'; i++ );
		operand[i] = 0;

		infoSetFromStr->symbol = symbol;
		infoSetFromStr->mnemonic = mnemonic;
		infoSetFromStr->operand = operand;
		return ;
	}

	infoSetFromStr->symbol = NULL;
	if (opcode_mnem(table_head[hash_func(symbol)], symbol) != -1 ) { // first param is mnemonic
		sscanf(str, "%s %[^.\n]", mnemonic, operand);
		for (i = 0; operand[i] != 0 
				&& operand[i] != ' ' 
				&& operand[i] != '\t' 
				&& operand[i] != '\n'; i++ );
		operand[i] = 0;

		infoSetFromStr->mnemonic = mnemonic;
		infoSetFromStr->operand = operand;
		free(symbol);
		return ;
	}

	free(symbol);
	free(mnemonic);
	free(operand);
}

int getLOCCTR(const char * str) {
	int num = 1;
	sscanf(str, "%d", &num);

	return num;
}

void initFetchedInfoFromStr(symbMnemOper * infoSetFromStr) {
	// FATAL : We cannot check whether a pointer is allocated. But I make a pointer to be not NULL when one is allocated
	if (infoSetFromStr->symbol) {
		free(infoSetFromStr->symbol);
		infoSetFromStr->symbol = NULL;
	}
	if (infoSetFromStr->mnemonic) {
		free(infoSetFromStr->mnemonic);
		infoSetFromStr->mnemonic = NULL;
	}
	if (infoSetFromStr->operand) {
		free(infoSetFromStr->operand);
		infoSetFromStr->operand = NULL;
	}
}

void initObjectCode (char * objectcode) {
	int i;

	for (i = 0; i < LEN_OBJCODE; i++) {
		objectcode[i] = 0;
	}
}

void initTextRecord (char * textRecord) {
	int i;

	for (i = 0; i < LEN_TEXT_RECORD; i++) {
		textRecord[i] = 0;
	}
}

void initRegister(struct reg * registerSet) {
	registerSet->n = 0;
	registerSet->i = 0;
	registerSet->b = 0;
	registerSet->x = 0;
	registerSet->p = 0;
	registerSet->e = 0;
}

int hash_func(const char * mnemonic) {
	int i = 0,sum = 0;
	int len;

	if (!mnemonic) {
		return 0;
	}

	len = strlen(mnemonic);
	for(; i< len; i++)
	{
		sum += mnemonic[i];
	}
	return sum % 20;
}


// XXX : 더 효율적으로 돌아가게 생각해보기
void make_linking_table(op_list ** table_addr, int opcode, const char * mnemonic, int format) {
	op_list * new_op = *table_addr;

	while (new_op->next) 
	{
		new_op = new_op->next;
	}

	/* allocate a element of op_list-struct, store opcode and mnemonic and link to head */
	new_op->next = (op_list*) calloc(1, sizeof(op_list));
	new_op->next->opcode = opcode;
	new_op->next->format = format;
	strcpy(new_op->next->mnemonic, mnemonic);
	new_op->next->next = NULL;
}


// TODO : format_mnem 과 opcode_mnem을 합치는 작업 필요
// TODO : table이 입력으로 들어오니 parameter를 넣는 과정이 복잡해지니 이 부분 수정
/* description : convert mnemonic to opcode.
 * return : opcode number , (error - there is no matching mnemonic) : -1
 * */
int opcode_mnem(op_list * table, const char *mnemonic) {
	table = table->next;
	if (!mnemonic) { // error
		return -1;
	}
	while (table) {
		if (!strcmp(table->mnemonic, mnemonic)) {
			break;
		}
		table = table->next;
	}
	if (!table) { // error : there is no matching mnemonic
		return -1;
	}
	return table->opcode;
}

int isCommentLine(const char * str) {
	int i = 0;

	while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') { i++; }

	if (str[i] != '.') { // str is not a comment line
		return 0;
	}
	else { // str is a comment line
		return 1;
	}

}

int format_mnem(op_list * table, const char *mnemonic) {
	table = table->next;
	if (!mnemonic) { // error
		return -1;
	}
	while (table) {
		if (!strcmp(table->mnemonic, mnemonic)) {
			break;
		}
		table = table->next;
	}
	if (!table) { // error : there is no matching mnemonic
		return -1;
	}
	return table->format;
}

/* description : string to integer
 * parameter : string(may be Hex number)
 * return : error_flag = 1 if this string is not integer 
 * */
int strtoi(const char * str, int* error_flag, int exponential) {
	int i = 0, res = 0,
		minus_flag = 1, x_flag = 0;

	if (str == NULL) {
		return -1;
	}

	// case : negative integer, this case is usually error
	if (str[0] == '-') {
		minus_flag = -1;
		i ++;
	}
	// case : 0x123 or 0X123
	else if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		if (exponential != 16) {
			*error_flag = 1;
			return -1;
		}
		i += 2;
	}

	// string to integer
	for (; str[i] != 0; i++) {
		if ('0' <= str[i] && str[i] <= '9') {
			res = res * exponential + str[i] - '0';
		}
		else if ('A' <= str[i] && str[i] <= 'F') {
			res = res * exponential + str[i] - 'A' + 10;
		}
		else if ('a' <= str[i] && str[i] <= 'f') {
			res = res * exponential + str[i] - 'a' + 10;
		}
		else if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
			continue;
		}
		else if (x_flag && str[i] == 0) {
			return res

				;
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

// return 0 : there is no error
//		  0 : there are errors
int TokenizeOperand(const char * operandStr, char ** operand) {
	int i = 0, idx = 0, last_word = 0, comma_flag = 0, order = 0;

	for (; operandStr[i] != 0; i++) {
		if (operandStr[i] != ' ' 
				&& operandStr[i] != ',' 
				&& operandStr[i] != '\t' 
				&& operandStr[i] != '\n'
		   ) 
		{
			last_word = i;
		}
		if (operandStr[i] == ',') {
			comma_flag = 1;
		}
	}

	for (i = 0; operandStr[i] != 0; i++) {
		if (operandStr[i] != ',' && operandStr[i] != ' ') {
			operand[order][idx++] = operandStr[i];
		}
		else if (operandStr[i] == ',') {
			order ++;
			idx = 0;
		}
	}
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

int a2dec (const char * str, int * error_flag) {
	int i = 0, res = 0;

	for (; str[i] != 0; i++) {
		if (('0' <= str[i] && str[i] <= '9')) {
			*error_flag = 1; // error
			return 0;
		}
		res *= 10;
		res += str[i] - '0';
	}

	return res;
}

int error_check_moreargv (const char * input_str, int idx_input_str) {
	while (input_str[idx_input_str] != 0) {
		/* error case : command and argv are overfull*/
		if (!(input_str[idx_input_str] == ' ') || !(input_str[idx_input_str] == '\n')) {
			return 1;
		}
		idx_input_str++;
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
