#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>


char *
masq_find_exec(char *path, const char *exec, const char *exclude)
{
    if (path == NULL)
        return NULL;
    path = strdup(path);
    if (path == NULL)
        return NULL;

    char *state;
    char *dir = strtok_r(path, ":", &state);
    for (; dir; dir = strtok_r(NULL, ":", &state)) {
        char *exec_path = malloc(strlen(dir) + 1 + strlen(exec) + 1);
        if (exec_path == NULL) {
            free(path);
            return NULL;
        }
        sprintf(exec_path, "%s/%s", dir, exec);

        struct stat stl, stf;
        if (access(exec_path, X_OK) == 0 &&
            lstat(exec_path, &stl) == 0 &&
            stat(exec_path, &stf) == 0 &&
            S_ISREG(stf.st_mode))
        {
            if (S_ISLNK(stl.st_mode)) {
                char *real_exec_path = realpath(exec_path, NULL);
                if (real_exec_path) {
                    char *target = basename(real_exec_path);
                    free(real_exec_path);
                    if (strcmp(target, exclude) == 0) {
                        free(exec_path);
                        continue;
                    }
                }
            }
            free(path);
            return exec_path;
        }
        free(exec_path);
    }
    free(path);
    return NULL;
}
