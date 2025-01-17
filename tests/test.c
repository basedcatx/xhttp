//
// Created by BaseDCaTx on 1/16/2025.
//

#include <stdio.h>
#include "../includes/utils.h"
#include <stdint.h>
#include "../includes/packet.h"
#include "../includes/crypt.h"


int main (int argc, char *argv[]) {
    uint8_t buffer[2048];
    uint8_t compressedBuffer[2048];

    struct Packet packet = {
            .msgLength = 5,
            .structSize = sizeof(struct Packet),
            .flag = 0x01,
            .message = (uint8_t *)"Hello"
    };

    // Encode the packet
    int encodedSize = BufferEncode(&packet, buffer, sizeof(buffer));

    if (encodedSize < 0) {
        printf("Failed to encode packet.\n");
        return -1;
    }

    for (int i = 0; i < 2048; i++) {
        printf("%c", buffer[i]);
    }

    // Decode the packet
    struct Packet decodedPacket;

    int decodedSize = BufferDecode(buffer, sizeof(buffer), &decodedPacket);

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

    // Clean up allocated memory
    free(decodedPacket.message);

}
