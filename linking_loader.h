#ifndef __linking_loader__
#define __linking_loader__


// define function 
#define isOverMemoryBoundary(addr) (addr < 0 || addr > 0xFFFFF)
#define getRecoredType(str, record_type) sscanf(str, "%c", &record_type)
#define USE_NUM_OPCODE 20

#define check_def printf("addr : %06X/ num_half_byte : %d/run_param_set->reg[2] : %04X\n", addr, num_half_byte, run_param_set->reg[2])

#include "20141570.h"
#include "linking_loader.h"

struct __extern_symbol {
	struct __extern_symbol * next;
	char * symbol;
	int address;
} ;

typedef struct __ESTAB {
	int length;
	int address;
	char * control_section_name;
	struct __extern_symbol * extern_symbol;
} ESTAB;

struct RUN_PARAM {
	int break_flag;
	int progaddr;
	int EXECADDR;
	int reg[10];
	// /A : 0/X : 1/L : 2/B : 3/S : 4/T : 5/PC : 8/SW : 9
};

int linking_loader_main (int num_command, const char * inputStr);

/* command function */
int command_progaddr (const char * inputStr, int * error_flag);
int command_loader (const char * inputStr, int progaddr, int * EXECADDR);
int command_run (struct RUN_PARAM * run_param_set, const unsigned char break_point_addr[]);
int command_bp (unsigned char break_point_addr[], const char * inputStr);


// return error_flag
int linking_loader_pass1(int progaddr, int argc, char *object_filename[], ESTAB extern_symbol_table[]);
int linking_loader_pass2 (int progaddr, int argc, char * object_filename[], ESTAB extern_symbol_table[], int * EXECADDR);

int linking_loader_search_estab_symbol (ESTAB extern_symbol_table[], int argc, char * str, int * address);
void linking_loader_enter_symbol (struct __extern_symbol **extern_symbol, char * str, int address);
void linking_loader_print_load_map(int argc, ESTAB extern_symbol_table[]);

int linking_loader_search_control_section_name(const char * str, int argc, ESTAB extern_symbol_table[]);

int linking_loader_fetch_objcode_from_memory (int addr, int num_half_byte);
void linking_loader_load_memory (int addr, int num_half_byte, int objcode);

void linking_loader_deallocate_ESTAB (ESTAB extern_symbol_table[], int argc);

void bp_clear(unsigned char break_point_addr[]);
void bp_address(unsigned char break_point_addr[], int addr);

void print_register_set(struct RUN_PARAM run_param_set);
int run_get_addr(int objcode, int opcode_format, struct RUN_PARAM * run_param_set);

#endif
