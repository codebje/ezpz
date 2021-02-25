/*
 * debugger.c
 *
 *  Created on: 8 Feb 2021
 *      Author: bje
 */

#include <stdlib.h>
#include <stdio.h>

#include "debugger.h"
#include "zdi.h"

uint32_t zdi_rw_read(uint8_t command)
{
	if (command > 7) return 0xffffffff;
	zdi_write(ZDI_RW_CTL, command);
	return (zdi_read(ZDI_RD_L))
			| (zdi_read(ZDI_RD_H) << 8)
			| (zdi_read(ZDI_RD_U) << 16);
}

void zdi_rw_write(uint8_t command, uint32_t value)
{
	if (command < 0x80 || command > 0x87) return;
	zdi_write(ZDI_WR_DATA_L, value & 0xff);
	zdi_write(ZDI_WR_DATA_H, (value >> 8) & 0xff);
	zdi_write(ZDI_WR_DATA_U, (value >> 16) & 0xff);
	zdi_write(ZDI_RW_CTL, command);
}

/* I/O ports */
uint8_t zdi_in(uint16_t port)
{
	uint8_t in_a_bc[2] = { 0xed, 0x78 };		// in a, (bc)
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);
	uint32_t af = zdi_rw_read(ZDI_RD_MBASE_AF);
	uint32_t bc = zdi_rw_read(ZDI_RD_BC);

	zdi_rw_write(ZDI_WR_BC, port);
	zdi_exec(in_a_bc, 2);
	uint32_t result = zdi_rw_read(ZDI_RD_MBASE_AF);
	zdi_rw_write(ZDI_WR_BC, bc);
	zdi_rw_write(ZDI_WR_MBASE_AF, af);
	zdi_rw_write(ZDI_WR_PC, pc);
	return (result & 0xff);
}

void zdi_out(uint16_t port, uint8_t value)
{
	uint8_t out_bc_a[2] = { 0xed, 0x79 };		// out (bc), a
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);
	uint32_t af = zdi_rw_read(ZDI_RD_MBASE_AF);
	uint32_t bc = zdi_rw_read(ZDI_RD_BC);

	zdi_rw_write(ZDI_WR_BC, port);
	zdi_rw_write(ZDI_WR_MBASE_AF, (af & 0xffff00) | value);
	zdi_exec(out_bc_a, 2);
	zdi_rw_write(ZDI_WR_BC, bc);
	zdi_rw_write(ZDI_WR_MBASE_AF, af);
	zdi_rw_write(ZDI_WR_PC, pc);
}

/* State examination */
void zdi_status()
{
	uint8_t stat = zdi_read(ZDI_STAT);
	uint8_t bus_stat  = zdi_read(ZDI_BUS_STAT);

	printf("ACTIVE=%c   HALT_SLP=%c   ADL=%c   MADL=%c   IEF1=%c   ZDI_BUSACK=%c   ZDI_BUS_STAT=%c\r\n",
			(stat & 0x80) ? '*' : ' ',
			(stat & 0x20) ? '*' : ' ',
			(stat & 0x10) ? '*' : ' ',
			(stat & 0x08) ? '*' : ' ',
			(stat & 0x04) ? '*' : ' ',
			(bus_stat & 0x80) ? '*' : ' ',
			(bus_stat & 0x40) ? '*' : ' ');
}

void zdi_registers()
{
	uint32_t mfa = zdi_rw_read(ZDI_RD_MBASE_AF);
	uint32_t bc = zdi_rw_read(ZDI_RD_BC);
	uint32_t de = zdi_rw_read(ZDI_RD_DE);
	uint32_t hl = zdi_rw_read(ZDI_RD_HL);
	uint32_t ix = zdi_rw_read(ZDI_RD_IX);
	uint32_t iy = zdi_rw_read(ZDI_RD_IY);
	uint32_t sp = zdi_rw_read(ZDI_RD_SP);
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);

	printf("MBASE=%02lx FLAGS=%c%c%c%c%c%c%c%c A=%02lx BC=%06lx DE=%06lx HL=%06lx IX=%06lx IY=%06lx SP=%06lx PC=%06lx\r\n",
			(mfa >> 16) & 0xff,
			(mfa & 0x8000) ? 'S' : 's',
			(mfa & 0x4000) ? 'Z' : 'z',
			(mfa & 0x2000) ? '*' : '-',
			(mfa & 0x1000) ? 'H' : 'h',
			(mfa & 0x0800) ? '*' : '-',
			(mfa & 0x0400) ? 'P' : 'p',
			(mfa & 0x0200) ? 'N' : 'n',
			(mfa & 0x0100) ? 'C' : 'c',
			mfa & 0xff,
			bc, de, hl, ix, iy, sp, pc);
}

void zdi_stacktrace(int depth)
{

}

void zdi_memdump(uint32_t address)
{
	uint8_t storage[8*16+1];
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);
	uint8_t adl = !!(zdi_read(ZDI_STAT) & 0x10);

	if (!adl) zdi_write(ZDI_RW_CTL, ZDI_SET_ADL);
	zdi_rw_write(ZDI_WR_PC, address - 1);
	zdi_read_block(ZDI_RD_MEM, storage, 8*16+1);

	for (int r = 0; r < 8; r++) {
		printf("%06lx:", address + r * 16);
		for (int c = 0; c < 16; c++) {
			printf(" %02x", storage[r*16+c+1]);
			if (c == 7) printf(" ");
		}
		printf("\r\n");
	}

	if (!adl) zdi_write(ZDI_RW_CTL, ZDI_RESET_ADL);
	zdi_rw_write(ZDI_WR_PC, pc);
}

