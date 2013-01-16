#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>

#include "transport.h"

/* env var for intercept symlink directory */
#define TRACE_INTERCEPT_DIR "TRACE_INTERCEPT_DIR"
#define TRACE_PACKAGE_NAME "BS_PACKAGE_NAME"

#define PATH_MAX 10240

/**
 * Quote all special characters with a backslash
 *
 * Returns a malloc'ed string
 */
char *shell_quote (char *string)
{
    int c;
    char *qstring, *q, *s;

    qstring = malloc(2 * strlen(string) + 1);

    for (q = qstring, s = string; s && (c = *s); s++) {
        switch (c)
        {
        case ' ':  case '\t': case '\n': case'$':   case '`': case ',': 
        case '(':  case ')':  case '<':  case '>':  case '{': case '}':
        case '[':  case ']':  case '!':  case '*':  case '?': case '^':
        case '\'': case '"':  case '\\': case '|':  case '&': case ';':
            *q++ = '\\';
            *q++ = c;
            break;
        case '#':
            if (s == string)
                *q++ = '\\';
        default:
            *q++ = c;
            break;
        }
    }

    *q = '\0';
    return qstring;
}

/**
 * Remove the a path from the a ':' delimited path list, in place.
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

/**
 * Get the $PACKAGE_NAME from environment, or "none"
 *
 * Returns a malloc'ed string
 */
char *trace_get_package(void)
{
    char *package_name = getenv(TRACE_PACKAGE_NAME);
    if (package_name == NULL) {
        package_name = malloc(5);
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

/**
 * Rebuild and requote the command line from argv
 *
 * Returns a malloc'ed string
 */
char *trace_get_command(int argc, char *argv[])
{
    char *command_string;
    char *quoted_arg;
    int i, len;

    for (i = 0, len = 0; i < argc; i++) {
        len += strlen(argv[i]);
    }
    /* 2x to allow room for worst case quoting */
    command_string = malloc(2 *(len + argc));
    *command_string = '\0';

    for (i = 0; i < argc; i++) {
        quoted_arg = shell_quote(argv[i]);
        strcat(command_string, quoted_arg);
        free(quoted_arg);
        if (i < (argc -1))
            strcat(command_string, " ");
    }
    return command_string;
}


void trace_send(int sockfd, int argc, char *argv[])
{
    char *pkg, *cwd, *cmd;
    char *msg;
    int msg_len = 0;

    pkg = trace_get_package();
    cwd = trace_get_directory();
    cmd = trace_get_command(argc, argv);
    msg_len = (strlen(pkg) + strlen(cwd) + strlen(cmd) + 3);
    msg = malloc(msg_len);
    snprintf(msg, msg_len, "%s\t%s\t%s", pkg, cwd, cmd);
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


    /* Setup the trace transport */
    if (getenv("TRACE_UNIX") != NULL) {
        sockfd = trace_transport_unix();
    } else if (getenv("TRACE_HOST") != NULL) {
        sockfd = trace_transport_inet();
    } else {
        fprintf(stderr, "Error: No trace transport specified\n");
        exit(EXIT_FAILURE);
    }
    if (sockfd < 0) {
        /* Transport setup failed */
        switch (sockfd) {
            case TRACE_TRANSPORT_ERROR_SOCK:
            case TRACE_TRANSPORT_ERROR_ADDR:
            case TRACE_TRANSPORT_ERROR_CONN:
                break;
        }
    } else {
        /* Send and shutdown transport */
        trace_send(sockfd, argc, argv);
        trace_transport_close(sockfd);
    }

    if(getenv("TRACE_CACHE") == NULL)
    {
        return execvp(argv[0], argv);
    } else {
        return execvp(getenv("TRACE_CACHE"), argv);
    }
}
