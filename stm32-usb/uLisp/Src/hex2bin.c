/*
 * hex2bin.c
 *
 *  Created on: 4 Feb 2021
 *      Author: bje
 */

#include <stdint.h>
#include <stdio.h>

#include "hex2bin.h"
#include "zdi.h"

static int convert(int c)
{
	if (c < '0') return -1;
	if (c <= '9') return c - '0';
	if (c < 'A') return -1;
	if (c <= 'F') return c - 'A' + 10;
	if (c < 'a') return -1;
	if (c > 'f') return -1;
	return c - 'a' + 10;
}

static int readhex()
{
	int a = getchar(); putchar(a);
	int b = getchar(); putchar(b);

	int c = convert(a);
	int d = convert(b);

	if (c < 0) {
		printf("Invalid character '%c'\r\n", a);
		return c;
	}

	if (d < 0) {
		printf("Invalid character '%c'\r\n", b);
		return d;
	}

	return c * 16 + d;
}

static int ihex_line(uint32_t *baseaddr) {
	uint8_t length;
	uint32_t address;
	uint8_t buffer[256];
	uint8_t cmd;
	uint8_t sum = 0;
	int byte;
	int ch;
	int i;

	// scan forever for ':'
	while ((ch = getchar()) != ':') { putchar(ch); };
	putchar(ch);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
/* code generating the warning */

	if ((byte = readhex()) < 0) return 0;length = byte; sum += byte;
	if ((byte = readhex()) < 0) return 0; address = byte * 256; sum += byte;
	if ((byte = readhex()) < 0) return 0; address += byte; sum += byte;
	if ((byte = readhex()) < 0) return 0; cmd = byte; sum += byte;
	for (i = 0; i < length; i++) {
		if ((byte = readhex()) < 0) return 0; buffer[i] = byte;
		sum += byte;
	}
#pragma GCC diagnostic pop

	if ((byte = readhex()) < 0) return 0;
	sum = -sum;
	if (byte != sum) {
		printf("invalid checksum %02x, expected %02x\r\n", byte, sum);
		return 0;
	}
	switch (cmd) {
		case 0:
			address += *baseaddr;
			zdi_write(ZDI_WR_DATA_L, address & 0xff);
			zdi_write(ZDI_WR_DATA_H, (address >> 8) & 0xff);
			zdi_write(ZDI_WR_DATA_U, (address >> 16) & 0xff);
			zdi_write(ZDI_RW_CTL, ZDI_WR_PC);
			for (int i = 0; i < length; i++) {
				zdi_write(ZDI_WR_MEM, buffer[i]);
			}
			printf("   [WROTE AT %06lx]", address);
			break;
		case 1:
			printf("IHEX load complete\r\n");
			return 0;
		case 2:
			if (length == 2) {
				*baseaddr = (buffer[0] << 12) + (buffer[1] << 4);
			} else {
				printf("Invalid Extended Segment Address Record\r\n");
				return 0;
			}
			break;
		case 4:
			if (length == 2) {
				*baseaddr = (buffer[0] << 24) + (buffer[1] << 16);
			} else {
				printf("Invalid Extended Linear Address Record\r\n");
				return 0;
			}
			break;
		default:
			printf("Unrecognised IHEX command byte %02x\r\n", cmd);
			return 0;
	}

	return 1;
}

// load an iHex file into memory
void ihex_load(void)
{
	uint32_t baseaddr = 0;
	uint32_t pc;

	uint8_t status = zdi_read(ZDI_STAT);

	if (!(status & 0x80)) {
		zdi_write(ZDI_BRK_CTL, 0x80);
	}
	if (!(status & 0x10)) {
		zdi_write(ZDI_RW_CTL, ZDI_SET_ADL);
	}

	zdi_write(ZDI_RW_CTL, ZDI_RD_PC);
	pc = (zdi_read(ZDI_RD_L))
	   + (zdi_read(ZDI_RD_H) << 8)
	   + (zdi_read(ZDI_RD_U) << 16);

	printf("IHEX upload ready\r\n");

	do {
	} while (ihex_line(&baseaddr));

	zdi_write(ZDI_WR_DATA_L, pc & 0xff);
	zdi_write(ZDI_WR_DATA_H, (pc >> 8) & 0xff);
	zdi_write(ZDI_WR_DATA_U, (pc >> 16) & 0xff);
	zdi_write(ZDI_RW_CTL, ZDI_WR_PC);
	if (!(status & 0x10)) {
		zdi_write(ZDI_RW_CTL, ZDI_RESET_ADL);
	}
	if (!(status & 0x80)) {
		zdi_write(ZDI_BRK_CTL, 0x00);
	}
}