/* Code execution */
void zdi_exec(uint8_t *opcodes, uint8_t count)
{
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);

	// the instruction executes when ZDI_IS0 is written, so write from end to start
	for (int x = count > 5 ? 4 : count - 1; x >= 0; x--) {
		zdi_write(ZDI_IS0 - x, opcodes[x]);
	}

	zdi_rw_write(ZDI_WR_PC, pc);
}

static struct breakstate {
	uint32_t bp[4];
	uint8_t bpflags;
	uint8_t stopped;
} breakstate = { { 0, 0, 0, 0 }, 0, 0 };

static void set_bctrl(int stop)
{
	breakstate.stopped = stop ? breakstate.stopped : !!(zdi_read(ZDI_STAT) & 0x80);
	zdi_write(ZDI_ADDR0_L, breakstate.bp[0] & 0xff);
	zdi_write(ZDI_ADDR0_H, (breakstate.bp[0] >> 8) & 0xff);
	zdi_write(ZDI_ADDR0_U, (breakstate.bp[0] >> 16) & 0xff);
	zdi_write(ZDI_ADDR1_L, breakstate.bp[1] & 0xff);
	zdi_write(ZDI_ADDR1_H, (breakstate.bp[1] >> 8) & 0xff);
	zdi_write(ZDI_ADDR1_U, (breakstate.bp[1] >> 16) & 0xff);
	zdi_write(ZDI_ADDR2_L, breakstate.bp[2] & 0xff);
	zdi_write(ZDI_ADDR2_H, (breakstate.bp[2] >> 8) & 0xff);
	zdi_write(ZDI_ADDR2_U, (breakstate.bp[2] >> 16) & 0xff);
	zdi_write(ZDI_ADDR3_L, breakstate.bp[3] & 0xff);
	zdi_write(ZDI_ADDR3_H, (breakstate.bp[3] >> 8) & 0xff);
	zdi_write(ZDI_ADDR3_U, (breakstate.bp[3] >> 16) & 0xff);
	uint8_t ctrl = (breakstate.stopped ? 0x80 : 0x00) | (breakstate.bpflags << 3);
	zdi_write(ZDI_BRK_CTL, ctrl);

}

void zdi_set_break(uint8_t bpnum, uint32_t address)
{
	if (bpnum < 4) {
		breakstate.bp[bpnum] = address;
		breakstate.bpflags |= (1 << bpnum);
	}

	set_bctrl(0);
}

uint32_t zdi_get_break(uint8_t bpnum)
{
	if (bpnum < 4 && (breakstate.bpflags & (1 << bpnum))) {
		return breakstate.bp[bpnum];
	} else {
		return 0xffffffff;
	}
}

void zdi_clear_break(uint8_t bpnum)
{
	breakstate.bpflags &= ~(1 << bpnum);
	set_bctrl(0);
}

void zdi_stop()
{
	breakstate.stopped = 1;
	set_bctrl(1);
}

void zdi_run()
{
	breakstate.stopped = 0;
	set_bctrl(1);
}

static char *disasm(uint8_t *ops, uint8_t *count, uint32_t addr);
static void disasm_here()
{
	uint8_t opcodes[6*8+1];
	uint8_t stat = zdi_read(ZDI_STAT);
	if (!(stat & 0x80)) return;
	uint32_t pc = zdi_rw_read(ZDI_RD_PC);
	uint8_t offset = 1;

	zdi_rw_write(ZDI_WR_PC, pc - 1);
	zdi_read_block(ZDI_RD_MEM, opcodes, 6*8+1);
	zdi_rw_write(ZDI_WR_PC, pc);

	for (int i = 0; i < 8; i++) {
		uint8_t count;
		char *mnemonic = disasm(opcodes+offset, &count, pc+offset-1);
		printf("%06lx\t", pc+offset-1);
		for (int c = 0; c < 6; c++) {
			if (c < count) {
				printf("%02x ", opcodes[offset+c]);
			} else {
				printf("   ");
			}
		}
		printf("%s\r\n", mnemonic);
		offset = offset + count;
	}

}

void zdi_stepinto()
{
	uint8_t ctrl = 0x81 | (breakstate.bpflags << 3);
	zdi_write(ZDI_BRK_CTL, ctrl);
	zdi_registers();
	disasm_here();
}

void zdi_stepover()
{
	printf("Not implemented.\r\n");
}

void zdi_stepuntil(uint32_t addr)
{
	printf("Not implemented.\r\n");
}

static char *disasm_word(char *buf, const char *op, const char *args, const char *prefix, uint8_t *bytes, uint8_t adl)
{
	uint32_t arg = bytes[0] | (bytes[1] << 8) | (adl ? (bytes[2] << 16) : 0);
	sprintf(buf, "%s%s            ", op, prefix);
	sprintf(buf+12, args, arg);
	return buf;
}

