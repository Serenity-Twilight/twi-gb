#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/cpu/opc.h"
#include "gb/cpu/opc/decoder/oper.h"
#include "gb/cpu/opc/decoder/opnd.h"
#include "gb/cpu/reg.h"
#include "gb/mem.h"

//=======================================================================
//-----------------------------------------------------------------------
// Internal type definitions
//-----------------------------------------------------------------------
//=======================================================================
struct write_dst {
	char* buf;
	size_t pos;
	size_t size;
}; // end struct write_dst

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
//static size_t
//format_operand(
//		char* restrict dst, size_t dstsz,
//		enum gb_opc_opnd_id src,
//		const struct gb_core* restrict core);
static const char*
oper_string(enum gb_opc_oper_id id);
//static const char*
//opnd_string(enum gb_opc_opnd_id id);
static void
prefix_byteno(struct write_dst* restrict dst, uint16_t byteno);
static void
prefix_opbytes(
		struct write_dst* restrict dst,
		const struct gb_opc_components* restrict opc,
		const struct gb_core* restrict core);
static inline void
dst_puts(struct write_dst* restrict dst, const char* src);
static inline void
dst_putc(struct write_dst* restrict dst, char src);
static inline void
dst_putsdec(
		struct write_dst* restrict dst,
		int64_t value, uint8_t always_sign);
static inline char
hexchar(uintmax_t number);
static inline void
dst_puthex(
		struct write_dst* restrict dst,
		uintmax_t value,
		size_t output_length);
static inline void
dst_putopnd(
		struct write_dst* restrict dst,
		enum gb_opc_opnd_id opnd,
		const struct gb_core* restrict core);
static inline uint8_t
r8_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd);
static inline uint8_t
flag_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd);
static inline uint16_t
r16_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd);
static inline uint8_t
u8_value(const struct gb_core* restrict core);
static inline int8_t
s8_value(const struct gb_core* restrict core);
static inline uint16_t
u16_value(const struct gb_core* restrict core);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================
size_t
gb_opc_string(
		char* restrict buf, size_t bufsz,
		const struct gb_opc_components* restrict opc,
		const struct gb_core* restrict core) {
	struct write_dst dst = {
		.buf = buf,
		.pos = 0,
		.size = bufsz
	};
	if (core != NULL) {
		// Write opcode's memory byte offset
		prefix_byteno(&dst, core->cpu.pc);
		// Write opcode and immediate bytes
		prefix_opbytes(&dst, opc, core);
	}
	// Write disassembled operation name
	dst_puts(&dst, oper_string(opc->oper_id));
	//printf("dst.pos (after name) = %zu\n", dst.pos);

	if (opc->opnd1_id == OPND_NONE)
		goto end;
	dst_putc(&dst, ' ');
	// Write disassembled lhs operand
	dst_putopnd(&dst, opc->opnd1_id, core);
	//printf("dst.pos (after opnd1) = %zu\n", dst.pos);

	if (opc->opnd2_id == OPND_NONE)
		goto end;
	dst_puts(&dst, ", ");
	// Write disassembled rhs operand
	dst_putopnd(&dst, opc->opnd2_id, core);
	//printf("dst.pos (after opnd2) = %zu\n", dst.pos);

end:
	if (dst.pos >= dst.size) {
		dst.buf[dst.size - 1] = 0; // NUL
		return dst.pos;
	} else {
		dst_putc(&dst, 0); // NUL
		return dst.pos - 1; // Number of characters minus NUL.
	}
} // end gb_opc_string()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================
//static size_t
//format_operand(
//		char* write_dst* restrict dst,
//		enum gb_opc_opnd_id src,
//		const struct gb_core* restrict core) {
// 	// TODO: Output live core values
//
//	size_t bytes_written = 0;
//	if (src == OPND_NONE)
//		goto end;
//
//	const char* opnd = opnd_string(src);
//	if (dstsz > 0)
//		strncpy(dst, opnd, dstsz);
//	bytes_written += strlen(opnd);
//end:
//	if (dstsz > 0)
//		dst[dstsz-1] = 0; // Ensure NUL-termination of `dst`.
//	return bytes_written;
//} // end format_operand()

