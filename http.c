/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2020 Wladimir Bec
 */
#include "http.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 4096

#define call_cb(cb, ...) cb != NULL &&cb(__VA_ARGS__)

// features wish:
// timeout
// resume
// download if newer
// gzip support
// authorization
// tls (with bearssl)
// support redirect

static int
http_connect(struct url const *url, struct http_callbacks const *cbs)
{
    if (call_cb(cbs->resolve, cbs->resolve_data) == -1) {
        errno = ECANCELED;
        return -1;
    }

    char const *port = url->port;
    if (port == NULL) {
        port = strcasecmp(url->scheme, "https") == 0 ? "443" : "80";
    }

    struct addrinfo *res = NULL;
    struct addrinfo hint = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    if (getaddrinfo(url->host, port, &hint, &res) != 0) {
        errno = EHOSTUNREACH;
        return -1;
    }

    int s = -1;
    for (struct addrinfo *r = res; r != NULL; r = r->ai_next, s = -1) {
        if (call_cb(cbs->connect, r, cbs->connect_data) == -1) {
            errno = ECANCELED;
            break;
        }

        if ((s = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1) {
            continue;
        }

        if (connect(s, r->ai_addr, r->ai_addrlen) != -1) {
            break;
        }

        close(s);
    }

    freeaddrinfo(res);
    return s;
}

static int
make_request(struct url const *u, int sock, struct http_callbacks const *cbs)
{
    char buf[BUF_SIZE] = {0};
    size_t len         = 0;
    int n              = 0;

#define APPEND_TO_BUF(...)                                              \
    if ((n = snprintf(buf + len, BUF_SIZE - len, __VA_ARGS__)) == -1) { \
        return -1;                                                      \
    } else if (n >= (BUF_SIZE - len)) {                                 \
        printf("'%s'\n", buf);                                          \
        errno = EOVERFLOW;                                              \
        return -1;                                                      \
    } else {                                                            \
        len += n;                                                       \
    }

    APPEND_TO_BUF("GET %s HTTP/1.1\r\n", u->path == NULL ? "/" : u->path);
    APPEND_TO_BUF("Host: %s\r\n", u->host);
    APPEND_TO_BUF("Connection: close\r\n");
    APPEND_TO_BUF("Accept-Encoding: identity\r\n");
    APPEND_TO_BUF("\r\n");
#undef APPEND_TO_BUF

    int sent = 0;
    while (sent < len) {
        if ((n = send(sock, buf + sent, len, 0)) == -1) {
            return -1;
        }

        if (call_cb(cbs->write, buf + sent, len, cbs->write_data) == -1) {
            errno = ECANCELED;
            return -1;
        }

        sent += n;
        len -= n;
    }

    return 0;
}

static int
read_response(int sock, struct http_callbacks const *cbs)
{
    char buf[BUF_SIZE] = {0};
    size_t len         = 0;
    int n              = 0;
    char *body         = NULL;

    // first get the headers
    while (len < BUF_SIZE) {
        if ((n = recv(sock, buf + len, BUF_SIZE, 0)) == -1) {
            return -1;
        }
        len += n;

        // we got full headers
        if ((body = strstr(buf, "\r\n\r\n")) != NULL) {
            break;
        }
    }

    // headers not found
    if (body == NULL) {
        errno = EOVERFLOW;
        return -1;
    }

    char *hdr = buf;
    do {
        char *end = strstr(hdr, "\r\n");
        end[0]    = '\0';

        // todo analyze the headers

        hdr = end + 2;
        if (end[2] == '\r' && end[3] == '\n') {
            break;
        }
    } while (1);

    // move start of the body at the start of the buffer
    len = BUF_SIZE - (body - buf) - 4;
    memmove(buf, body + 4, len);
    if (len > 0 && call_cb(cbs->read, buf, len, cbs->read_data) == -1) {
        errno = ECANCELED;
        return -1;
    }

    // read the rest of the body
    while ((n = recv(sock, buf, BUF_SIZE, 0)) != 0) {
        if (n == -1) {
            return -1;
        }

        if (call_cb(cbs->read, buf, n, cbs->read_data) == -1) {
            errno = ECANCELED;
            return -1;
        }
    }

    return 0;
}

int
http_download(struct url const *url, struct http_callbacks const *cbs)
{
    if (cbs == NULL) {
        cbs = &(struct http_callbacks){0};
    }

    int sock = http_connect(url, cbs);
    if (sock == -1) {
        return -1;
    }

    int err = 0;
    if ((err = make_request(url, sock, cbs) == -1)) {
        goto end;
    }

    if ((err = read_response(sock, cbs)) == -1) {
        goto end;
    }

end:
    close(sock);
    return err;
}
