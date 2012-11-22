#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>

#include "transport.h"

/* env var for intercept symlink directory */
#define TRACE_INTERCEPT_DIR "TRACE_INTERCEPT_DIR"

#define PATH_MAX 10240

/**
 * Remove the a path from the a ':' delimited path list.
 */
char *trace_path_remove(char *env_path, const char *path)
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


char *trace_get_package(void)
{
    char *package_name = getenv("PACKAGE_NAME");
    if (package_name == NULL) {
        package_name = (char *) malloc(sizeof(char) * 5);
        strcpy(package_name, "none");
    } else {
        package_name = strdup(package_name);
    }
    return package_name;
}

char *trace_get_directory(void)
{
    char cwd[PATH_MAX] = {0};
    getcwd(cwd, PATH_MAX);
    return strdup(cwd);

}

char *trace_get_command(int argc, char *argv[])
{
    char *command_string;
    int len = 0;
    int i;

    for (i = 0; i < argc; i++) {
        len += strlen(argv[i]);
    }
    /* Length of all the args, plus one char between, plus a null */
    command_string = (char *) malloc((sizeof(char) * len) + argc);

    for (i = 0; i < argc; i++) {
        strcat(command_string, argv[i]);
        if (i < (argc -1))
            strcat(command_string, " ");
    }
    return command_string;
}


void trace_send(int argc, char *argv[])
{
    int sockfd;
    char *package, *cwd, *cmd, *path;
    char *message;
    int message_len = 0;

    if ((sockfd = trace_transport_inet()) < 0) {
        /* Transport init failed */
        switch (sockfd) {
            case TRACE_TRANSPORT_ERROR_SOCK:
            case TRACE_TRANSPORT_ERROR_ADDR:
            case TRACE_TRANSPORT_ERROR_CONN:
                break;
        }
        return;
    }

    package = trace_get_package();
    cwd = trace_get_directory();
    cmd = trace_get_command(argc, argv);
    path = getenv("PATH");
    /* TODO Add hostname / other unique ID to message */
    /* TODO How do we determine target platform? */
    message_len = (
        strlen(package) + strlen(cwd) + strlen(cmd) + strlen(path) + 4
    );
    message = (char *) malloc(sizeof(char) * message_len);
    snprintf(message, message_len, "%s\t%s\t%s\t%s", package, cwd, cmd, path);
    message_len = strlen(message);

    write(sockfd, message, strlen(message));
    close(sockfd);
    free(message);
    free(package);
    free(cwd);
    free(cmd);
}

int main(int argc, char *argv[])
{
    const char *env_intercept_dir;
    char *env_path;
    int i;

    /* Remove $TRACE_INTERCEPT_DIR from the system path */
    if ((env_intercept_dir = getenv(TRACE_INTERCEPT_DIR)) == NULL) {
        /* How are we supposed to do anything without the intercept dir? */
        fprintf(stderr, "Error: %s env var not set\n", TRACE_INTERCEPT_DIR);
        exit(EXIT_FAILURE);
    }
    env_path = trace_path_remove(getenv("PATH"), env_intercept_dir);
    setenv("PATH", env_path, 1);

    /* Chop the $TRACE_INTERCEPT_DIR off our called name 
     * ie. "/trace-int/gcc" -> "gcc" */
    argv[0] = basename(argv[0]);

    fprintf(stdout, "trace-exec: ");
    for (i = 0; i < argc; i++) {
        fprintf(stdout, " %s", argv[i]);
    }
    fprintf(stdout, "\n");

    return execvp(argv[0], argv);
}
