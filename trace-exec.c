#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>

/* env var for intercept symlink directory */
#define TRACE_INTERCEPT_DIR "TRACE_INTERCEPT_DIR"

/**
 * Remove the a path from the an environment path list.
 */
char *env_path_remove(char *env_path, const char *path)
{
    char *match;
    char *next;

    while ((match = strstr(env_path, path))) {
        next = index(match, ':');
        if (next) {
            /* Skip ':' */
            next++;
            /* Overwrite intercept dir path with a cut & shut */
            memmove(match, next, (strlen(next) + 1));
        } else {
            /* Intercept dir was the last or only path, finding the actual
             * executable is never going to work, but lets just play along.
             * Also trim trailing ':' if there is one */
            if (match > env_path) {
                match[-1] = '\0';
            } else {
                match[0] = '\0';
            }
        }
    }
    return env_path;
}


int main(int argc, char *argv[])
{
    const char *env_intercept_dir;
    char *env_path;

    /* Remove $TRACE_INTERCEPT_DIR from the system path */
    if ((env_intercept_dir = getenv(TRACE_INTERCEPT_DIR)) == NULL) {
        /* How are we supposed to do anything without the intercept dir? */
        fprintf(stderr, "Error: %s env var not set\n", TRACE_INTERCEPT_DIR);
        exit(EXIT_FAILURE);
    }
    env_path = env_path_remove(getenv("PATH"), env_intercept_dir);
    setenv("PATH", env_path, 1);

    /* Chop the $TRACE_INTERCEPT_DIR off our called name 
     * ie. "/trace-int/gcc" -> "gcc" */
    argv[0] = basename(argv[0]);

    fprintf(stdout, "trace-exec: ");
    for (int i = 0; i < argc; i++) {
        fprintf(stdout, " %s", argv[i]);
    }
    fprintf(stdout, "\n");

    return execvp(argv[0], argv);
}
