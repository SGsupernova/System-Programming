#ifndef __linking_loader__
#define __linking_loader__

#define isOverMemoryBoundary(addr) (addr < 0 || addr > 0xFFFFF)


#include "20141570.h"
#include "linking_loader.h"

struct bpLink {
	struct bpLink * next;
	int bpLineNum;
};

int linking_loader_main (int num_command, const char * inputStr);

/* command function */
int command_progaddr ();
int command_loader (const char * inputStr);
int command_run (struct reg regSet);
int command_bp (struct bpLink ** bpLinkHead_ptr, const char * inputStr);

void linking_loader_print_load_map(int prog_len);

void bp_clear(struct bpLink ** bpLinkHead_ptr);
void bp_address(struct bpLink ** bpLinkHead_ptr, int addr);


#endif