static char *disasm_byte(char *buf, const char *op, const char *args, const char *prefix, uint8_t *bytes, uint8_t adl)
{
	sprintf(buf, "%s%s            ", op, prefix);
	sprintf(buf+12, args, *bytes);
	return buf;
}

static char *disasm_disp(char *buf, const char *op, const char *args, const char *prefix, uint8_t *bytes, uint8_t adl)
{
	int8_t d = (int8_t)*bytes;
	sprintf(buf, "%s%s            ", op, prefix);
	sprintf(buf+12, args, d);
	return buf;
}

static char *disasm_none(char *buf, const char *op, const char *args, const char *prefix, uint8_t *bytes, uint8_t adl)
{
	sprintf(buf, "%s%s            ", op, prefix);
	sprintf(buf+12, args);
	return buf;
}

static char *disasm_rel(char *buf, const char *op, const char *args, const char *prefix, uint8_t *bytes, uint8_t adl, uint32_t addr)
{
	uint32_t offset = (uint32_t)((int32_t)((int8_t)*bytes));
	uint32_t target = (addr+offset) & (adl ? 0xffffff : 0xffff);

	sprintf(buf, "%s%s            ", op, prefix);
	sprintf(buf+12, args, target);
	return buf;
}

#define NONE(instr, args) *count += 1; return disasm_none(buffer, instr, args, prefix, ops+1, adl)
#define BYTE(instr, args) *count += 2; return disasm_byte(buffer, instr, args, prefix, ops+1, adl)
#define DISP(instr, args) *count += 2; return disasm_disp(buffer, instr, args, prefix, ops+1, adl);
#define WORD(instr, args) *count += 3 + adl; return disasm_word(buffer, instr, args, prefix, ops+1, adl);
#define PCREL(instr, args) *count += 2; return disasm_rel(buffer, instr, args, prefix, ops+1, adl, addr + *count);
#define REGINSTRS(base, instr, start) \
		case base+0: NONE(instr, start "B"); \
		case base+1: NONE(instr, start "C"); \
		case base+2: NONE(instr, start "D"); \
		case base+3: NONE(instr, start "E"); \
		case base+4: NONE(instr, start "H"); \
		case base+5: NONE(instr, start "L"); \
		case base+6: NONE(instr, start "(HL)"); \
		case base+7: NONE(instr, start "A");
