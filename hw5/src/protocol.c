#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "protocol.h"

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {
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
        bytes_written = write(fd, data, ntohl(pkt->size));
        if (bytes_written < ntohl(pkt->size)) {
            return -1;
        }
    }

    return 0; // or appropriate error code
}



int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **data) {
    // Read the packet header
    ssize_t bytes_read = read(fd, pkt, sizeof(XACTO_PACKET));
    if (bytes_read < sizeof(XACTO_PACKET)) {
        return -1;
    }

    // Convert multi-byte fields to host byte order
    pkt->serial = ntohl(pkt->serial);
    pkt->size = ntohl(pkt->size);
    pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
    pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

    // Read the payload, if present
    if (pkt->size > 0) {
        *data = malloc(pkt->size);
        if (*data == NULL) {
            return -1;
        }

        bytes_read = read(fd, *data, pkt->size);
        if (bytes_read < pkt->size) {
            return -1;
        }
    }

    return 0; // or appropriate error code
}
