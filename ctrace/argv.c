#include <stdlib.h>
#include <string.h>

#include "quote.h"
#include "argv.h"


/*
 * Free an argv.
 *
 * Must not be called on process argv.
 */
void
argv_free(char **argv)
{
    if (argv) {
        for (char **a = argv; *a != NULL; a++) {
            free(*a);
        }
        free(argv);
    }
}


/*
 * Copy an argv.
 *
 * Can be used to copy process argv.
 *
 * returns 0 on success, -1 on failure.
 * caller frees **dst.
 */
int
argv_copy(char **src, char ***dst)
{
    int len = argv_len(src);
    char **tmp = malloc((len+1) * (sizeof src[0]));
    if (tmp == NULL) {
        return -1;
    }
    for (int i = 0; i < len; i++) {
        if ((tmp[i] = strdup(src[i])) == NULL) {
            argv_free(tmp);
            return -1;
        }
    }
    tmp[len] = NULL;
    *dst = tmp;
    return 0;
}


/*
 * Calculate length of argv, excluding terminating NULL.
 *
 * returns number of arguments in argv.
 */
int
argv_len(char **argv)
{
    int i;
    for (i = 0; argv[i]; i++);
    return i;
}


/*
 * Find first occurrence of argument in argv.
 *
 * returns pointer to matched argument, or NULL if not found.
 */
char *
argv_index(char **argv, const char *needle)
{
    for (; *argv; argv++) {
        if (strcmp(*argv, needle) == 0)
            return *argv;
    }
    return NULL;
}


/*
 * Convert argv to a shell-escaped string.
 *
 * returns pointer to string, or NULL on failure.
 * caller frees.
 */
char *
argv_str(char **argv)
{
    char **qargv;
    char *s, *string;
    int l = 0;

    if (argv_copy(argv, &qargv) != 0) {
        return NULL;
    }
    if (argv_quote(qargv) != 0) {
        argv_free(qargv);
        return NULL;
    }

    for (int i = 0; argv[i]; i++) {
        l += strlen(argv[i]) + 1;
    }

    string = malloc(l + 1);
    if (string == NULL) {
        argv_free(qargv);
        return NULL;
    }

    s = string;
    for (int i = 0; argv[i]; i++) {
        if (i)
            *s++ = ' ';
        strcpy(s, argv[i]);
        s += strlen(argv[i]);
    }
    *s = '\0';

    argv_free(qargv);
    return string;
}

/*
 * Quote all arguments.
 *
 * on failure argv may be left in a semi quoted-state.
 *
 * returns 0 on success, or -1 on failure.
 */
int
argv_quote(char **argv)
{
    for (; *argv; argv++) {
        char *qarg = shell_quote(*argv);
        if (qarg == NULL)
            return -1;
        free(*argv);
        *argv = qarg;
    }
    return 0;
}

