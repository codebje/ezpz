/*
 * zdi.h
 *
 *  Created on: Jan 10, 2021
 *      Author: bje
 */

#ifndef INC_ZDI_H_
#define INC_ZDI_H_

#define ZDI_ADDR0_L			0x00		// Address Match 0 Low Byte
#define ZDI_ADDR0_H			0x01		// Address Match 0 High Byte
#define ZDI_ADDR0_U			0x02		// Address Match 0 Upper Byte
#define ZDI_ADDR1_L			0x04		// Address Match 1 Low Byte
#define ZDI_ADDR1_H			0x05		// Address Match 1 High Byte
#define ZDI_ADDR1_U			0x06		// Address Match 1 Upper Byte
#define ZDI_ADDR2_L			0x08		// Address Match 2 Low Byte
#define ZDI_ADDR2_H			0x09		// Address Match 2 High Byte
#define ZDI_ADDR2_U			0x0a		// Address Match 2 Upper Byte
#define ZDI_ADDR3_L			0x0c		// Address Match 3 Low Byte
#define ZDI_ADDR3_H			0x0d		// Address Match 3 High Byte
#define ZDI_ADDR3_U			0x0e		// Address Match 3 Upper Byte
#define ZDI_BRK_CTL			0x10		// Break Control Register
#define ZDI_MASTER_CTL		0x11		// Master Control Register
#define ZDI_WR_DATA_L		0x13		// Write Data Low Byte
#define ZDI_WR_DATA_H		0x14		// Write Data High Byte
#define ZDI_WR_DATA_U		0x15		// Write Data Upper Byte
#define ZDI_RW_CTL			0x16		// Read/Write Control Register
#define ZDI_BUS_CTL			0x17		// Bus Control Register
#define ZDI_IS4				0x21		// Instruction Store 4
#define ZDI_IS3				0x22		// Instruction Store 3
#define ZDI_IS2				0x23		// Instruction Store 2
#define ZDI_IS1				0x24		// Instruction Store 1
#define ZDI_IS0				0x25		// Instruction Store 0
#define ZDI_WR_MEM			0x30		// Write Memory Register

#define ZDI_ID_L			0x00		// eZ80 Product ID Low Byte Register
#define ZDI_ID_H			0x01		// eZ80 Product ID High Byte Register
#define ZDI_ID_REV			0x02		// eZ80 Product ID Revision Register
#define ZDI_STAT			0x03		// Status Register
#define ZDI_RD_L			0x10		// Read Memory Address Low Byte Register
#define ZDI_RD_H			0x11		// Read Memory Address High Byte Register
#define ZDI_RD_U			0x12		// Read Memory Address Upper Byte Register
#define ZDI_BUS_STAT		0x17		// Bus Status Register
#define ZDI_RD_MEM			0x20		// Read Memory Register

/* ZDI_BRK_CTL bits */
#define ZDI_BRK_NEXT		(1<<7)		// Enable break on next instruction
#define ZDI_BRK_ADDR3		(1<<6)		// Enable break address 3
#define ZDI_BRK_ADDR2		(1<<5)		// Enable break address 2
#define ZDI_BRK_ADDR1		(1<<4)		// Enable break address 1
#define ZDI_BRK_ADDR0		(1<<3)		// Enable break address 0
#define ZDI_IGN_LOW_1		(1<<2)		// Ignore Low Byte on ADDR1
#define ZDI_IGN_LOW_0		(1<<1)		// Ignore Low Byte on ADDR0
#define ZDI_SINGLE_STEP		(1<<0)		// Single Step Mode enable

/* ZDI_MASTER_CTL bits */
#define ZDI_RESET			(1<<7)		// Reset CPU

/* ZDI_RW_CTL functions */
#define ZDI_RD_MBASE_AF		0x00		// Read {MBASE, F, A}
#define ZDI_WR_MBASE_AF		0x80		// Write {MBASE, F, A}
#define ZDI_RD_BC			0x01		// Read BC
#define ZDI_WR_BC			0x81		// Write BC
#define ZDI_RD_DE			0x02		// Read DE
#define ZDI_WR_DE			0x82		// Write DE
#define ZDI_RD_HL			0x03		// Read HL
#define ZDI_WR_HL			0x83		// Write HL
#define ZDI_RD_IX			0x04		// Read IX
#define ZDI_WR_IX			0x84		// Write IX
#define ZDI_RD_IY			0x05		// Read IY
#define ZDI_WR_IY			0x85		// Write IY
#define ZDI_RD_SP			0x06		// Read SP
#define ZDI_WR_SP			0x86		// Write SP
#define ZDI_RD_PC			0x07		// Read PC
#define ZDI_WR_PC			0x87		// Write PC
#define ZDI_SET_ADL			0x08		// Set ADL mode
#define ZDI_RESET_ADL		0x09		// Reset ADL mode
#define ZDI_EXX				0x0A		// Exchange register sets
#define ZDI_RD_PCMEM		0x0B		// Read from [PC]
#define ZDI_WR_PCMEM		0x8B		// Write to [PC]

void zdi_write(uint8_t reg, uint8_t data);
uint8_t zdi_read(uint8_t reg);

#endif /* INC_ZDI_H_ */
