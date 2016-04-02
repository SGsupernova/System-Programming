#ifndef __linking_loader__

struct bpLink {
	struct bpLink * next;
	int bpLineNum;
}

int linking_loader_main (const char * command, const char * inputStr);
int command_progaddr ();
int command_loader ();
int command_run ();
int command_bp ();

void bpClear();

#endif
