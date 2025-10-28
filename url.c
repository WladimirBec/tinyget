/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2020 Wladimir Bec
 */
#include "url.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct url *
url_parse(char const *s)
{
    if (s == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // + 2 because every sections are separated by a character we can NULize
    // (://, @, :), except the path which starts with a '/' and thus cannot be
    // removed.
    struct url *u = calloc(1, sizeof(*u) + sizeof(*s) * (strlen(s) + 2));
    if (u == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    strcpy(u->original, s);

    char *pos = u->original;
    char *p   = NULL;

    if ((p = strstr(pos, "://")) != NULL) {
        *p        = '\0';
        u->scheme = pos;
        pos       = p + 3;
    }

    if ((p = strchr(pos, '/')) != NULL) {
        memmove(p + 1, p, strlen(p) + 1);
        *p      = '\0';
        u->path = p + 1;
    }

    if ((p = strchr(pos, '@')) != NULL) {
        *p      = '\0';
        u->user = pos;
        pos     = p + 1;

        if ((p = strchr(u->user, ':')) != NULL) {
            *p      = '\0';
            u->pass = (*++p == '\0' ? NULL : p);
        }
    }

    if ((p = strchr(pos, ':')) != NULL) {
        *p      = '\0';
        u->port = p + 1;
    }

    u->host = pos;
    return u;
}
