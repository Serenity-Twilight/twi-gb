#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include "gb/cpu/interpreter/debug.h"
#include "gb/cpu/reg.h"
#include "gb/mem.h"

#define SELF (core->cpu)

// Increments `ptr` by the quantity returned by `proc`.
// Checks and handles formatting errors and running out of space at `ptr`.
#define INCREMENT_PTR(ptr, ptrstart, ptrend, proc) { \
	int bytes_written = proc; \
	if (bytes_written < 0) \
		return ((size_t)-1); \
	(ptr) += bytes_written; \
	if ((ptr) >= (ptrend)) \
		return (ptrend) - (ptrstart); \
}

#define RETURN_NZ(call) { \
	uint_fast8_t retval = call; \
	if (retval) return retval; \
}

struct operand8 {
	char* name;
	uint8_t id;
	uint8_t (*value)(
			const struct gb_core* restrict core,
			uint8_t id);
};

struct operand8 arg_order8[] = {
	{"B", iB, r8_value},
	{"C", iC, r8_value},
	{"D", iD, r8_value},
	{"E", iE, r8_value},
	{"H", iH, r8_value},
	{"L", iL, r8_value},
	{"[HL]", iHL, r16m8_value},
	{"A", iA, r8_value},
}; // end order8[]

struct gb_dasm_op {
	uint16_t addr;
	uint8_t len;
	uint8_t bytes[3];
	uint8_t nameid;
	uint8_t arg1len;
	uint8_t arg1id;
	uint8_t arg2len;
	uint8_t arg2id;
	uint8_t flags;
}; // end struct gb_dasm_op

enum gb_dasm_op_nameid {
	DASM_UNKNOWN,
	// Loads
	DASM_LD, DASM_PUSH, DASM_POP,
	// Arithmetic / Logical
	DASM_ADD, DASM_ADC, DASM_INC,
	DASM_SUB, DASM_SBC, DASM_DEC,
	DASM_AND, DASM_XOR, DASM_OR, DASM_CP,
	DASM_DAA, DASM_CPL, DASM_SCF, DASM_CCF,
	// Rotation / Shift
	DASM_RLC, DASM_RRC, DASM_RL, DASM_RR,
	DASM_SLA, DASM_SRA, DASM_SWAP, DASM_SRL,
	// Bit fetch / manipulation
	DASM_BIT, DASM_RES, DASM_SET,
	// Branching
	DASM_JP, DASM_JR, DASM_CALL, DASM_RET, DASM_RETI, DASM_RST,
	// Misc / Control
	DASM_NOP, DASM_STOP, DASM_HALT, DASM_DI, DASM_EI
}; // end enum gb_dasm_op_nameid

enum gb_dasm_op_argid {
	DASM_NONE,
	// 1-byte registers
	DASM_B, DASM_C, DASM_D, DASM_E,
	DASM_H, DASM_L, DASM_HLptr, DASM_A,
	// Bits
	DASM_BIT0, DASM_BIT1, DASM_BIT2, DASM_BIT3,
	DASM_BIT4, DASM_BIT5, DASM_BIT6, DASM_BIT7
	// Flags
	DASM_FZ, DASM_FN, DASM_FH, DASM_FC,
	DASM_ENDOF_1BYTE_ARGS,
	// 2-byte register pairs
	DASM_BC = DASM_ENDOF_1BYTE_ARGS,
	DASM_DE, DASM_HL, DASM_AF, DASM_SP,
	// Stack pointer register variations
	DASM_SPR, // Stack Pointer Relative (+s8)
} // end enum gb_dasm_op_argid

#define MEMu8(addr) gb_mem_direct_read(core, addr)

void
gb_cpu_opstr(
		const struct gb_core* restrict core,
		size_t dstsz, char* dst,
		uint16_t inaddr) {
	uint8_t opcode = gb_mem_direct_read(core, inaddr);
} // end gb_cpu_opstr()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================
// def CB_opstring()
static void
CB_opstr00(
		const struct gb_core* restrict core,
		size_t dstsz, char* dst) {
	uint8_t opcode = MEMu8(SELF.pc+1);
	} // end switch (opcode)
} // end CB_opstring()

static void
opstr00(
		const struct gb_core* restrict core,
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint16_t inaddr,
		uint8_t opcode) {
} // end opstr00()

