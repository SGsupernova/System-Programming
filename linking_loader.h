#ifndef __linking_loader__
#define __linking_loader__

#define isOverMemoryBoundary(addr) (addr < 0 || addr > 0xFFFFF)

struct bpLink {
	struct bpLink * next;
	int bpLineNum;
};

int linking_loader_main (int num_command, const char * inputStr);
int command_progaddr ();
int command_loader ();
int command_run ();
int command_bp (struct bpLink ** bpLinkHead_ptr, const char * inputStr);

void linkingFetchObjFilename ();

void bp_clear(struct bpLink ** bpLinkHead_ptr);
void bp_address(struct bpLink ** bpLinkHead_ptr, int addr);

#endif
