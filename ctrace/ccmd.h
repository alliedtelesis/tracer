#ifndef _CCMD_H
#define _CCMD_H    1

typedef struct ccmd {
    char *command;
    char *directory;
    char *file;
    char *package;
} ccmd_t;

ccmd_t *ccmd_init(char **argv);
void ccmd_free(ccmd_t *ccmd);

char *ccmd_json(ccmd_t *ccmd);

#endif /* ccmd.h */
