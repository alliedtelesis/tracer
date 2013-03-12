#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

#include "argv.h"
#include "ctrace.h"
#include "quote.h"
#include "ccmd.h"

#ifndef PATH_MAX
    #if defined(MAXPATHLEN)
        #define PATH_MAX MAXPATHLEN
    #else
        #define PATH_MAX 4096
    #endif
#endif


char *
_ccmd_get_directory(void)
{
    char buf[PATH_MAX];
    if (!getcwd(buf, sizeof buf)) {
        perror("Failed to determine cwd");
        return NULL;
    }
    return strdup(buf);
}

char *
_ccmd_get_command(char **argv)
{
    return argv_str(argv);
}

char *
_ccmd_get_file(char **argv)
{
    regex_t regex;

    if (regcomp(&regex, "\\.(s|(c(pp|xx|\\+\\+)?))$", REG_EXTENDED)) {
        perror("Failed to compile regex");
        return NULL;
    }

    for (; *argv; argv++) {
        int m = regexec(&regex, *argv, 0, NULL, 0);
        if (!m)
            return strdup(*argv);
    }
    return NULL;
}

char *
_ccmd_get_package(void) {
    char *package;
    {
        char *env_var = getenv(ENV_PKG);
        package = getenv(env_var ? env_var : ENV_PKG_DEFAULT);
    }
    return strdup(package ? package : "none");
}


ccmd_t *
ccmd_init(char **argv)
{
    ccmd_t *ccmd = malloc(sizeof(ccmd_t));

    if (ccmd == NULL)
        return NULL;

    ccmd->command = _ccmd_get_command(argv);
    ccmd->directory = _ccmd_get_directory();
    ccmd->file = _ccmd_get_file(argv);
    ccmd->package = _ccmd_get_package();

    if (!(ccmd->command && ccmd->directory && ccmd->file && ccmd->package)) {
        ccmd_free(ccmd);
        ccmd = NULL;
    }

    return ccmd;
}


void
ccmd_free(ccmd_t *ccmd)
{
    free(ccmd->command);
    free(ccmd->directory);
    free(ccmd->file);
    free(ccmd->package);
    free(ccmd);
}

char *
ccmd_json(ccmd_t *ccmd)
{
    char *cmd = json_quote(ccmd->command);
    char *cwd = json_quote(ccmd->directory);
    char *file = json_quote(ccmd->file);
    char *pkg = json_quote(ccmd->package);
    if (!(ccmd && cwd && file && pkg)) {
        perror("Failed to json quote ccmd");
        free(cmd);
        free(cwd);
        free(file);
        free(pkg);
        return NULL;
    }

    char *fmt = (
        "{\"command\": \"%s\", "
        "\"directory\": \"%s\", "
        "\"file\": \"%s\", "
        "\"package\": \"%s\"}"
    );
    char *json = malloc(
        strlen(fmt) - 8 +
        strlen(cmd) + strlen(cwd) + strlen(file) + strlen(pkg) + 1
    );
    sprintf(json, fmt, cmd, cwd, file, pkg);

    free(cmd);
    free(cwd);
    free(file);
    free(pkg);

    return json;

}