#define INDEX(reg,opp) \
		{ \
			*count += 1; \
			ops = ops + 1; \
			switch (*ops) { \
				case 0x07: DISP("LD", "BC, ("reg"%+d)"); \
				case 0x09: NONE("ADD", reg ", BC"); \
				case 0x0F: DISP("LD", "("reg"%+d), BC"); \
				case 0x17: DISP("LD", "DE, ("reg"%+d)"); \
				case 0x19: NONE("ADD", reg", DE"); \
				case 0x1F: DISP("LD", "("reg"%+d), DE"); \
				case 0x21: WORD("LD", ""reg", $%lx"); \
				case 0x22: WORD("LD", "($%lx), "reg); \
				case 0x23: NONE("INC", reg); \
				case 0x24: NONE("INC", reg"H"); \
				case 0x25: NONE("DEC", reg"H"); \
				case 0x26: BYTE("LD", ""reg"H, $%x"); \
				case 0x27: DISP("LD", "HL, ("reg"%+d)"); \
				case 0x29: NONE("ADD", reg", "reg); \
				case 0x2A: WORD("LD", ""reg", ($%lx)"); \
				case 0x2B: NONE("DEC", reg""); \
				case 0x2C: NONE("INC", reg"L"); \
				case 0x2D: NONE("DEC", reg"L"); \
				case 0x2E: BYTE("LD", ""reg"L, $%x"); \
				case 0x2F: DISP("LD", "("reg"%+d), HL"); \
				case 0x31: DISP("LD", ""opp", ("reg"%+d)"); \
				case 0x34: DISP("INC", "("reg"%+d)"); \
				case 0x35: DISP("DEC", "("reg"%+d)"); \
				case 0x36: *count += 3; sprintf(buffer, "LD%10s("reg"%+d), $%x", prefix, ops[1], ops[2]); \
				case 0x37: DISP("LD", ""reg", ("reg"%+d)"); \
				case 0x39: NONE("ADD", reg", SP"); \
				case 0x3E: DISP("LD", "("reg"%+d), "opp); \
				case 0x3F: DISP("LD", "("reg"%+d), "reg); \
				case 0x44: NONE("LD", "B, "reg"H"); \
				case 0x45: NONE("LD", "B, "reg"L"); \
				case 0x46: DISP("LD", "B, ("reg"%+d)"); \
				case 0x4C: NONE("LD", "C, "reg"H"); \
				case 0x4D: NONE("LD", "C, "reg"L"); \
				case 0x4E: DISP("LD", "C, ("reg"%+d)"); \
				case 0x54: NONE("LD", "D, "reg"H"); \
				case 0x55: NONE("LD", "D, "reg"L"); \
				case 0x56: DISP("LD", "D, ("reg"%+d)"); \
				case 0x5C: NONE("LD", "E, "reg"H"); \
				case 0x5D: NONE("LD", "E, "reg"L"); \
				case 0x5E: DISP("LD", "E, ("reg"%+d)"); \
				case 0x60: NONE("LD", reg"H, B"); \
				case 0x61: NONE("LD", reg"H, C"); \
				case 0x62: NONE("LD", reg"H, D"); \
				case 0x63: NONE("LD", reg"H, E"); \
				case 0x64: NONE("LD", reg"H, "reg"H"); \
				case 0x65: NONE("LD", reg"H, "reg"L"); \
				case 0x66: DISP("LD", "H, ("reg"%+d)"); \
				case 0x67: NONE("LD", reg"H, A"); \
				case 0x68: NONE("LD", reg"L, B"); \
				case 0x69: NONE("LD", reg"L, C"); \
				case 0x6a: NONE("LD", reg"L, D"); \
				case 0x6b: NONE("LD", reg"L, E"); \
				case 0x6c: NONE("LD",reg"L, "reg"H"); \
				case 0x6d: NONE("LD", reg"L, "reg"L"); \
				case 0x6e: DISP("LD", "L, ("reg"%+d)"); \
				case 0x6f: NONE("LD", reg"L, A"); \
				case 0x70: DISP("LD", "("reg"%+d), B"); \
				case 0x71: DISP("LD", "("reg"%+d), C"); \
				case 0x72: DISP("LD", "("reg"%+d), D"); \
				case 0x73: DISP("LD", "("reg"%+d), E"); \
				case 0x74: DISP("LD", "("reg"%+d), H"); \
				case 0x75: DISP("LD", "("reg"%+d), L"); \
				case 0x77: DISP("LD", "("reg"%+d), A"); \
				case 0x7c: NONE("LD", "A, "reg"H"); \
				case 0x7d: NONE("LD", "A, "reg"L"); \
				case 0x7e: DISP("LD", "A, ("reg"%+d)"); \
				case 0x84: NONE("ADD", "A, "reg"H"); \
				case 0x85: NONE("ADD", "A, "reg"L"); \
				case 0x86: DISP("ADD", "A, ("reg"%+d)"); \
				case 0x8c: NONE("ADC", "A, "reg"H"); \
				case 0x8d: NONE("ADC", "A, "reg"L"); \
				case 0x8e: DISP("ADC", "A, ("reg"%+d)"); \
				case 0x94: NONE("SUB", "A, "reg"H"); \
				case 0x95: NONE("SUB", "A, "reg"L"); \
				case 0x96: DISP("SUB", "A, ("reg"%+d)"); \
				case 0x9c: NONE("SBC", "A, "reg"H"); \
				case 0x9d: NONE("SBC", "A, "reg"L"); \
				case 0x9e: DISP("SBC", "A, ("reg"%+d)"); \
				case 0xa4: NONE("AND", "A, "reg"H"); \
				case 0xa5: NONE("AND", "A, "reg"L"); \
				case 0xa6: DISP("AND", "A, ("reg"%+d)"); \
				case 0xac: NONE("XOR", "A, "reg"H"); \
				case 0xad: NONE("XOR", "A, "reg"L"); \
				case 0xae: DISP("XOR", "A, ("reg"%+d)"); \
				case 0xb4: NONE("OR", "A, "reg"H"); \
				case 0xb5: NONE("OR", "A, "reg"L"); \
				case 0xb6: DISP("OR", "A, ("reg"%+d)"); \
				case 0xbc: NONE("CP", "A, "reg"H"); \
				case 0xbd: NONE("CP", "A, "reg"L"); \
				case 0xbe: DISP("CP", "A, ("reg"%+d)"); \
				case 0xcb: { \
					*count += 1; \
					switch (ops[2]) { \
						case 0x06: DISP("RLC", "("reg"%+d)"); \
						case 0x0E: DISP("RRC", "("reg"%+d)"); \
						case 0x16: DISP("RL", "("reg"%+d)"); \
						case 0x1E: DISP("RR", "("reg"%+d)"); \
						case 0x26: DISP("SLA", "("reg"%+d)"); \
						case 0x2E: DISP("SRA", "("reg"%+d)"); \
						case 0x3E: DISP("SRL", "("reg"%+d)"); \
						case 0x46: DISP("BIT", "0, ("reg"%+d)"); \
						case 0x4E: DISP("BIT", "1, ("reg"%+d)"); \
						case 0x56: DISP("BIT", "2, ("reg"%+d)"); \
						case 0x5E: DISP("BIT", "3, ("reg"%+d)"); \
						case 0x66: DISP("BIT", "4, ("reg"%+d)"); \
						case 0x6E: DISP("BIT", "5, ("reg"%+d)"); \
						case 0x76: DISP("BIT", "6, ("reg"%+d)"); \
						case 0x7E: DISP("BIT", "7, ("reg"%+d)"); \
						case 0x86: DISP("RES", "0, ("reg"%+d)"); \
						case 0x8E: DISP("RES", "1, ("reg"%+d)"); \
						case 0x96: DISP("RES", "2, ("reg"%+d)"); \
						case 0x9E: DISP("RES", "3, ("reg"%+d)"); \
						case 0xA6: DISP("RES", "4, ("reg"%+d)"); \
						case 0xAE: DISP("RES", "5, ("reg"%+d)"); \
						case 0xB6: DISP("RES", "6, ("reg"%+d)"); \
						case 0xBE: DISP("RES", "7, ("reg"%+d)"); \
						case 0xC6: DISP("SET", "0, ("reg"%+d)"); \
						case 0xCE: DISP("SET", "1, ("reg"%+d)"); \
						case 0xD6: DISP("SET", "2, ("reg"%+d)"); \
						case 0xDE: DISP("SET", "3, ("reg"%+d)"); \
						case 0xE6: DISP("SET", "4, ("reg"%+d)"); \
						case 0xEE: DISP("SET", "5, ("reg"%+d)"); \
						case 0xF6: DISP("SET", "6, ("reg"%+d)"); \
						case 0xFE: DISP("SET", "7, ("reg"%+d)"); \
						default: NONE("TRAP", ""); \
					} \
				} \
				case 0xe1: NONE("POP", reg); \
				case 0xe3: NONE("EX", "(SP), "reg); \
				case 0xe5: NONE("PUSH", reg); \
				case 0xe9: NONE("JP", "("reg")"); \
				case 0xf9: NONE("LD", "SP, "reg); \
				default: NONE("TRAP", ""); \
			} \
		}

