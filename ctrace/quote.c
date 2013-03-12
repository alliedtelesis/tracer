#include <stdlib.h>
#include <string.h>

#include "quote.h"


/*
 * Shell quote a string
 *
 * returns pointer to quoted string, or NULL on failure.
 * caller frees.
 */
char *
shell_quote(char *string)
{
    int c;
    char *qstring, *q, *s;

    if (string == NULL)
        return NULL;

    qstring = malloc(2 * strlen(string) + 1);
    if (qstring == NULL)
        return NULL;

    for (q = qstring, s = string; s && (c = *s); s++) {
        switch (c)
        {
        case ' ':  case '\t': case '\n': case '$':  case '`': case ',':
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


/*
 * JSON quote a string
 *
 * returns pointer to quoted string, or NULL on failure.
 * caller frees.
 */
char *
json_quote(char *string)
{
    int c;
    char *qstring, *q, *s;

    if (string == NULL)
        return NULL;

    qstring = malloc(2 * strlen(string) + 1);
    if (qstring == NULL)
        return NULL;

    for (q = qstring, s = string; s && (c = *s); s++) {
        switch (c)
        {
        case '\b': case '\f': case '\n': case '\r':
        case '"':  case '\\': case '\t':
//      case '/':
            *q++ = '\\';
        default:
            *q++ = c;
            break;
        }
    }

    *q = '\0';
    return qstring;
}