static uint_fast8_t
opstr01(
		const struct gb_core* restrict core,
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint16_t instr_addr,
		uint8_t opcode) {
	// Print opcode
	RETURN_NZ(codestr(dst, dstend, 1, &opcode));

	if (opcode == 0x76) { // HALT
		alt_strncpy(dst, dstend, "HALT");
		return 0;
	}

	// Copy LD to dst
	alt_strncpy(dst, dstend, "LD ");
	// Decode bits 3-5 to print first operand.
	RETURN_NZ(r8mHLfmt(core, dst, dstend, (opcode >> 3) & 0x7));
	// Decode bits 0-2 to print second operand.
	RETURN_NZ(r8mHLfmt(core, dst, dstend, opcode & 0x7));
	return 0;
} // end opstr01()

static void
opstr10(
		const struct gb_core* restrict core,
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint16_t inaddr,
		uint8_t opcode) {
	// Print opcode
	RETURN_NZ(codestr(dst, dstend, 1, &opcode));
	// Print ALU instruction string and first operand, which is always A.
	const char* instr = ALUstr((opcode >> 3) & 0x7);
	RETURN_NZ(alt_snprintf(dst, dstend,
			"%s A=%" PRIX8 ", ", instr, rA));
	// Print second operand.
	RETURN_NZ(r8mHLfmt(core, dst, dstend, opcode & 0x7));
	return 0;
} // end opstr10()

static void
opstr11(
		const struct gb_core* restrict core,
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint16_t inaddr,
		uint8_t opcode) {
} // end opstr11()

static uint_fast8_t
codestr(
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint8_t instrlen,
		uint8_t* instr) {
	if (*dst >= dstend)
		return 0;

	switch (instrlen) {
		case 1:
			RETURN_NZ(alt_snprintf(dst, dstend, "%" PRIX8 " : ",
						instr[0]));
			break;
		case 2:
			RETURN_NZ(alt_snprintf(dst, dstend, "%" PRIX8 " %" PRIX8 " : ",
						instr[0], instr[1]));
			break;
		case 3:
			RETURN_NZ(alt_snprintf(dst, dstend,
					"%" PRIX8 " %" PRIX8 " %" PRIX8 " : ",
					instr[0], instr[1], instr[2]));
			break;
		default:
			alt_strncpy(*dst, dstend, "ERR : ");
			break;
	} // end instrlen()
	return 0;
} // end codestr()

static const char*
r8mHLstr(uint8_t identifier) {
	switch (identifier) {
		case 0: return "B";
		case 1: return "C";
		case 2: return "D";
		case 3: return "E";
		case 4: return "H";
		case 5: return "L";
		case 6: return "[HL]";
		case 7: return "A";
		default: return "ERR";
	}
} // end r8mHLstr()

static uint_fast8_t
r8mHLfmt(
		const struct gb_core* restrict core,
		char* restrict * restrict dst,
		const char* restrict dstend,
		uint8_t identifier) {
	static const uint8_t r8order = { iB, iC, iD, iE, iH, iL, 0, iA };
	if (*dst >= dstend)
		return 0;

	const char* opnd_str = r8mHLstr(identifier);
	if (identifier != 6) {
		assert(identifier < sizeof(r8order));
		RETURN_NZ(alt_snprintf(dst, dstend,
				"%s=%" PRIX8, opnd_str, r8(r8order[identifier])));
	} else {
		uint8_t memval = MEMu8(rHL);
		RETURN_NZ(alt_snprintf(dst, dstend,
				"%s=[%" PRIX16 "]=%" PRIX8, opnd_str, rHL, memval));
	}
	return 0;
} // end r8mHLfmt()

static const char*
ALUstr(uint8_t identifier) {
	switch (identifier) {
		case 0: return "ADD";
		case 1: return "ADC";
		case 2: return "SUB";
		case 3: return "SBC";
		case 4: return "AND";
		case 5: return "XOR";
		case 6: return "OR";
		case 7: return "CP";
		default: return "ERR";
	}
} // end ALUstr()

static inline void
alt_strncpy(
		char* restrict * restrict dst,
		const char* restrict dstend,
		const char* restrict src) {
	if (*dst >= dstend)
		return;
	strncpy(*dst, src, dstend - *dst);
	*dst += strlen(src);
} // end alt_strncpy()

static inline uint_fast8_t
alt_snprintf(
		char* restrict * restrict dst,
		const char* restrict dstend,
		const char* restrict format,
		...) {
	if (*dst >= dstend)
		return 0;

	va_list args;
	va_start(args, format);
	int bytes_written = vsnprintf(*dst, dstend - *dst, format, args);
	va_end(args);
	if (bytes_written < 0)
		return 1;
	*dst += bytes_written;
	return 0;
} // end alt_snprintf()

