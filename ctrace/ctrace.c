#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include "argv.h"
#include "ccmd.h"
#include "masq.h"
#include "transport.h"
#include "ctrace.h"

#define PROGRAM_NAME "ctrace"

static const char USAGE[] =
    "Usage:\n"
//    "    "PROGRAM_NAME" [options]\n"
    "    "PROGRAM_NAME" cc [compiler options]\n"
    "    cc [compiler options]         (via symbolic link)\n"
    "\n"
    "Options:\n"
    "    "ENV_SOCK"\tAF_UNIX socket\n"
    "    "ENV_PREFIX"\toptional command prefix, ie. \"ccache\"\n"
    "    "ENV_PKG"\tenv-var containing package name, defaults to "ENV_PKG_DEFAULT"\n"
;


int
send_trace(char **exec_args)
{
    char *path = getenv(ENV_SOCK);
    if (!path)
        return -1;

    char *json;
    {
        ccmd_t *ccmd = ccmd_init(exec_args);
        if (!ccmd)
            return -1;

        json = ccmd_json(ccmd);
        ccmd_free(ccmd);
        if (!json)
            return -1;
    }

    {
        int sockfd = transport_open(path);
        if (sockfd == -1) {
            free(json);
            return -1;
        }
        if (write(sockfd, json, strlen(json)) == -1) {
            transport_close(sockfd);
            free(json);
            return -1;
        }
        transport_close(sockfd);
    }

    free(json);
    return 0;
}

int
main(int argc, char *argv[])
{
    char **exec_args = NULL;
    char *invoked_as = basename(argv[0]);

    if (strstr(invoked_as, PROGRAM_NAME) != NULL) {
        /* "ctrace ..." */
        if (argc < 2 || (argc > 1 && argv[1][0] == '-')) {
            /* "ctrace" */
            fputs(USAGE, stderr);
            exit(EXIT_FAILURE);
        }

        /* "ctrace cc -c hello.c" */
        if (argv_copy(argv+1, &exec_args)) {
            perror("Failed to copy arguments");
            exit(EXIT_FAILURE);
        }

    } else {
        /* "cc -c hello.c" */
        char *exec = masq_find_exec(getenv("PATH"), invoked_as, PROGRAM_NAME);
        if (exec == NULL) {
            fprintf(stderr, "Failed to find '%s'\n", invoked_as);
            exit(EXIT_FAILURE);
        }
        if (argv_copy(argv, &exec_args) != 0) {
            perror("Failed to copy arguments");
            exit(EXIT_FAILURE);
        }
        free(exec_args[0]);
        exec_args[0] = exec;
    }

    if (exec_args) {
        send_trace(exec_args);
        char *prefix = getenv(ENV_PREFIX);
        if (prefix)
            argv_prepend(&exec_args, prefix);
        return execvp(exec_args[0], exec_args);
    }

    exit(EXIT_FAILURE);
}