#define TOSTR(id) case OPER_##id: return #id
static const char*
oper_string(enum gb_opc_oper_id id) {
	switch (id) {
		TOSTR(NOP);
		TOSTR(STOP);
		TOSTR(HALT);
		TOSTR(DI);
		TOSTR(EI);
		TOSTR(JP);
		TOSTR(JR);
		TOSTR(CALL);
		TOSTR(RST);
		TOSTR(RET);
		TOSTR(RETI);
		case OPER_LD8: return "LD";
		case OPER_ADD8: return "ADD";
		TOSTR(ADC);
		TOSTR(SUB);
		TOSTR(SBC);
		TOSTR(AND);
		TOSTR(XOR);
		TOSTR(OR);
		TOSTR(CP);
		case OPER_INC8: return "INC";
		case OPER_DEC8: return "DEC";
		TOSTR(RLCA);
		TOSTR(RRCA);
		TOSTR(RLA);
		TOSTR(RRA);
		TOSTR(DAA);
		TOSTR(CPL);
		TOSTR(SCF);
		TOSTR(CCF);
		case OPER_LD16: return "LD";
		TOSTR(PUSH);
		TOSTR(POP);
		case OPER_ADD16: return "ADD";
		case OPER_INC16: return "INC";
		case OPER_DEC16: return "DEC";
		TOSTR(RLC);
		TOSTR(RRC);
		TOSTR(RL);
		TOSTR(RR);
		TOSTR(SLA);
		TOSTR(SRA);
		TOSTR(SWAP);
		TOSTR(SRL);
		TOSTR(BIT);
		TOSTR(RES);
		TOSTR(SET);
		default: return "INVALID";
	} // end switch (id)
} // end oper_string()
#undef TOSTR

static void
prefix_byteno(struct write_dst* restrict dst, uint16_t byteno) {
	dst_putc(dst, 'L');
	dst_puthex(dst, byteno, 4);
	dst_puts(dst, " : ");
} // end prefix_byteno()

static void
prefix_opbytes(
		struct write_dst* restrict dst,
		const struct gb_opc_components* restrict opc,
		const struct gb_core* restrict core) {
	uint_fast8_t byte_count = 1;
	if (opc->opnd1_id & OPND_IMMED && opc->opnd1_id & OPND_AFTER)
		byte_count += (opc->opnd1_id & OPND_16BIT ? 2 : 1);
	else if (opc->opnd2_id & OPND_IMMED && opc->opnd2_id & OPND_AFTER)
		byte_count += (opc->opnd2_id & OPND_16BIT ? 2 : 1);

	// Write opcode bytes to dst.
	// Put spaces between each byte (2 hex digits).
	uint16_t offset = core->cpu.pc;
	uint_fast8_t byte_pos;
	for (byte_pos = 0; byte_pos < byte_count; ++byte_pos) {
		dst_puthex(dst, gb_mem_direct_read(core, offset++), 2);
		dst_putc(dst, ' ');
	}
	// For instructions with less than 3 bytes (which is most of them), pad
	// out the unused space with spaces, to keep all of the opcode printing
	// horizontally aligned and easy-to-read.
	static const uint_fast8_t byte_pos_count = 3;
	for (; byte_pos < byte_pos_count; ++byte_pos)
		dst_puts(dst, "   ");
	dst_puts(dst, ": ");
} // end prefix_opbytes()

static inline void
dst_puts(struct write_dst* restrict dst, const char* src) {
	if (dst->pos < dst->size)
		strncpy(dst->buf + dst->pos, src, dst->size - dst->pos);
	dst->pos += strlen(src);
} // end dst_puts()

static inline void
dst_putc(struct write_dst* restrict dst, char src) {
	if (dst->pos < dst->size)
		dst->buf[dst->pos] = src;
	dst->pos += 1;
} // end dst_putc()

static inline void
dst_putsdec(
		struct write_dst* restrict dst,
		int64_t value, uint8_t always_sign) {
	uint8_t negative = (value < 0);
	if (negative)
		value = -value;

	// Convert value digits into characters, from least-significant digit
	// to most significant digit. Store into acc.
	char acc[32]; // 2^63 has 19 digits, well within range
	uint_fast8_t i = 0;
	if (value == 0) {
		acc[i++] = '0';
	} else {
		do {
			acc[i++] = (value % 10) + '0';
			value /= 10;
		} while (value != 0);
	}

	// Sign
	if (negative)
		dst_putc(dst, '-');
	else if (always_sign)
		dst_putc(dst, '+');
	// Unroll acc into dst
	for (; i > 0; --i)
		dst_putc(dst, acc[i-1]);
} // end dst_putsdec()

