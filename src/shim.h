#ifndef SHIM_H
#define SHIM_H

typedef int rl_command_func_t(int, int);

char* readline(const char* prompt);
void add_history(const char* line);

void using_history(void);
int read_history(const char* filename);
int write_history(const char* filename);
int rl_variable_bind(const char* variable, const char* value);
void rl_initialize(void);
int rl_bind_key(int key, rl_command_func_t* func);

extern rl_command_func_t rl_insert;

#endif
