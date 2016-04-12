#include <string.h>
#include <stdlib.h>
#include "tokenizer.h"
#include <stdio.h>

#ifdef TEST
int main () {
	char str[50] = "hello man I'm Taeseung";
	char ** argv = NULL;
	int argc = 0, i;

	tokenizer(str, &argc, &argv);

	printf("str : %s\n", str);
	printf("argc : %d\n", argc);
	printf("argv : %p\n", argv);

	for (i = 0; i < argc; i++) {
		printf("argv[%d] : %s\n", i, argv[i]);
	}

	return 0;
}

#endif

void tokenizer(const char *str, int * argc, char *** argv) {
	char * duplicatedStr = strdup(str);
	*argc = tokenizer_delimiter_space(duplicatedStr, argv);

	free(duplicatedStr);
}

int tokenizer_delimiter_space (char *str, char *** res_argv) {
	char delimiter[] = { ' ', '\t', '\n' };
	int num_del = 3;
	char * temp_token = (char *) calloc (MAX_LEN_TOKEN, sizeof(char));
	struct token_link * token_head = NULL,
					  * link = NULL;
	int i = 0, argc = 0, token_process_flag = 0, new_flag = 0, find_delimeter_flag = 0;
	int idx_del = 0, idx_temp = 0;


	for (i = 0; str[i] != 0; i ++) {
		for (idx_del = 0; idx_del < num_del; idx_del ++ ) {
			find_delimeter_flag |= (str[i] == delimiter[idx_del]);
		}

		if (find_delimeter_flag) {
			if (token_process_flag) {
				tokenizer_token_insert (&token_head, temp_token);

				/* initialize temp_token */
				for (idx_temp = 0; temp_token[idx_temp] != 0; idx_temp++) {
					temp_token[idx_temp] = 0;
				}
				idx_temp = 0;

				new_flag = 1;
				token_process_flag = 0;
			}

			/* initialize */
			find_delimeter_flag = 0;
		}
		else {
			if (new_flag) {
				argc ++;
				new_flag = 0;
			}
			temp_token[idx_temp ++] = str[i];
			token_process_flag = 1;
		}
	}

	// last token
	if (token_process_flag) {
		tokenizer_token_insert (&token_head, temp_token);
		argc ++;
	}


	/* store all argvs */
	i = 0;
	link = token_head;
	*res_argv = (char **) calloc (argc, sizeof(char *));

	while (link) {
		(*res_argv)[i++] = strdup(link->token);
		link = link->next;
	}

	/* deallocation */
	tokenizer_deallocate_token_link(&token_head);

	return argc;
}

void tokenizer_token_insert (struct token_link ** token_head, char * token) {
	struct token_link * new = NULL;

	new = (struct token_link *) calloc(MAX_LEN_TOKEN, sizeof(struct token_link));
	new->token = strdup(token);

	if (*token_head) {
		struct token_link * link = *token_head;

		while (link->next) {
			link = link->next;
		}
		link->next = new;
	}
	else {
		*token_head = new;
	}
}

void tokenizer_deallocate_token_link(struct token_link ** token_head) {
	struct token_link * link = *token_head,
					  * temp = NULL;

	while (link) {
		temp = link;
		link = link->next;

		if (temp) free(temp->token);
		free(temp);
	}

	*token_head = NULL;
}

// return 0 : there is no error
//		  0 : there are errors
int TokenizeOperand(const char * operandStr, char ** operand) {
	sscanf(operandStr, "%[^ ,] , %[^ ,]", operand[0], operand[1]);
}

void tokenize_deallocate_argvs (char *** argv_ptr, int argc) {
	if (*argv_ptr && argc) {
		int i = 0;

		for (i = 0; i < argc; i++) {
			if (!(*argc)[i]) { printf("tokenizing error!!\n"); }
			free((*argc)[i]);
		}
		
	}

	(*argv_ptr) = NULL;
}
