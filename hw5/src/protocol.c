#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <debug.h>
#include <errno.h>
#include "protocol.h"

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload) {
    size_t header_write_size = sizeof(CHLA_PACKET_HEADER);
    size_t bytes_wrote = 0;
    while (header_write_size > 0) {
        int write_size = write(fd, ((char *)hdr + bytes_wrote), header_write_size);
        if (write_size == -1 || write_size == 0) {
            // the errno is set by write so no need to set errno value
            info("WRITE first loop return -1, read_size: %d", write_size);
            errno = ECANCELED;
            return -1;
        }
        // decrement the write size
        header_write_size -= write_size;
        bytes_wrote += write_size;
    }

    // handle the payload writing
    uint32_t payload_write_size = 0;
    bytes_wrote = 0;
    if (payload != NULL && (payload_write_size = ntohl(hdr->payload_length)) > 0) {
        while (payload_write_size > 0) {
            int write_size = write(fd, ((char *)payload + bytes_wrote), payload_write_size);
            if (write_size == -1 || write_size == 0) {
                info("WRITE first loop return -1, read_size: %d", write_size);
                errno = ECANCELED;
                return -1;
            }
            // decrement the write size
            payload_write_size -= write_size;
            bytes_wrote += write_size;
        }
    }
    return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload) {
    size_t header_read_size = sizeof(CHLA_PACKET_HEADER);
    size_t bytes_read = 0;
    while (header_read_size > 0) {
        int read_size = read(fd, ((char *)hdr + bytes_read), header_read_size);
        if (read_size == -1 || read_size == 0) {
            info("READ first loop return -1, read_size: %d", read_size);
            errno = ECANCELED;
            return -1;
        }
        // decrement read size
        header_read_size -= read_size;
        bytes_read += read_size;
    }

    // handling the reading of payload
    uint32_t payload_read_size = 0;
    bytes_read = 0;
    if ((payload_read_size = ntohl(hdr->payload_length)) > 0) {
        info("payload_read_size: %u", payload_read_size);
        *payload = calloc(1, payload_read_size);
        if (payload == NULL) {
            info("READ payload dynamic memory allocation failed.");
            errno = ENOMEM;
            return -1;
        }
        while (payload_read_size > 0) {
            int read_size = read(fd, ((char *)*payload + bytes_read), payload_read_size);
            if (read_size == -1 || read_size == 0) {
                info("READ second loop return -1, read_size: %d", read_size);
                free(*payload);
                errno = ECANCELED;
                return -1;
            }
            // decrement read size
            payload_read_size -= read_size;
            bytes_read += read_size;
        }
    }
    return 0;
}
