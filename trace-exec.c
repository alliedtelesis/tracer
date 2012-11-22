#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/* env var for intercept symlink directory */
#define TRACE_INTERCEPT_DIR "TRACE_INTERCEPT_DIR"
#define TRACE_TRANSPORT_TCP
#define TRACE_ADDR "TRACE_ADDR"
#define TRACE_PORT "TRACE_PORT"

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
        package_name = malloc(sizeof(char) * 5);
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

    for (int i = 0; i < argc; i++) {
        len += strlen(argv[i]);
    }
    /* Length of all the args, plus one char between, plus a null */
    command_string = malloc((sizeof(char) * len) + argc);

    for (int i = 0; i < argc; i++) {
        strcat(command_string, argv[i]);
        if (i < (argc -1))
            strcat(command_string, " ");
    }
    return command_string;
}

#ifdef TRACE_TRANSPORT_TCP
void init_sockaddr(struct sockaddr_in *name,
                   const char *hostname,
                   unsigned short int port)
{
    struct hostent *hostinfo;

    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    hostinfo = gethostbyname(hostname);
    if (hostinfo == NULL) {
        fprintf(stderr, "Error: no such host '%s'", hostname);
        exit(EXIT_FAILURE); /* We really should fall through to execvp */
    }
    name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}
#endif /* TRACE_TRANSPORT_TCP */


void trace_send(int argc, char *argv[])
{

    char *package, *cwd, *cmd, *path;

#ifdef TRACE_TRANSPORT_TCP
    int sockfd;
    struct sockaddr_in addr;
    int port;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Error: cannot open socket");
        exit(EXIT_FAILURE); /* We really should fall through to execvp */
    }

    port = atoi(getenv(TRACE_PORT));
    if (!(port > 0 && port < 65536)) {
        fprintf(stderr, "Error: invalid port");
    }
    init_sockaddr(&addr, getenv(TRACE_ADDR), port);

    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: cannot connect");
        exit(EXIT_FAILURE); /* We really should fall through to execvp */
    }
#endif /* TRACE_TRANSPORT_TCP */

    package = trace_get_package();
    write(sockfd, package, strlen(package));
    write(sockfd, "\t", 1);
    free(package);

    cwd = trace_get_directory();
    write(sockfd, cwd, strlen(cwd));
    write(sockfd, "\t", 1);
    free(cwd);

    cmd = trace_get_command(argc, argv);
    write(sockfd, cmd, strlen(cmd));
    write(sockfd, "\t", 1);
    free(cwd);

    path = getenv("PATH");
    write(sockfd, path, strlen(path));
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
    env_path = trace_path_remove(getenv("PATH"), env_intercept_dir);
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
