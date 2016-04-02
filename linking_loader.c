#include "linking_loader.h"
#include "20141570.h"

int linking_loader_main (int num_command, const char * inputStr) {
	static struct bpLink * bpLinkHead = NULL;
	static int progaddr = 0;



	int error_flag = 0;


	switch (command_num) {
		case 1 : // progaddr command
			progaddr = command_progaddr(inputStr, &error_flag);
			break;

		case 2: // loader command

			break;

		case 3 : // run command

			break;

		case 4 : // break point command
			command_bp(inputStr);
			break;


	}


}

int command_progaddr (const char * inputStr, int * error_flag) {
	int progaddr = strtoi (inputStr, &error_flag, 16);
	if (temp_int < 0 || temp_int > MEMORY_SIZE - 1) {
		error_flag = 1;
		SEND_ERROR_MESSAGE("INPUT DOES NOT ADDRESS");
		return 0;
	}

	return progaddr;
}

int command_loader () {



}

int command_run () {


}

int command_bp (struct bpLink ** bpLinkHead) {
	

}

void linkingFetchObjFilename () {


}
