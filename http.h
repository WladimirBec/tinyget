/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2020 Wladimir Bec
 */
#ifndef TINYGET_HTTP_H
#define TINYGET_HTTP_H

#include "url.h"
#include <netdb.h>
#include <stddef.h>
#include <stdint.h>

struct http_opts {
    /**
     * Specify the timeout amount in seconds for the connection to be made.
     *
     * 0 means never timeout.
     */
    /**
     * Struct containg the various timeouts used along the http_download().
     *
     * 0 means never timeout.
     */
    struct {
        /**
         * Timeout used to connect to the given url.
         */
        size_t connect;

        /**
         * Timeout used to send requests to the given url.
         */
        size_t write;

        /**
         * Timeout used to read responses from the given url.
         */
        size_t read;
    } timeout;

    /**
     * Struct containing the global callbacks used by http_download(), if any of
     * these callbacks return -1 http_download() will be stopped and returns -1.
     */
    struct {
        /**
         * Called before attempting to resolve the given url.
         */
        int (*resolve)(void *data);
        void *resolve_data;

        /**
         * Called before attempting to connect to the given url.
         */
        int (*connect)(struct addrinfo *addr, void *data);
        void *connect_data;

        /**
         * Called each time after data are sent to the given url.
         */
        int (*write)(char const *buf, size_t len, void *data);
        void *write_data;

        /**
         * Called each time after data are received from the given url.
         */
        int (*read)(char const *buf, size_t len, void *data);
        void *read_data;
    } callbacks;
};

/**
 * Downloads the given url.
 *
 * Returns 0 on success, otherwise -1 and set errno.
 */
int http_download(struct url const *url, struct http_opts const *opts);

#endif
