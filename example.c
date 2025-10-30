#include "http.h"
#include "url.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

static int
resolve_cb(void *data)
{
    printf("resolving %s...\n", (char *)data);
    return 0;
}

static int
connect_cb(struct addrinfo *addr, void *data)
{
    char s[INET6_ADDRSTRLEN] = {0};
    void *d                  = NULL;

    switch (addr->ai_addr->sa_family) {
    default:       fputs("unknown sa_family", stderr); return -1;
    case AF_INET:  d = &((struct sockaddr_in *)addr->ai_addr)->sin_addr; break;
    case AF_INET6: d = &((struct sockaddr_in6 *)addr->ai_addr)->sin6_addr;
    }

    if (inet_ntop(addr->ai_addr->sa_family, d, s, INET6_ADDRSTRLEN) == NULL) {
        perror("inet_ntop failed");
        return -1;
    }

    printf("connecting to %s...\n", s);
    return 0;
}

static int
write_cb(char const *buf, size_t len, void *data)
{
    printf("sending:\n%.*s\n", (int)len, buf);
    return 0;
}

static int
read_cb(char const *buf, size_t len, void *data)
{
    printf("read:\n%.*s\n", (int)len, buf);
    return 0;
}

int
main(void)
{
    struct url *u = url_parse("http://www.google.com/404");
    if (u == NULL) {
        perror("failed to parse url");
        return -1;
    }

    int err                   = 0;
    struct http_callbacks cbs = {
        .resolve      = resolve_cb,
        .resolve_data = u->host,
        .connect      = connect_cb,
        .write        = write_cb,
        .read         = read_cb,
    };
    if ((err = http_download(u, &cbs)) == -1) {
        perror("download failed");
        err = 1;
    }

    free(u);
    return err;
}
