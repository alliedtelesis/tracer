#ifndef _ARGV_H
#define _ARGV_H    1

void argv_free(char **argv);
int argv_quote(char **argv);

int argv_copy(char **src, char ***dst);
int argv_prepend(char ***argv, char *arg);

int argv_len(char **argv);
char *argv_index(char **argv, const char *needle);
char *argv_str(char **argv);

#endif /* argv.h */
