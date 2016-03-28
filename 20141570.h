// TODO : order functions
// TODO : string handling function 중에서 입력이 NULL인 경우 처리 할 것
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

// TODO : 0x100000 -> MEMORY_SIZE

#define START_RAW_STR_PASS2 5
#define MAX_LEN_INPUT 100
#define MAX_LEN_COMMAND 10
#define NUM_OF_OPCODES 58
#define LEN_MNEMONIC 6
#define LEN_SYMBOL 10
#define LEN_OPERAND 64
#define MEMORY_SIZE 0x100000
#define SEND_ERROR_MESSAGE(str) printf("\"%s\" ERROR OCCUR!!\n", str)
#define SEND_ERROR_LINE(number) printf("%d LINE IS ERROR OCCUR\n", number)
#define ADDR_BOUNDARY(addr) ((addr) < 0x100000 ? (addr) : 0xFFFFF)
#define INTERMEDIATE_FILENAME "inter.asm"
#define LEN_TEXT_RECORD 61
#define LEN_OBJCODE 61

// TODO : global variable 묶기
typedef struct _HIST{
	struct _HIST * next;
	char command_line[MAX_LEN_INPUT];
} hist_list;

typedef struct _HASH_LINK{
	struct _HASH_LINK * next;
	int opcode;
	int format;
	char mnemonic[LEN_MNEMONIC];
} op_list;

typedef struct _SYMBOL_TABLE {
	struct _SYMBOL_TABLE * next;
	int LOCCTR;
	char * symbol;
} SYMTAB;

typedef struct _OPTAB {
	int opcode;
	int format;
	char mnemonic[LEN_MNEMONIC];
} OPTAB;

typedef struct _symb {
	char * symbol;
	char * mnemonic;
	char * operand;
} symbMnemOper;

struct reg {
	int n; // indirect addressing
	int i; // immediate addressing
	int x; // index addressing
	int b; // base relative
	int p; // pc relative
	int e; // extended
};

OPTAB opcode_table[NUM_OF_OPCODES];
SYMTAB *symbol_table;
op_list *table_head[20];
hist_list *history_head;
unsigned char memory[MEMORY_SIZE]; // 2^20

void command_help(void);
void command_dir(void);
void command_history(void);
void memory_print(int start_addr, int end_addr);
void command_dump(int start, int end);
void command_edit(int addr, int val);
void command_fill(int start, int end, int val);
void command_reset(void);
void command_opcode(const char * input_mnem, int * error_flag);
void command_opcodelist(void);
void command_assemble(const char * filename, int * error_flag);
void command_type(const char * filename, int * error_flag);
void command_symbol(void);

int getLOCCTR(const char * str);
int hash_func(const char * mnemonic);
void make_linking_table(op_list ** table_addr, int opcode, const char * mnemonic, int format);
int opcode_mnem(op_list * table, const char *mnemonic);
int format_mnem(op_list * table, const char *mnemonic);

int strtoi(const char * str, int* error_flag, int exponential);

int assemblePass1(FILE * fpOrigin, int * prog_len);
// TODO : For using same fetch function used assemblePass1, make a function that find a  location except LOCCTR
int assemblePass2 (const char * filename, int prog_len);

int searchSYMTAB(const char * symbol, int * LOCCTR);
void insert2SYMTAB(char * symbol, int LOCCTR);

void fetch_info_from_str(const char * str, symbMnemOper *infoSetFromStr);
void initFetchedInfoFromStr(symbMnemOper * infoSetFromStr);
void initObjectCode (char * objectcode);
void initTextRecord (char * textRecord);
void initRegister(struct reg * registerSet);

int analyseBYTE(const char * strBYTE, int * byteLength);
int isCommentLine(const char * str);
int isDirective(const char * str);

int TokenizeOperand(const char * operandStr, char ** operand);

void sortSYMTABandPrint();

// TODO : DELETE SYMBOL TABLE FUNCTION
// TODO : DEALLOCATING sth FUNCTIONS 필요함!!

/* error handling */
int error_check_comma (int i,int comma_flag);
int error_check_moreargv (const char * input_str, int idx_input_str);

/* deallocate */
void deallocate_history(void);
void deallocate_opcode(void);