/* NOT re-entrant */
static char *disasm(uint8_t *ops, uint8_t *count, uint32_t addr)
{
	static char buffer[24];
	uint8_t adl = !!(zdi_read(ZDI_STAT) & 0x10);
	const char *prefix = "";
	*count = 0;

	if (*ops == 0x40 || *ops == 0x49) {
		adl = 0;
		*count = 1;
		prefix = (*ops == 0x40) ? ".SIS" : ".LIS";
		ops++;
	} else if (*ops == 0x52 || *ops == 0x5B) {
		adl = 1;
		*count = 1;
		prefix = (*ops == 0x52) ? ".SIL" : ".LIL";
		ops++;
	}

	switch (*ops) {
		case 0x00: NONE("NOP", "");
		case 0x01: WORD("LD", "BC, $%lx");
		case 0x02: NONE("LD", "(BC), A");
		case 0x03: NONE("INC", "BC");
		case 0x04: NONE("INC", "B");
		case 0x05: NONE("DEC", "B");
		case 0x06: BYTE("LD", "B, $%x");
		case 0x07: NONE("RLCA", "");
		case 0x08: NONE("EX", "AF, AF'");
		case 0x09: NONE("ADD", "HL, BC");
		case 0x0a: NONE("LD", "A, (BC)");
		case 0x0b: NONE("DEC", "BC");
		case 0x0c: NONE("INC", "C");
		case 0x0d: NONE("DEC", "C");
		case 0x0e: BYTE("LD", "C, $%x");
		case 0x0f: NONE("RRCA", "");
		case 0x10: PCREL("DJNZ", "$%lx");
		case 0x11: WORD("LD", "DE, $%lx");
		case 0x12: NONE("LD", "(DE), A");
		case 0x13: NONE("INC", "DE");
		case 0x14: NONE("INC", "D");
		case 0x15: NONE("DEC", "D");
		case 0x16: BYTE("LD", "D, $%x");
		case 0x17: NONE("RLA", "");
		case 0x18: PCREL("JR", "$%lx");
		case 0x19: NONE("ADD", "HL, DE");
		case 0x1a: NONE("LD", "A, (DE)");
		case 0x1b: NONE("DEC", "DE");
		case 0x1c: NONE("INC", "E");
		case 0x1d: NONE("DEC", "E");
		case 0x1e: BYTE("LD", "E, $%x");
		case 0x1f: NONE("RRA", "");
		case 0x20: PCREL("JR", "NZ, $%lx");
		case 0x21: WORD("LD", "HL, $%lx");
		case 0x22: NONE("LD", "(HL), A");
		case 0x23: NONE("INC", "HL");
		case 0x24: NONE("INC", "H");
		case 0x25: NONE("DEC", "H");
		case 0x26: BYTE("LD", "H, $%x");
		case 0x27: NONE("DAA", "");
		case 0x28: PCREL("JR", "Z, $%lx");
		case 0x29: NONE("ADD", "HL, HL");
		case 0x2a: WORD("LD", "HL, ($%lx)");
		case 0x2b: NONE("DEC", "HL");
		case 0x2c: NONE("INC", "L");
		case 0x2d: NONE("DEC", "L");
		case 0x2e: BYTE("LD", "L, $%x");
		case 0x2f: NONE("CPL", "");
		case 0x30: PCREL("JR", "NC, $%lx");
		case 0x31: WORD("LD", "SP, $%lx");
		case 0x32: WORD("LD", "($%lx), A");
		case 0x33: NONE("INC", "SP");
		case 0x34: NONE("INC", "(HL)");
		case 0x35: NONE("DEC", "(HL)");
		case 0x36: BYTE("LD", "(HL), $%x");
		case 0x37: NONE("SCF", "");
		case 0x38: PCREL("JR", "C,$%lx");
		case 0x39: NONE("ADD", "HL, SP");
		case 0x3a: WORD("LD", "A, ($%lx)");
		case 0x3b: NONE("DEC", "SP");
		case 0x3c: NONE("INC", "A");
		case 0x3d: NONE("DEC", "A");
		case 0x3e: BYTE("LD", "A, $%x");
		case 0x3f: NONE("CCF", "");
		case 0x40: NONE("TRAP", "");
		case 0x41: NONE("LD", "B, C");
		case 0x42: NONE("LD", "B, D");
		case 0x43: NONE("LD", "B, E");
		case 0x44: NONE("LD", "B, H");
		case 0x45: NONE("LD", "B, L");
		case 0x46: NONE("LD", "B, (HL)");
		case 0x47: NONE("LD", "B, A");
		case 0x48: NONE("LD", "C, B");
		case 0x49: NONE("TRAP", "");
		case 0x4a: NONE("LD", "C, D");
		case 0x4b: NONE("LD", "C, E");
		case 0x4c: NONE("LD", "C, H");
		case 0x4d: NONE("LD", "C, L");
		case 0x4e: NONE("LD", "C, (HL)");
		case 0x4f: NONE("LD", "C, A");
		case 0x50: NONE("LD", "D, B");
		case 0x51: NONE("LD", "D, C");
		case 0x52: NONE("TRAP", "");
		case 0x53: NONE("LD", "D, E");
		case 0x54: NONE("LD", "D, H");
		case 0x55: NONE("LD", "D, L");
		case 0x56: NONE("LD", "D, (HL)");
		case 0x57: NONE("LD", "D, A");
		case 0x58: NONE("LD", "E, B");
		case 0x59: NONE("LD", "E, C");
		case 0x5a: NONE("LD", "E, D");
		case 0x5b: NONE("TRAP", "");
		case 0x5c: NONE("LD", "E, H");
		case 0x5d: NONE("LD", "E, L");
		case 0x5e: NONE("LD", "E, (HL)");
		case 0x5f: NONE("LD", "E, A");
		REGINSTRS(0x60, "LD", "H, ");
		REGINSTRS(0x68, "LD", "L, ");
		case 0x70: NONE("LD", "(HL), B");
		case 0x71: NONE("LD", "(HL), C");
		case 0x72: NONE("LD", "(HL), D");
		case 0x73: NONE("LD", "(HL), E");
		case 0x74: NONE("LD", "(HL), H");
		case 0x75: NONE("LD", "(HL), L");
		case 0x76: NONE("HALT", "");
		case 0x77: NONE("LD", "(HL), A");
		REGINSTRS(0x78, "LD", "A, ");
		REGINSTRS(0x80, "ADD", "A, ");
		REGINSTRS(0x88, "ADC", "A, ");
		REGINSTRS(0x90, "SUB", "A, ");
		REGINSTRS(0x98, "SBC", "A, ");
		REGINSTRS(0xa0, "AND", "A, ");
		REGINSTRS(0xa8, "XOR", "A, ");
		REGINSTRS(0xb0, "OR", "A, ");
		REGINSTRS(0xb8, "CP", "A, ");
		case 0xc0: NONE("RET", "NZ");
		case 0xc1: NONE("POP", "BC");
		case 0xc2: WORD("JP", "NZ, $%lx");
		case 0xc3: WORD("JP", "$%lx");
		case 0xc4: WORD("CALL", "NZ, $%lx");
		case 0xc5: NONE("PUSH", "BC");
		case 0xc6: BYTE("ADD", "A, $%x");
		case 0xc7: NONE("RST", "$00");
		case 0xc8: NONE("RET", "Z");
		case 0xc9: NONE("RET", "");
		case 0xca: WORD("JP", "Z, $%lx");
		case 0xcb: {
			*count += 1;
			ops = ops + 1;
			switch (*ops) {
				REGINSTRS(0x00, "RLC", "");
				REGINSTRS(0x08, "RRC", "");
				REGINSTRS(0x10, "RL", "");
				REGINSTRS(0x18, "RR", "");
				REGINSTRS(0x20, "SLA", "");
				REGINSTRS(0x28, "SRA", "");
				REGINSTRS(0x38, "SRL", "");
				REGINSTRS(0x40, "BIT", "0, ");
				REGINSTRS(0x48, "BIT", "1, ");
				REGINSTRS(0x50, "BIT", "2, ");
				REGINSTRS(0x58, "BIT", "3, ");
				REGINSTRS(0x60, "BIT", "4, ");
				REGINSTRS(0x68, "BIT", "5, ");
				REGINSTRS(0x70, "BIT", "6, ");
				REGINSTRS(0x78, "BIT", "7, ");
				REGINSTRS(0x80, "SET", "0, ");
				REGINSTRS(0x88, "SET", "1, ");
				REGINSTRS(0x90, "SET", "2, ");
				REGINSTRS(0x98, "SET", "3, ");
				REGINSTRS(0xa0, "SET", "4, ");
				REGINSTRS(0xa8, "SET", "5, ");
				REGINSTRS(0xb0, "SET", "6, ");
				REGINSTRS(0xb8, "SET", "7, ");
				REGINSTRS(0xc0, "RES", "0, ");
				REGINSTRS(0xc8, "RES", "1, ");
				REGINSTRS(0xd0, "RES", "2, ");
				REGINSTRS(0xd8, "RES", "3, ");
				REGINSTRS(0xe0, "RES", "4, ");
				REGINSTRS(0xe8, "RES", "5, ");
				REGINSTRS(0xf0, "RES", "6, ");
				REGINSTRS(0xf8, "RES", "7, ");
				default: NONE("TRAP", "");
			}
		}
		case 0xcc: WORD("CALL", "Z, $%lx");
		case 0xcd: WORD("CALL", "$%lx");
		case 0xce: BYTE("ADC", "A, $%x");
		case 0xcf: NONE("RST", "$08");
		case 0xd0: NONE("RET", "NC");
		case 0xd1: NONE("POP", "DE");
		case 0xd2: WORD("JP", "NC, $%lx");
		case 0xd3: BYTE("OUT", "($%x), A");
		case 0xd4: WORD("CALL", "NC, $%lx");
		case 0xd5: NONE("PUSH", "DE");
		case 0xd6: BYTE("SUB", "A, $%x");
		case 0xd7: NONE("RST", "$10");
		case 0xd8: NONE("RET", "C");
		case 0xd9: NONE("EXX", "");
		case 0xda: WORD("JP", "C, $%lx");
		case 0xdb: BYTE("IN", "A, ($%x)");
		case 0xdc: WORD("CALL", "C, $%lx");
		case 0xdd: INDEX("IX", "IY");
		case 0xde: BYTE("SBC", "A, $%x");
		case 0xdf: NONE("RST", "$18");
		case 0xe0: NONE("RET", "PO");
		case 0xe1: NONE("POP", "HL");
		case 0xe2: WORD("JP", "PO, $%lx");
		case 0xe3: NONE("EX", "(SP), HL");
		case 0xe4: WORD("CALL", "PO, $%lx");
		case 0xe5: NONE("PUSH", "HL");
		case 0xe6: BYTE("AND", "A, $%x");
		case 0xe7: NONE("RST", "$20");
		case 0xe8: NONE("RET", "PE");
		case 0xe9: NONE("JP", "(HL)");
		case 0xea: WORD("JP", "PE, $%lx");
		case 0xeb: NONE("EX", "DE, HL");
		case 0xec: WORD("CALL", "PE, $%lx");
		case 0xed: {
			*count += 1;
			ops = ops + 1;
			switch (*ops) {
				case 0x00: BYTE("IN0", "B, ($%x)");
				case 0x01: BYTE("OUT0", "($%x), B");
				case 0x02: DISP("LEA", "BC, IX%+d");
				case 0x03: DISP("LEA", "BC, IY%+d");
				case 0x04: NONE("TST", "A, B");
				case 0x07: NONE("LD", "BC, (HL)");
				case 0x08: BYTE("IN0", "C, ($%x)");
				case 0x09: BYTE("OUT0", "($%x), C");
				case 0x0C: NONE("TST", "A, C");
				case 0x0F: NONE("LD", "(HL), BC");
				case 0x10: BYTE("IN0", "D, ($%x)");
				case 0x11: BYTE("OUT0", "($%x), D");
				case 0x12: DISP("LEA", "DE, IX%+d");
				case 0x13: DISP("LEA", "DE, IY%+d");
				case 0x14: NONE("TST", "A, D");
				case 0x17: NONE("LD", "DE, (HL)");
				case 0x18: BYTE("IN0", "E, ($%x)");
				case 0x19: BYTE("OUT0", "($%x), E");
				case 0x1C: NONE("TST", "A, E");
				case 0x1F: NONE("LD", "(HL), DE");
				case 0x20: BYTE("IN0", "H, ($%x)");
				case 0x21: BYTE("OUT0", "($%x), H");
				case 0x22: DISP("LEA", "HL, IX%+d");
				case 0x23: DISP("LEA", "HL, IY%+d");
				case 0x24: NONE("TST", "A, H");
				case 0x27: NONE("LD", "HL, (HL)");
				case 0x28: BYTE("IN0", "L, ($%x)");
				case 0x29: BYTE("OUT0", "($%x), L");
				case 0x2C: NONE("TST", "A, L");
				case 0x2F: NONE("LD", "(HL), HL");
				case 0x31: NONE("LD", "IY, HL");
				case 0x32: DISP("LEA", "IX, IX%+d");
				case 0x33: DISP("LEA", "IY, IY%+d");
				case 0x34: NONE("TST", "A, (HL)");
				case 0x37: NONE("LD", "IX, (HL)");
				case 0x38: BYTE("IN0", "A, ($%x)");
				case 0x39: BYTE("OUT0", "($%x), A");
				case 0x3C: NONE("TST", "A, A");
				case 0x3E: NONE("LD", "(HL), IY");
				case 0x3F: NONE("LD", "(HL), IX");
				case 0x40: NONE("IN", "B,(BC)");
				case 0x41: NONE("OUT", "(BC), B");
				case 0x42: NONE("SBC", "HL, BC");
				case 0x43: WORD("LD", "($%lx), BC");
				case 0x44: NONE("NEG", "");
				case 0x45: NONE("RETN", "");
				case 0x46: NONE("IM", "0");
				case 0x47: NONE("LD", "I, A");
				case 0x48: NONE("IN", "C, (BC)");
				case 0x49: NONE("OUT", "(BC), C");
				case 0x4A: NONE("ADC", "HL, BC");
				case 0x4B: WORD("LD", "BC, ($%lx)");
				case 0x4C: NONE("MLT", "BC");
				case 0x4D: NONE("RETI", "");
				case 0x4F: NONE("LD", "R, A");
				case 0x50: NONE("IN", "D, (BC)");
				case 0x51: NONE("OUT", "(BC), D");
				case 0x52: NONE("SBC", "HL, DE");
				case 0x53: WORD("LD", "($%lx), DE");
				case 0x54: DISP("LEA", "IX, IY%+d");
				case 0x55: DISP("LEA", "IY, IX%+d");
				case 0x56: NONE("IM", "1");
				case 0x57: NONE("LD", "A, I");
				case 0x58: NONE("IN", "E, (BC)");
				case 0x59: NONE("OUT", "(BC), E");
				case 0x5a: NONE("ADC", "HL, DE");
				case 0x5b: WORD("LD", "DE, ($%lx)");
				case 0x5c: NONE("MLT", "DE");
				case 0x5e: NONE("IM", "2");
				case 0x5f: NONE("LD", "A, R");
				case 0x60: NONE("IN", "H, (BC)");
				case 0x61: NONE("OUT", "(BC), H");
				case 0x62: NONE("SBC", "HL, HL");
				case 0x63: WORD("LD", "($%lx), HL");
				case 0x64: BYTE("TST", "A, $%x");
				case 0x65: DISP("PEA", "IX%+d");
				case 0x66: DISP("PEA", "IY%+d");
				case 0x67: NONE("RRD", "");
				case 0x68: NONE("IN", "L, (BC)");
				case 0x69: NONE("OUT", "(BC), L");
				case 0x6a: NONE("ADC", "HL, HL");
				case 0x6b: WORD("LD", "HL, ($%lx)");
				case 0x6c: NONE("MLT", "HL");
				case 0x6d: NONE("LD", "MB, A");
				case 0x6e: NONE("LD", "A, MB");
				case 0x6f: NONE("RLD", "");
				case 0x72: NONE("SBC", "HL, SP");
				case 0x73: WORD("LD", "($%lx), SP");
				case 0x74: BYTE("TSTIO", "$%x");
				case 0x76: NONE("SLP", "");
				case 0x78: NONE("IN", "A, (BC)");
				case 0x79: NONE("OUT", "(BC), A");
				case 0x7a: NONE("ADC", "HL, SP");
				case 0x7b: WORD("LD", "SP, ($%lx)");
				case 0x7c: NONE("MLT", "SP");
				case 0x7d: NONE("STMIX", "");
				case 0x7e: NONE("RSMIX", "");
				case 0x82: NONE("INIM", "");
				case 0x83: NONE("OTIM", "");
				case 0x84: NONE("INI2", "");
				case 0x8A: NONE("INDM", "");
				case 0x8B: NONE("OTDM", "");
				case 0x8C: NONE("IND2", "");
				case 0x92: NONE("INIMR", "");
				case 0x93: NONE("OTIMR", "");
				case 0x94: NONE("INI2R", "");
				case 0x9A: NONE("INDMR", "");
				case 0x9B: NONE("OTDMR", "");
				case 0x9C: NONE("IND2R", "");
				case 0xA0: NONE("LDI", "");
				case 0xA1: NONE("CPI", "");
				case 0xA2: NONE("INI", "");
				case 0xA3: NONE("OUTI", "");
				case 0xA4: NONE("OUTI2", "");
				case 0xA8: NONE("LDD", "");
				case 0xA9: NONE("CPD", "");
				case 0xAA: NONE("IND", "");
				case 0xAB: NONE("OUTD", "");
				case 0xAC: NONE("OUTD2", "");
				case 0xB0: NONE("LDIR", "");
				case 0xB1: NONE("CPIR", "");
				case 0xB2: NONE("INIR", "");
				case 0xB3: NONE("OTIR", "");
				case 0xB4: NONE("OTI2R", "");
				case 0xB8: NONE("LDDR", "");
				case 0xB9: NONE("CPDR", "");
				case 0xBA: NONE("INDR", "");
				case 0xBB: NONE("OTDR", "");
				case 0xBC: NONE("OTD2R", "");
				case 0xC2: NONE("INIRX", "");
				case 0xC3: NONE("OTIRX", "");
				case 0xC7: NONE("LD", "I, HL");
				case 0xCA: NONE("INDRX", "");
				case 0xCB: NONE("OTDRX", "");
				case 0xD7: NONE("LD", "HL, I");
				default: NONE("TRAP", "");
			}
		}
		case 0xee: BYTE("XOR", "A, $%x");
		case 0xef: NONE("RST", "$28");
		case 0xf0: NONE("RET", "P");
		case 0xf1: NONE("POP", "AF");
		case 0xf2: WORD("JP", "P, $%lx");
		case 0xf3: NONE("DI", "");
		case 0xf4: WORD("CALL", "P, $%lx");
		case 0xf5: NONE("PUSH", "AF");
		case 0xf6: BYTE("OR", "A, $%x");
		case 0xf7: NONE("RST", "$30");
		case 0xf8: NONE("RET", "M");
		case 0xf9: NONE("LD", "SP, HL");
		case 0xfa: WORD("JP", "M, $%lx");
		case 0xfb: NONE("EI", "");
		case 0xfc: WORD("CALL", "M, $%lx");
		case 0xfd: INDEX("IY", "IX");
		case 0xfe: BYTE("CP", "A, $%x");
		case 0xff: NONE("RST", "$38");
		default: NONE("TRAP", "");
	}
}
