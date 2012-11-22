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
    int size;
    int len = 0;
    int i;

    for (i = 0; i < argc; i++) {
        len += strlen(argv[i]);
    }
    /* Length of all the args, plus one char between, plus a null */
    size = (sizeof(char) * len + argc);
    command_string = (char *) malloc(size);
    memset(command_string, 0, size);

    for (i = 0; i < argc; i++) {
        strcat(command_string, argv[i]);
        if (i < (argc -1))
            strcat(command_string, " ");
    }
    return command_string;
}


void trace_send(int sockfd, int argc, char *argv[])
{
    char *pkg, *cwd, *cmd, *path;
    char *msg;
    int msg_len = 0;

    pkg = trace_get_package();
    cwd = trace_get_directory();
    cmd = trace_get_command(argc, argv);
    path = getenv("PATH");
    /* TODO Add hostname / other unique ID to message */
    /* TODO How do we determine target platform? */
    msg_len = (strlen(pkg) + strlen(cwd) + strlen(cmd) + strlen(path) + 5);
    msg = (char *) malloc(sizeof(char) * msg_len);
    snprintf(msg, msg_len, "%s\t%s\t%s\t%s\n", pkg, cwd, cmd, path);
    write(sockfd, msg, (msg_len - 1)); /* Dont send the null */
    free(msg);
    free(pkg);
    free(cwd);
    free(cmd);
}

int main(int argc, char *argv[])
{
    const char *env_intercept_dir;
    char *env_path;
    int sockfd;

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

    /* Setup the trace transport and send */
    /* TODO Selection of other transports */
    if ((sockfd = trace_transport_inet()) < 0) {
        /* Transport setup failed */
        switch (sockfd) {
            case TRACE_TRANSPORT_ERROR_SOCK:
            case TRACE_TRANSPORT_ERROR_ADDR:
            case TRACE_TRANSPORT_ERROR_CONN:
                break;
        }
    } else {
        trace_send(sockfd, argc, argv);
        if (sockfd != STDOUT_FILENO)
            close(sockfd);
    }

    return execvp(argv[0], argv);
}
