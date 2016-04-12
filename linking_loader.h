#ifndef __linking_loader__
#define __linking_loader__


// define function 
#define isOverMemoryBoundary(addr) (addr < 0 || addr > 0xFFFFF)
#define getRecoredType(str, record_type) sscanf(str, "%c", &record_type)

#include "20141570.h"
#include "linking_loader.h"

struct __extern_symbol {
	struct _extern_symbol * next;
	char * symbol;
	int address;
} ;

typedef struct __ESTAB {
	int length;
	int address;
	char * control_section_name;
	struct __extern_symbol * extern_symbol;
} ESTAB;


struct bpLink {
	struct bpLink * next;
	int bpLineNum;
};

int linking_loader_main (int num_command, const char * inputStr);

/* command function */
int command_progaddr ();
int command_loader (const char * inputStr, int progaddr);
int command_run (struct reg regSet);
int command_bp (struct bpLink ** bpLinkHead_ptr, const char * inputStr);


// return error_flag
int linking_loader_pass1();
int linking_loader_pass2();

int linking_loader_search_estab_control_section_name(const char * str, int argc, ESTAB extern_symbol_table[]);
int linking_loader_search_estab_symbol (struct __extern_symbol *extern_symbol, char * str);
void linking_loader_enter_symbol (struct __extern_symbol **extern_symbol, char * str, int address);
void linking_loader_print_load_map(int prog_len);

int linking_loader_search_control_section_name(const char * str, int argc, ESTAB extern_symbol_table[]);

void bp_clear(struct bpLink ** bpLinkHead_ptr);
void bp_address(struct bpLink ** bpLinkHead_ptr, int addr);


#endif
