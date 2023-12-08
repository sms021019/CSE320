#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "debug.h"
#include "protocol.h"

char* getTypeString(int type);

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {
    // info("%s Packet received", getTypeString(pkt->type));
    // debug("Packet serial: %d", pkt->serial);
    // debug("htonl(pkt->seria): %d", ntohl(pkt->serial));
    // debug("Packet status: %d", pkt->status);
    // debug("Packet size: %d", pkt->size);
    // debug("Packet null: %d", pkt->null);
    // debug("Packet timestamp_sec: %d", pkt->timestamp_sec);
    // debug("Packet timestamp_nsec: %d", pkt->timestamp_nsec);

    // Convert multi-byte fields to network byte order
    pkt->serial = htonl(pkt->serial);
    pkt->size = htonl(pkt->size);
    pkt->timestamp_sec = htonl(pkt->timestamp_sec);
    pkt->timestamp_nsec = htonl(pkt->timestamp_nsec);

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
    // info("%s Packet received", getTypeString(pkt->type));
    // debug("Packet serial: %d", pkt->serial);
    // debug("htonl(pkt->seria): %d", ntohl(pkt->serial));
    // debug("Packet status: %d", pkt->status);
    // debug("Packet size: %d", pkt->size);
    // debug("Packet null: %d", pkt->null);
    // debug("Packet timestamp_sec: %d", pkt->timestamp_sec);
    // debug("Packet timestamp_nsec: %d", pkt->timestamp_nsec);

    // Convert multi-byte fields to network byte order
    pkt->serial = ntohl(pkt->serial);
    pkt->size = ntohl(pkt->size);
    pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
    pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

    // Read the payload, if present
    if (pkt->size > 0) {
        *datap = malloc(pkt->size);
        if (*datap == NULL) {
            return -1;
        }

        bytes_read = read(fd, *datap, pkt->size);
        if (bytes_read < pkt->size) {
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
