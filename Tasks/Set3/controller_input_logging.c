/*
 * Copyright (c) 2017 Michael Zammit
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#define CONTROLLER_DEV "controller0"

typedef struct packet_bf_t packet_bf_t;
typedef union packet_t packet_t;

struct packet_bf_t {
    unsigned left: 1;
    unsigned right: 1;
    unsigned up: 1;
    unsigned down: 1;
    unsigned x: 1;
    unsigned y: 1;
    unsigned a: 1;
    unsigned b: 1;
    unsigned bk: 1;
    unsigned st: 1;
    unsigned se: 1;
    unsigned mo: 1;
    unsigned z: 1;
    unsigned w: 1;
    unsigned lt: 1;
    unsigned rt: 1;
    unsigned ltrig: 4;
    unsigned rtrig: 4;
    unsigned ctrlid: 8;
};

union packet_t {
    uint32_t raw_data;
    packet_bf_t bits;
};

void print_packet(packet_t *packet);
void print_packets(int count);

void print_packet(packet_t *packet) {
    //printf("raw_data: %08x\n", packet->raw_data);
    printf("#%u - left: %u right: %u up: %u down: %u x: %u y: %u a: %u b: "
           "%u bk: %u st: %u se: %u mo: %u z: %u w: %u lt: %u rt: %u ltrig: %u "
           "rtrig: %u\n",
           packet->bits.ctrlid,
           packet->bits.left, packet->bits.right, packet->bits.up, packet->bits.down,
           packet->bits.x, packet->bits.y, packet->bits.a, packet->bits.b,
           packet->bits.bk, packet->bits.st, packet->bits.se, packet->bits.mo,
           packet->bits.z, packet->bits.w, packet->bits.lt, packet->bits.rt,
           packet->bits.ltrig, packet->bits.rtrig
    );
}

void print_packet2(packet_bf_t *packet) {
    //printf("raw_data: %08x\n", packet->raw_data);
    printf("#%u - left: %u right: %u up: %u down: %u x: %u y: %u a: %u b: "
           "%u bk: %u st: %u se: %u mo: %u z: %u w: %u lt: %u rt: %u ltrig: %u "
           "rtrig: %u\n",
           packet->ctrlid,
           packet->left, packet->right, packet->up, packet->down,
           packet->x, packet->y, packet->a, packet->b,
           packet->bk, packet->st, packet->se, packet->mo,
           packet->z, packet->w, packet->lt, packet->rt,
           packet->ltrig, packet->rtrig
    );
}

void print_packets(int count) {
    // try and open the file descriptor
    int fd = open(CONTROLLER_DEV, O_RDWR);

    if (fd < 0) {
        printf("Controller Does Not Exist\n");
        exit(1);
    }

    char buf[32] = {0};
    // is this correct??
    write(fd, &count, sizeof(count));

    memset(buf, 0, 32);

    int packets_read = 0;

    while (packets_read < count) {
        if (read(fd, buf, sizeof(uint32_t)) == sizeof(uint32_t)) {

            uint32_t *data = (uint32_t *)buf;
            //for (int i = 0; i < 4; i++) {
            //    printf("buf[%d] = %02x\n",i,(unsigned char)buf[i]);
            //}
            packet_t pk;
            // set the raw data of the packet
            pk.raw_data = *data;
            print_packet2( &pk.bits );

            memset(buf, 0, 32);
        }
        packets_read++;
    }

    // close the file descriptor
    close(fd);
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("No Argument Specified\n");
        return 1;
    }

    int count = 0;

    if (sscanf(argv[1],"%d",&count) == 0 || count <= 0) {
        printf("Invalid Argument\n");
        return 1;
    }

    print_packets(count);

    return 0;
}