static inline char
hexchar(uintmax_t number) {
	number &= 0xF;
	if (number < 0xA)
		return number + '0';
	else
		return number + ('A' - 0xA);
} // end hexchar()

static inline void
dst_puthex(
		struct write_dst* restrict dst,
		uintmax_t value,
		size_t output_length) {
	size_t shift = 4 * output_length;
	while (shift > 0) {
		shift -= 4;
		dst_putc(dst, hexchar(value >> shift));
	}
} // end dst_puthex()

static inline void
dst_putopnd(
		struct write_dst* restrict dst,
		enum gb_opc_opnd_id opnd,
		const struct gb_core* restrict core) {
	static const char reg_char[] =
			{ 'B', 'C', 'D', 'E', 'H', 'L', 'A', 'F', 'S', 'P' };
	static const char flag_char[] = { 'C', 'H', 'N', 'Z' };

	if (opnd & OPND_PTR)
		dst_putc(dst, '[');

	uint16_t ptr_addr;
	if (!(opnd & OPND_IMMED)) { // register/flag
		if (!(opnd & OPND_16BIT)) { // 8-bit
			if (!(opnd & OPND_FLAG)) {// 8-bit register
				dst_putc(dst, reg_char[opnd & 0x7]);
				if (core != NULL) { // Report live value if possible
					uint8_t val = r8_value(core, opnd);
					ptr_addr = 0xFF00 + val;
					dst_putc(dst, '=');
					dst_puthex(dst, val, 2);
				}
			} else { // flag
				if (opnd & OPND_INVERT)
					dst_putc(dst, 'N');
				dst_putc(dst, flag_char[opnd & 0x3]);
				if (core != NULL) { // Report live value if possible
					ptr_addr = 0;
					dst_putc(dst, '=');
					dst_putc(dst, flag_value(core, opnd) ? '1' : '0');
				}
			}
		} else { // 16-bit
			uint_fast8_t reg_char_offset = (opnd & 0x7) * 2;
			dst_putc(dst, reg_char[reg_char_offset]);
			dst_putc(dst, reg_char[reg_char_offset+1]);
			if (opnd & OPND_MODIFY) // operand is increment/decremented after use
				dst_putc(dst, (opnd & OPND_INCR ? '+' : '-'));
			if (core != NULL) { // Report live value if possible
				ptr_addr = r16_value(core, opnd);
				dst_putc(dst, '=');
				dst_puthex(dst, ptr_addr, 4);
			}
		} // end ifelse 8-bit or 16-bit
	} else { // immediate
		if (!(opnd & OPND_AFTER)) { // immediate within opcode
			uint_fast8_t immed = opnd & 0x7;
			if (!(opnd & OPND_IS_RST)) // bit immediate
				dst_putc(dst, immed + '0');
			else { // address offset immediate
				dst_putc(dst, '$'); // Literal hex value prefix
				dst_puthex(dst, immed * 8, 2);
			}
		} else { // immediate after opcode
			if (core == NULL) { // use placeholders for immediates
				if (opnd & OPND_ADD_SP)
					dst_puts(dst, "SP+");
				dst_putc(dst, (!(opnd & OPND_SIGNED) ? 'u' : 's'));
				dst_puts(dst, (!(opnd & OPND_16BIT) ? "8" : "16"));
			} else { // use live values for immediates
				if (opnd & OPND_ADD_SP) {
					dst_puts(dst, "SP=");
					dst_puthex(dst, r16_value(core, OPND_rSP), 4);
				}

				if (!(opnd & OPND_16BIT)) { // 8-bit
					if (!(opnd & OPND_SIGNED)) { // unsigned
						// Prefix with '$' to indicate literal hex value.
						dst_putc(dst, '$');
						uint8_t val = u8_value(core);
						if (!(opnd & OPND_PTR)) // literal
							dst_puthex(dst, val, 2);
						else { // pointer
							ptr_addr = 0xFF00 + val;
							dst_puthex(dst, ptr_addr, 4);
						} // end ifelse literal/pointer
					} else // signed
						dst_putsdec(dst, s8_value(core), opnd & OPND_ADD_SP);
					// end ifelse un/signed
				} else { // 16-bit
					ptr_addr = u16_value(core);
					// Prefix with '$' to indicate literal hex value.
					dst_putc(dst, '$');
					dst_puthex(dst, ptr_addr, 4);
				} // end ifelse 8/16-bit
			} // end ifelse core isn't/is available for reading live values
		} // end ifelse immediate within or after opcode
	} // end ifelse register/flag or immediate
	
	if (opnd & OPND_PTR) {
		dst_putc(dst, ']');
		if (core != NULL) { // report live value pointed to by ptr_addr
			dst_putc(dst, '=');
			dst_puthex(dst, gb_mem_direct_read(core, ptr_addr), 2);
		}
	} // end if pointer then close
} // end dst_putopnd()

