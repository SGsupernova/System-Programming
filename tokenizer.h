#ifndef __tokenizer__
#define __tokenizer__

#define MAX_LEN_TOKEN 80

struct token_link {
	struct token_link * next;
	char * token;
};

void tokenizer (const char * str, int *argc, char *** argv);
int tokenizer_delimiter_space (char * str, char *** res_argv);
void tokenizer_token_insert (struct token_link ** token_head, char * token);
void tokenizer_deallocate_token_link(struct token_link ** token_head);

int TokenizeOperand(const char * operandStr, char ** operand);

void tokenizer_deallocate_argvs (char *** argv_ptr, int argc);

#endif
