/*
Copyright (c) 2014, Antmicro Ltd <www.antmicro.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// a20 boot0 header
typedef struct {
        uint32_t jmp;     /* entry point jump */
        uint8_t magic[8]; /* "eGON.BT0" */
        uint32_t crc; 
        uint32_t length;

} __attribute__((packed)) boothdr_t;

#define BOOT0_MAGIC "eGON.BT0"
#define STAMP_VALUE 0x5F0A6C39
#define NEW_SIZE (24 * 1024)

uint32_t count_crc(uint32_t *data) {
        int i;
        uint32_t crc = 0;
        boothdr_t *hdr = (boothdr_t*)data;
        uint32_t orig_crc = hdr->crc;
        hdr->crc = STAMP_VALUE;
        for (i = 0; i < (hdr->length / 4); i++) crc += data[i];
        hdr->crc = orig_crc;
        return crc;
}

int main(int argc, char ** argv) {
	if (argc != 3) {
		printf("usage: %s orig_file new_file\n", argv[0]);
		return 1;
	}

	char buf[NEW_SIZE];
	memset(buf, 0xFF, NEW_SIZE);
	FILE *f = fopen(argv[1], "r");
	if (!f) {
		fprintf(stderr, "Error: cannot open file '%s'!\n", argv[1]);
	}
	fread(buf, NEW_SIZE, 1, f); // read no more than NEW_SIZE
	boothdr_t *h = (boothdr_t*) buf;
        if (strncmp((char*)h->magic, BOOT0_MAGIC, 8) != 0) {
                fprintf(stderr, "Error: this file has no eGON boot0 complaint header!\n");
                return 1;
        }
        uint32_t new_crc = count_crc((uint32_t*)buf);
	if (h->crc != new_crc) {
		printf("Warning: original file has wrong crc! (0x%08X != 0x%08X)\n", h->crc, new_crc);
	}
	uint32_t entrypoint = 8 + ((h->jmp & 0x00FFFFFF) << 2);
	printf("Entry point: 0x%08X (insn=0x%08X)\nCRC:         0x%08X (%s)\nLength:      0x%08X (%d bytes)\n", entrypoint, h->jmp, h->crc, (new_crc == h->crc) ? "OK" : "WRONG", h->length,h->length);
	printf("\nPatching file to %d bytes...\n\n", NEW_SIZE);

	h->length = NEW_SIZE;
        h->crc = count_crc((uint32_t*)buf);
	printf("New CRC:     0x%08X\n", h->crc);
	fclose(f);
	f = fopen(argv[2], "w");
	if (!f) {
		fprintf(stderr, "Error: cannot create file '%s'!\n", argv[1]);
		return 1;
	}
	fwrite(buf, NEW_SIZE, 1, f);
	fclose(f);
	return 0;
}
