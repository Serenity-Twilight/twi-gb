#include <stdint.h>
#include "gb/core/typedef.h"
#include "gb/cpu.h"
#include "gb/cpu/opc.h"
#include "gb/cpu/opc/decoder/oper.h"
#include "gb/cpu/opc/decoder/opnd.h"
#include "gb/mem.h"

//=======================================================================
//-----------------------------------------------------------------------
// Internal type definitions
//-----------------------------------------------------------------------
//=======================================================================
static const uint8_t alu_ops[] =
		{ OPER_ADD8, OPER_ADC, OPER_SUB, OPER_SBC,
			OPER_AND, OPER_XOR, OPER_OR, OPER_CP };
static const uint8_t r8mHL_opnds[] =
		{ OPND_rB, OPND_rC, OPND_rD, OPND_rE, OPND_rH, OPND_rL, OPND_mHL, OPND_rA };
static const uint8_t r16AF_opnds[] =
		{ OPND_rBC, OPND_rDE, OPND_rHL, OPND_rAF };
static const uint8_t flag_opnds[] =
		{ OPND_fNZ, OPND_fZ, OPND_fNC, OPND_fC };
typedef void (*decode_proc)(struct gb_opc_components* restrict, uint8_t);

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static inline void
decode00(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode00bbb000(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode01(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode10(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode11(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode11bbb000(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode11bbb001(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decode11bbb010(struct gb_opc_components* restrict dst, uint8_t opcode);
static inline void
decodeCB(struct gb_opc_components* restrict dst, uint8_t opcode);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================
void
gb_opc_current_components(
		struct gb_opc_components* restrict dst,
		const struct gb_core* restrict core) {
	gb_opc_components_at(dst, core, core->cpu.pc);
} // end gb_opc_current_components()

void
gb_opc_components_at(
		struct gb_opc_components* restrict dst,
		const struct gb_core* restrict core,
		uint16_t offset) {
	static const decode_proc decode_nopfx[] = {
		decode00, decode01, decode10, decode11
	};

	uint8_t opcode = gb_mem_direct_read(core, offset);
	if (opcode != 0xCB) // No prefix
		decode_nopfx[opcode >> 6](dst, opcode);
	else // CB-prefixed
		decodeCB(dst, gb_mem_direct_read(core, offset+1));
} // end gb_opc_components_at()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================
static inline void
decode00(struct gb_opc_components* restrict dst, uint8_t opcode) {
	static const uint8_t r16SP_opnds[] = // Note that 4th operand is rSP
			{ OPND_rBC, OPND_rDE, OPND_rHL, OPND_rSP };
	static const uint8_t m16_opnds[] =
			{ OPND_mBC, OPND_mDE, OPND_mHLi, OPND_mHLd };
	static const uint8_t ops00bbb111[] =
			{ OPER_RLCA, OPER_RRCA, OPER_RLA, OPER_RRA,
				OPER_DAA, OPER_CPL, OPER_SCF, OPER_CCF };

	switch (opcode & 0x7)  { // bits[0-2]
		//--------------------------------------------------------
		case 0: // Miscellaneous instructions without consistent patterns.
			decode00bbb000(dst, opcode);
			break;
		//--------------------------------------------------------
		case 1: {
			// if bits[3] == 0: LD r16, ui16
			// if bits[3] == 1: ADD HL, r16
			uint8_t r16_opnd = r16SP_opnds[(opcode >> 4) & 0x3]; // bits[4-5]
			if (!(opcode & 0x8)) { // LD r16, ui16
				dst->oper_id = OPER_LD16;
				dst->opnd1_id = r16_opnd;
				dst->opnd2_id = OPND_ui16;
			} else { // ADD HL, r16
				dst->oper_id = OPER_ADD16;
				dst->opnd1_id = OPND_rHL;
				dst->opnd2_id = r16_opnd;
			}
			break;
		}
		//--------------------------------------------------------
		case 2: {
			// if bits[3] == 0: LD m16, A 
			// if bits[3] == 1: LD A, m16
			dst->oper_id = OPER_LD8;
			uint8_t m16_opnd = m16_opnds[(opcode >> 4) & 0x3]; // bits[4-5]
			if (!(opcode & 0x8)) { // LD m16, A
				dst->opnd1_id = m16_opnd;
				dst->opnd2_id = OPND_rA;
			} else { // LD A, m16
				dst->opnd1_id = OPND_rA;
				dst->opnd2_id = m16_opnd;
			}
			break;
		}
		//--------------------------------------------------------
		case 3:
			// if bits[3] == 0: INC r16 
			// if bits[3] == 1: DEC r16
			dst->oper_id = (!(opcode & 0x8)) ? OPER_INC16 : OPER_DEC16;
			dst->opnd1_id = r16SP_opnds[(opcode >> 4) & 0x3]; // bits[4-5]
			dst->opnd2_id = OPND_NONE;
			break;
		//--------------------------------------------------------
		case 4: // INC r8/mHL
			dst->oper_id = OPER_INC8;
			dst->opnd1_id = r8mHL_opnds[(opcode >> 3) & 0x7]; // bits[3-5]
			dst->opnd2_id = OPND_NONE;
			break;
		//--------------------------------------------------------
		case 5: // DEC r8/mHL
			dst->oper_id = OPER_DEC8;
			dst->opnd1_id = r8mHL_opnds[(opcode >> 3) & 0x7]; // bits[3-5]
			dst->opnd2_id = OPND_NONE;
			break;
		//--------------------------------------------------------
		case 6: // LD r8/mHL, ui8
			dst->oper_id = OPER_LD8;
			dst->opnd1_id = r8mHL_opnds[(opcode >> 3) & 0x7]; // bits[3-5]
			dst->opnd2_id = OPND_ui8;
			break;
		//--------------------------------------------------------
		case 7: // Miscellaneous instructions with no operands
			dst->oper_id = ops00bbb111[(opcode >> 3) & 0x7]; // bits [3-5]
			dst->opnd1_id = dst->opnd2_id = OPND_NONE;
			break;
	} // end switch
} // end decode00()

//=======================================================================
// doc decode00bbb000()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object with type
// `struct gb_opc_components`.
// Behavior is undefined if bits[0-2,6-7] of `opcode` != 0.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[0,1,2,6,7] all cleared.
//=======================================================================
// def decode00bbb000()
static inline void
decode00bbb000(struct gb_opc_components* restrict dst, uint8_t opcode) {
	uint8_t bits3to4 = (opcode >> 3) & 0x3;
	if (!(opcode & 0x20)) { // bit[5] == 0, 000bb000
		switch (bits3to4) {
			case 0: // NOP
				dst->oper_id = OPER_NOP;
				dst->opnd1_id = dst->opnd2_id = OPND_NONE;
				break;
			case 1: // LD [u16], SP
				dst->oper_id = OPER_LD16;
				dst->opnd1_id = OPND_mui16;
				dst->opnd2_id = OPND_rSP;
				break;
			case 2: // STOP u8
				dst->oper_id = OPER_STOP;
				dst->opnd1_id = OPND_ui8;
				dst->opnd2_id = OPND_NONE;
				break;
			case 3: // JR s8
				dst->oper_id = OPER_JR;
				dst->opnd1_id = OPND_si8;
				dst->opnd2_id = OPND_NONE;
				break;
		} // end switch
	} else { // bit[5] == 1, 001bb000
		// JR <cond>, si8
		dst->oper_id = OPER_JR;
		dst->opnd1_id = flag_opnds[bits3to4];
		dst->opnd2_id = OPND_si8;
	} // end ifelse bit[5]
} // end decode00xx0000()

//=======================================================================
// doc decode01()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object of
// type `struct gb_opc_components`.
// Behavior is undefined if bits[6-7] of `opcode` != 1.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[6-7] == 1.
//=======================================================================
// def decode01()
static inline void
decode01(struct gb_opc_components* restrict dst, uint8_t opcode) {
	if (opcode == 0x76) { // HALT
		dst->oper_id = OPER_HALT;
		dst->opnd1_id = dst->opnd2_id = OPND_NONE;
		return;
	}

	// If opcode is not HALT, then it takes the form of:
	// LD r8/mHL, r8/mHL
	// Operand 1 is defined by bits[3-5]
	// Operand 2 is defined by bits[0-2]
	dst->oper_id = OPER_LD8;
	dst->opnd1_id = r8mHL_opnds[(opcode >> 3) & 0x7];
	dst->opnd2_id = r8mHL_opnds[opcode & 0x7];
} // end decode01()

//=======================================================================
// doc decode10()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object of
// type `struct gb_opc_components`.
// Behavior is undefined if bits[6-7] of `opcode` != 2.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[6-7] == 2.
//=======================================================================
// def decode10()
static inline void
decode10(struct gb_opc_components* restrict dst, uint8_t opcode) {
	// Opcode is an 8-bit ALU instruction with the form of:
	// <ALU> A, r8/mHL
	// The instruction is defined by bits[3-5]
	// Operand 2 is defined by bits[0-2]
	dst->oper_id = alu_ops[(opcode >> 3) & 0x7];
	dst->opnd1_id = OPND_rA;
	dst->opnd2_id = r8mHL_opnds[opcode & 0x7];
} // end decode10()

//=======================================================================
// doc decode11()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object of
// type `struct gb_opc_components`.
// Behavior is undefined if bits[6-7] of `opcode` != 3.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[6-7] == 3.
//=======================================================================
// def decode11()
static inline void
decode11(struct gb_opc_components* restrict dst, uint8_t opcode) {
	static const uint8_t rst_opnds[] =
			{ OPND_RST00, OPND_RST08, OPND_RST10, OPND_RST18,
				OPND_RST20, OPND_RST28, OPND_RST30, OPND_RST38 };

	switch (opcode & 0x7) {
		//--------------------------------------------------------
		case 0:
			decode11bbb000(dst, opcode);
			break;
		//--------------------------------------------------------
		case 1:
			decode11bbb001(dst, opcode);
			break;
		//--------------------------------------------------------
		case 2:
			decode11bbb010(dst, opcode);
			break;
		//--------------------------------------------------------
		case 3:
			dst->opnd2_id = OPND_NONE;
			if (opcode == 0xC3) { // JP ui16
				dst->oper_id = OPER_JP;
				dst->opnd1_id = OPND_ui16;
			} else {
				dst->opnd1_id = OPND_NONE;
				if (opcode == 0xF3) // DI
					dst->oper_id = OPER_DI;
				else if (opcode == 0xFB) // EI
					dst->oper_id = OPER_EI;
				else
					dst->oper_id = OPER_INVALID;
			} // end ifelse opcode == JP ui16
			break;
		//--------------------------------------------------------
		case 4:
			if (!(opcode & 0x20)) { // if bit[5] == 0
				// CALL <cond>, ui16
				dst->oper_id = OPER_CALL;
				dst->opnd1_id = flag_opnds[(opcode >> 3) & 0x3]; // bits[3-4]
				dst->opnd2_id = OPND_ui16;
			} else { // if bit[5] == 1
				// Invalid opcode (no associated instruction)
				dst->oper_id = OPER_INVALID;
				dst->opnd1_id = OPND_NONE;
				dst->opnd2_id = OPND_NONE;
			} // end ifelse bit[5]
			break;
		//--------------------------------------------------------
		case 5:
			dst->opnd2_id = OPND_NONE;
			if (!(opcode & 0x08)) { // if bit[3] == 0
				// PUSH r16
				dst->oper_id = OPER_PUSH;
				dst->opnd1_id = r16AF_opnds[(opcode >> 4) & 0x3]; // bits[4-5]
			} else if (opcode == 0xCD) { // CALL ui16
				dst->oper_id = OPER_CALL;
				dst->opnd1_id = OPND_ui16;
			} else { // Invalid opcode (no associated instruction)
				dst->oper_id = OPER_INVALID;
				dst->opnd1_id = OPND_NONE;
			} // end ifelseif chain
			break;
		//--------------------------------------------------------
		case 6: // <ALU> A, ui8
			dst->oper_id = alu_ops[(opcode >> 3) & 0x7]; // bits [3-5]
			dst->opnd1_id = OPND_rA;
			dst->opnd2_id = OPND_ui8;
			break;
		//--------------------------------------------------------
		case 7: // RST $XX
			dst->oper_id = OPER_RST;
			dst->opnd1_id = rst_opnds[(opcode >> 3) & 0x7]; // bits [3-5]
			dst->opnd2_id = OPND_NONE;
			break;
	} // end switch (bits[0-2])
} // end decode03()

//=======================================================================
// doc decode11bbb000()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object with type
// `struct gb_opc_components`.
// Behavior is undefined if bits[0-2] of `opcode` != 0.
// Behavior is undefined if bits[6-7] of `opcode` != 3.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[0-2] == 0 && bits[6-7] == 3.
//=======================================================================
// def decode11bbb000()
static inline void
decode11bbb000(struct gb_opc_components* restrict dst, uint8_t opcode) {
	uint8_t bits3to4 = (opcode >> 3) & 0x3;
	if (!(opcode & 0x20)) { // if bit[5] == 0, 110bb000
		// RET <cond>
		dst->oper_id = OPER_RET;
		dst->opnd1_id = flag_opnds[bits3to4];
		dst->opnd2_id = OPND_NONE;
	} else { // if bit[5] == 1, 111bb000
		switch (bits3to4) {
			case 0: // LD [$FF00|ui8], A
				dst->oper_id = OPER_LD8;
				dst->opnd1_id = OPND_mui8;
				dst->opnd2_id = OPND_rA;
				break;
			case 1: // ADD SP, si8
				dst->oper_id = OPER_ADD16;
				dst->opnd1_id = OPND_rSP;
				dst->opnd2_id = OPND_si8;
				break;
			case 2: // LD A, [$FF00|ui8]
				dst->oper_id = OPER_LD8;
				dst->opnd1_id = OPND_rA;
				dst->opnd2_id = OPND_mui8;
				break;
			case 3: // LD HL, SP+si8
				dst->oper_id = OPER_LD16;
				dst->opnd1_id = OPND_rHL;
				dst->opnd2_id = OPND_si8rSP;
				break;
		} // switch (bits[3-4])
	} // end ifelse bit[5]
} // end decode11bbb000()

//=======================================================================
// doc decode11bbb001()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object with type
// `struct gb_opc_components`.
// Behavior is undefined if bits[0-2] of `opcode` != 1.
// Behavior is undefined if bits[6-7] of `opcode` != 3.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[0-2] == 1 && bits[6-7] == 3.
//=======================================================================
// def decode11bbb001()
static inline void
decode11bbb001(struct gb_opc_components* restrict dst, uint8_t opcode) {
	uint8_t bits4to5 = (opcode >> 4) & 0x3;
	if (!(opcode & 0x08)) { // if bit[3] == 0: 11bb0001
		// POP r16
		dst->oper_id = OPER_POP;
		dst->opnd1_id = r16AF_opnds[bits4to5];
		dst->opnd2_id = OPND_NONE;
	} else { // if bit[3] == 1: 11bb1001
		switch (bits4to5) {
			case 0: // RET
				dst->oper_id = OPER_RET;
				dst->opnd1_id = dst->opnd2_id = OPND_NONE;
				break;
			case 1: // RETI
				dst->oper_id = OPER_RETI;
				dst->opnd1_id = dst->opnd2_id = OPND_NONE;
				break;
			case 2: // JP HL
				dst->oper_id = OPER_JP;
				dst->opnd1_id = OPND_rHL;
				dst->opnd2_id = OPND_NONE;
				break;
			case 3: // LD SP, HL
				dst->oper_id = OPER_LD16;
				dst->opnd1_id = OPND_rSP;
				dst->opnd2_id = OPND_rHL;
				break;
		} // end switch (bits[4-5])
	} // end ifelse bit[3]
} // end decode11bbb001()

//=======================================================================
// doc decode11bbb010()
// Decodes `opcode` and writes its instruction ID and operand IDs to
// `dst`.
//
// Behavior is undefined if `dst` does not point to an object with type
// `struct gb_opc_components`.
// Behavior is undefined if bits[0-2] of `opcode` != 2.
// Behavior is undefined if bits[6-7] of `opcode` != 3.
//-----------------------------------------------------------------------
// Parameters:
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU opcode with bits[0-2] == 2 && bits[6-7] == 3.
//=======================================================================
// def decode11bbb010()
static inline void
decode11bbb010(struct gb_opc_components* restrict dst, uint8_t opcode) {
	uint8_t bits3to4 = (opcode >> 3) & 0x3;
	if (!(opcode & 0x20)) { // if bit[5] == 0
		// JP <cond>, ui16
		dst->oper_id = OPER_JP;
		dst->opnd1_id = flag_opnds[bits3to4];
		dst->opnd2_id = OPND_ui16;
	} else { // if bit[5] == 1
		dst->oper_id = OPER_LD8;
		switch (bits3to4) {
			case 0: // LD [C], A
				dst->opnd1_id = OPND_mC;
				dst->opnd2_id = OPND_rA;
				break;
			case 1: // LD [ui16], A
				dst->opnd1_id = OPND_mui16;
				dst->opnd2_id = OPND_rA;
				break;
			case 2: // LD A, [C]
				dst->opnd1_id = OPND_rA;
				dst->opnd2_id = OPND_mC;
				break;
			case 3: // LD A, [ui16]
				dst->opnd1_id = OPND_rA;
				dst->opnd2_id = OPND_mui16;
				break;
		} // end switch (bits[3-4])
	} // end ifelse bit[5]
} // end decode11bbb010()

//=======================================================================
// doc decodeCB()
// Decodes the CB-prefixed `opcode` and writes its instruction ID and
// operand IDs to `dst`.
//
// NOTE: `opcode` should be the byte immediately following a 0xCB prefix
// byte. Behavior is undefined if this is not true.
// Behavior is undefined if `dst` does not point to an object with type
// `struct gb_opc_components`.
//-----------------------------------------------------------------------
// * dst: Destination for decoded opcode instruction and operand
//        identifiers.
// * opcode: 8-bit GB CPU CB-prefixed opcode.
//=======================================================================
static inline void
decodeCB(struct gb_opc_components* restrict dst, uint8_t opcode) {
	static const uint8_t shift_ops[] =
			{ OPER_RLC, OPER_RRC, OPER_RL, OPER_RR,
				OPER_SLA, OPER_SRA, OPER_SWAP, OPER_SRL };
	static const uint8_t bit_ops[] =
			{ OPER_BIT, OPER_RES, OPER_SET };
	static const uint8_t bit_opnds[] =
			{ OPND_b0, OPND_b1, OPND_b2, OPND_b3,
			  OPND_b4, OPND_b5, OPND_b6, OPND_b7 };

	uint8_t b0to2 = opcode & 0x7;
	uint8_t b3to5 = (opcode >> 3) & 0x7;
	if (opcode < 0x40) { // if bits[6-7] == 0
		// <shift-op> r8/mHL
		dst->oper_id = shift_ops[b3to5];
		dst->opnd1_id = r8mHL_opnds[b0to2];
		dst->opnd2_id = OPND_NONE;
	} else { // if bits[6-7] != 0
		// <bit-op> b, r8/mHL
		uint8_t b6to7 = (opcode >> 6) & 0x3;
		dst->oper_id = bit_ops[b6to7 - 1];
		dst->opnd1_id = bit_opnds[b3to5];
		dst->opnd2_id = r8mHL_opnds[b0to2];
	} // end ifelse bits[6-7]
} // end decodeCB()

