/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2020 Wladimir Bec
 */
#ifndef TINYGET_URL_H
#define TINYGET_URL_H

struct url {
    char *scheme;
    char *user;
    char *pass;
    char *host;
    char *port;
    char *path;

    char original[];
};

/**
 * Parses the given s into a struct url, returns NULL on error and set errno.
 */
struct url *url_parse(char const *s);

#endif
