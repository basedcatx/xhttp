//
// Created by BaseDCaTx on 1/16/2025.
//

#include <stdio.h>

#include <stdint.h>

#include "../includes/crypt.h"
#include "../includes/packet.h"


int main (int argc, char *argv[]) {
    uint8_t *buffer = NULL;

    struct Packet packet = {
            .msgLength = 5,
            .structSize = sizeof(struct Packet),
            .flag = 0x01,
            .message = "Hello"
    };

    int bufEnc;

    // Encode the packet
    buffer = BufferEncode(&packet, 2048, &bufEnc);

    if (bufEnc < 0) {
        printf("Failed to encode packet:: %d.\n", bufEnc);
        return -1;
    }

    printf("encode packet:: %d.\n", bufEnc);


     // Decode the packet
    struct Packet decodedPacket;

    int decodedSize = BufferDecode(buffer, 2048, &decodedPacket);

    if (decodedSize < 0) {
        printf("Failed to decode packet. Error: %d\n", decodedSize);
        return -1;
    }

    // Output the decoded packet
    printf("Decoded Packet:\n");
    printf("Message Length: %u\n", decodedPacket.msgLength);
    printf("Struct Size: %u\n", decodedPacket.structSize);
    printf("Flag: %02x\n", decodedPacket.flag);
    printf("Message: %.*s\n", decodedPacket.msgLength, decodedPacket.message);

    free(buffer);
}