static inline uint8_t
r8_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd) {
	static const uint8_t r8ids[] = { iB, iC, iD, iE, iH, iL, iA, iF };
	return core->cpu.r[r8ids[opnd & 0x7]];
} // end r8_value()

static inline uint8_t
flag_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd) {
	switch (opnd & 0x3) {
		case 0: return core->cpu.fc; // Carry
		case 1: return core->cpu.fh; // Half-carry
		case 2: return core->cpu.fn; // Negative
		case 3: return core->cpu.fz; // Zero
	} // end switch
} // end flag_value()

static inline uint16_t
r16_value(
		const struct gb_core* restrict core,
		enum gb_opc_opnd_id opnd) {
	static const uint8_t r16ids[] = { iBC, iDE, iHL, iAF };
	if ((opnd & (OPND_16BIT | 4)) == OPND_rSP)
		return core->cpu.sp;
	else
		return r16(r16ids[opnd & 0x3]);
} // end r16_value()

static inline uint8_t
u8_value(const struct gb_core* restrict core) {
	return gb_mem_direct_read(core, core->cpu.pc+1);
} // end u8_value()

static inline int8_t
s8_value(const struct gb_core* restrict core) {
	uint8_t val = gb_mem_direct_read(core, core->cpu.pc+1);
	return *((int8_t*)(&val));
} // end s8_value()

static inline uint16_t
u16_value(const struct gb_core* restrict core) {
	uint8_t lsbyte = gb_mem_direct_read(core, core->cpu.pc+1);
	uint8_t msbyte = gb_mem_direct_read(core, core->cpu.pc+2);
	return (msbyte << 8) | lsbyte;
} // end u16_value()

//static const char*
//opnd_string(enum gb_opc_opnd_id id) {
//	switch (id) {
//		case OPND_NONE: return "";
//		// 8-bit registers
//		case OPND_rB: return "B";
//		case OPND_rC: return "C";
//		case OPND_rD: return "D";
//		case OPND_rE: return "E";
//		case OPND_rH: return "H";
//		case OPND_rL: return "L";
//		case OPND_rA: return "A";
//		// 16-bit registers
//		case OPND_rBC: return "BC";
//		case OPND_rDE: return "DE";
//		case OPND_rHL: return "HL";
//		case OPND_rSP: return "SP";
//		case OPND_rAF: return "AF";
//		// Bit offsets
//		case OPND_b0: return "0";
//		case OPND_b1: return "1";
//		case OPND_b2: return "2";
//		case OPND_b3: return "3";
//		case OPND_b4: return "4";
//		case OPND_b5: return "5";
//		case OPND_b6: return "6";
//		case OPND_b7: return "7";
//		// RST addresses
//		case OPND_RST00: return "$00";
//		case OPND_RST08: return "$08";
//		case OPND_RST10: return "$10";
//		case OPND_RST18: return "$18";
//		case OPND_RST20: return "$20";
//		case OPND_RST28: return "$28";
//		case OPND_RST30: return "$30";
//		case OPND_RST38: return "$38";
//		// Flags
//		case OPND_fC: return "C";
//		case OPND_fNC: return "NC";
//		case OPND_fZ: return "Z";
//		case OPND_fNZ: return "NZ";
//		// 8-bit register pointers
//		case OPND_mC: return "[C]";
//		// 16-bit register pointers
//		case OPND_mBC: return "[BC]";
//		case OPND_mDE: return "[DE]";
//		case OPND_mHL: return "[HL]";
//		case OPND_mHLd: return "[HL-]";
//		case OPND_mHLi: return "[HL+]";
//		// External immediate operands
//		case OPND_ui8: return "u8";
//		case OPND_si8: return "s8";
//		case OPND_si8rSP: return "SP+s8";
//		case OPND_mui8: return "[u8]";
//		case OPND_ui16: return "u16";
//		case OPND_mui16: return "[u16]";
//
//		default: return "INVALID";
//	} // end switch (id)
//} // end opnd_string()

