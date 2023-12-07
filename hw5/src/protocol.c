#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "debug.h"
#include "protocol.h"

char* getTypeString(int type);

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {
    info("%s Packet received", getTypeString(pkt->type));

    // Write the packet header
    ssize_t bytes_written = write(fd, pkt, sizeof(XACTO_PACKET));
    if (bytes_written < sizeof(XACTO_PACKET)) {
        return -1;
    }

    // Write the payload, if present
    if (ntohl(pkt->size) > 0 && data != NULL) {
        debug("payload");
        bytes_written = write(fd, data, ntohl(pkt->size));
        if (bytes_written < ntohl(pkt->size)) {
            return -1;
        }
    }

    return 0;
}

int read_all(int fd, void *buffer, size_t size) {
    size_t bytesRead = 0;
    size_t result;

    while (bytesRead < size) {
        result = read(fd, buffer + bytesRead, size - bytesRead);
        if (result < 1) {
            if (errno == EINTR) continue;
            return -1;
        }
        bytesRead += result;
    }

    return 0;
}


int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap) {
    // Read the packet header
    ssize_t bytes_read = read(fd, pkt, sizeof(XACTO_PACKET));
    if (bytes_read < sizeof(XACTO_PACKET)) {
        return -1;
    }
    info("%s Packet received", getTypeString(pkt->type));
    info("Size: %d", ntohl(pkt->size));

    // Read the payload, if present
    if (ntohl(pkt->size) > 0) {
        *datap = malloc(ntohl(pkt->size));
        if (*datap == NULL) {
            return -1;
        }

        bytes_read = read(fd, *datap, ntohl(pkt->size));
        if (bytes_read < ntohl(pkt->size)) {
            free(*datap);
            return -1;
        }
    }

    return 0;
}



char* getTypeString(int type) {
    char* types[] = {
        "XACTO_NO_PKT", "PUT", "GET",
        "KEY", "VALUE", "COMMIT",
        "REPLY"
    };

    if (type < 0 || type >= sizeof(types) / sizeof(types[0])) {
        return "Unknown";
    }

    return types[type];
}