#include <assert.h>
#include <stdio.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/cpu/opc.h"
#include "gb/cpu/reg.h"
#include "gb/log.h"
#include "gb/mem.h"
#include "gb/mem/io.h"
#include "gb/sch.h"

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static inline uint8_t
call_isr(struct gb_core* restrict core);
//static inline uint8_t
//perform_halt(struct gb_core* restrict core);
static inline void
interpret_once(struct gb_core* restrict core);
static inline void
CB_interpret_once(struct gb_core* restrict core);
static inline void
execute_EI(struct gb_core* restrict core);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================
// def interpret_frame()
void
gb_cpu_interpret_frame(struct gb_core* restrict core) {
	while (1) {
		while (!(core->cpu.state))
			interpret_once(core);
		if (core->cpu.state & CPUSTATE_INTERRUPTED) {
			LOGD("Interrupt reported. Calling interrupt service routine...");
			if (call_isr(core) == IO_IFE_VBLANK) {
				LOGT("Returning.");
				return;
			}
		} else if (core->cpu.state & CPUSTATE_HALTED) {
			while (!gb_mem_io_pending_interrupts(core))
				gb_sch_advance(core, 1);
			core->cpu.state &= ~CPUSTATE_HALTED;
		}
	}
} // end interpret_frame()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================

#undef CORE
#define CORE core
#undef CPU
#define CPU (CORE->cpu)
#undef MEM
#define MEM (CORE->mem)
#define READ_MEMu8(addr) gb_mem_u8read(CORE, addr)
#define READ_MEMs8(addr) gb_mem_s8read(CORE, addr)
#define READ_MEMu16(addr) gb_mem_u16read(CORE, addr)
#define READ_MEMFF(addr) gb_mem_u8readff(CORE, addr)
#define WRITE_MEMu8(addr, value) gb_mem_u8write(CORE, addr, value)
#define WRITE_MEMu16(addr, value) gb_mem_u16write(CORE, addr, value)
#define WRITE_MEMFF(addr, value) gb_mem_u8writeff(CORE, addr, value)

// Flag bit position constants
enum flag_bit_position {
	fZpos = 7,
	fNpos = 6,
	fHpos = 5,
	fCpos = 4
};

//=======================================================================
//=======================================================================
// def pack_flags()
static inline void
pack_flags(struct gb_cpu* restrict cpu) {
	assert(cpu->fz <= 1);
	assert(cpu->fn <= 1);
	assert(cpu->fh <= 1);
	assert(cpu->fc <= 1);

	cpu->r[iF] = (cpu->fz << fZpos)
	           | (cpu->fn << fNpos)
	           | (cpu->fh << fHpos)
	           | (cpu->fc << fCpos);
} // end pack_flags()

//=======================================================================
//=======================================================================
// def unpack_flags()
static inline void
unpack_flags(struct gb_cpu* restrict cpu) {
	cpu->fz = (cpu->r[iF] >> fZpos) & 0x1;
	cpu->fn = (cpu->r[iF] >> fNpos) & 0x1;
	cpu->fh = (cpu->r[iF] >> fHpos) & 0x1;
	cpu->fc = (cpu->r[iF] >> fCpos) & 0x1;
} // end unpack_flags()

// Flag bit access macros
#define fZ (CPU.fz)
#define fN (CPU.fn)
#define fH (CPU.fh)
#define fC (CPU.fc)

//=======================================================================
// doc adv_cpu()
// Advance emulated CPU by specified number of program bytes and cycles.
//-----------------------------------------------------------------------
// Parameters:
// * core: Pointer to the emulated Game Boy core.
//         Behavior is undefined if `core` does not point to a valid
//         `struct gb_core` object.
// * int_pc: The number of bytes to increment `core`'s CPU's PC register.
// * cycles: The number of cycles to increment `core`'s scheduler.
//=======================================================================
// def adv_cpu()
static inline void
(adv_cpu)(
		struct gb_core* restrict core,
		uint_fast8_t inc_pc,
		uint_fast8_t cycles) {
	rPC += inc_pc;
	gb_sch_advance(core, cycles);
} // end adv_cpu()

//=======================================================================
// 8-bit load, arithmetic, and logical operations
//=======================================================================
static inline uint8_t
(LD8)(struct gb_core* restrict, uint8_t, uint8_t rhs) {
	return rhs;
} // end LD8()
//-----------------------------------------------------------------------
static inline uint8_t
(ADD8)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint_fast16_t sum = lhs + rhs;
	fZ = (sum == 0);
	fN = 0;
	fH = ((lhs & 0xF) + (rhs & 0xF)) > 0xF;
	fC = sum > 0xFF;
	return (uint8_t)sum;
} // end ADD8()
//-----------------------------------------------------------------------
static inline uint8_t
(ADC)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint_fast16_t sum = lhs + rhs + fC;
	fZ = (sum == 0);
	fN = 0;
	fH = ((lhs & 0xF) + (rhs & 0xF) + fC) > 0xF;
	fC = sum > 0xFF;
	return (uint8_t)sum;
} // end ADC()
//-----------------------------------------------------------------------
static inline uint8_t
(SUB)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint_fast16_t diff = lhs - rhs;
	fZ = (diff == 0);
	fN = 1;
	fH = ((lhs & 0xF) - (rhs & 0xF)) > 0xF;
	fC = diff > 0xFF;
	return (uint8_t)diff;
} // end SUB()
//-----------------------------------------------------------------------
static inline uint8_t
(SBC)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint_fast16_t diff = lhs - rhs - fC;
	fZ = (diff == 0);
	fN = 1;
	fH = ((lhs & 0xF) - (rhs & 0xF) - fC) > 0xF;
	fC = diff > 0xFF;
	return (uint8_t)diff;
} // end SBC()
//-----------------------------------------------------------------------
static inline uint8_t
(AND)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs & rhs;
	fZ = (result == 0);
	fN = fC = 0;
	fH = 1;
	return result;
} // end AND()
//-----------------------------------------------------------------------
static inline uint8_t
(XOR)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs ^ rhs;
	fZ = (result == 0);
	fN = fH = fC = 0;
	return result;
} // end XOR()
//-----------------------------------------------------------------------
static inline uint8_t
(OR)(struct gb_core* restrict core, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs | rhs;
	fZ = (result == 0);
	fN = fH = fC = 0;
	return result;
} // end OR()
//-----------------------------------------------------------------------
static inline uint8_t
(INC8)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	++target;
	fZ = (target == 0);
	fN = 0;
	fH = (target & 0xF) == 0; // Lower nybble was previously 0xF
	return target;
} // end INC8()
//-----------------------------------------------------------------------
static inline uint8_t
(DEC8)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	--target;
	fZ = (target == 0);
	fN = 1;
	fH = (target & 0xF) == 0xF; // Lower nybble was previously 0
	return target;
} // end DEC8()

//=======================================================================
// 16-bit load and arithmetic operations
//=======================================================================
static inline uint16_t
(LD16)(struct gb_core* restrict, uint16_t, uint16_t rhs) {
	return rhs;
} // end LD16()
//-----------------------------------------------------------------------
static inline void
(PUSH)(struct gb_core* restrict core, uint16_t src, uint16_t) {
	rSP -= 2;
	WRITE_MEMu16(rSP, src);
} // end PUSH()
//-----------------------------------------------------------------------
static inline uint16_t
(POP)(struct gb_core* restrict core, uint16_t, uint16_t) {
	uint16_t data = READ_MEMu16(rSP);
	rSP += 2;
	return data;
} // end POP()
//-----------------------------------------------------------------------
static inline void
(ADD_HL)(struct gb_core* restrict core, uint16_t rhs, uint16_t) {
	uint32_t sum = rHL + rhs;
	fN = 0;
	fH = (((rHL & 0xFFF) + (rhs & 0xFFF)) > 0xFFF);
	rHL += rhs;
	fC = rHL < rhs; // Did rHL overflow?
} // end ADD_HL()
//-----------------------------------------------------------------------
static inline uint16_t
(ADD_SP_si8)(struct gb_core* restrict core) {
	// DOES NOT MODIFY rSP WITHIN FUNCTION
	int8_t si8 = READ_MEMs8(rPC+1);
	uint16_t sum = rSP + READ_MEMs8(rPC+1);
	fZ = fN = 0;
	fH = (((rSP & 0xFFF) + si8) > 0xFFF);
	fC = (sum < rSP);
	return sum;
} // end ADDSP_si8()
//-----------------------------------------------------------------------
static inline uint16_t
(INC16)(struct gb_core* restrict, uint16_t src, uint16_t) {
	return src + 1;
} // end INC16()
//-----------------------------------------------------------------------
static inline uint16_t
(DEC16)(struct gb_core* restrict, uint16_t src, uint16_t) {
	return src - 1;
} // end DEC16()

//=======================================================================
// Rotation and Shift operations
//=======================================================================
static inline uint8_t
(RLCA)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	fC = (target >> 7);
	fZ = fN = fH = 0;
	return ((target << 1) | fC);
} // end RLCA()
//-----------------------------------------------------------------------
static inline uint8_t
(RRCA)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	fC = (target & 0x1);
	fZ = fN = fH = 0;
	return ((target >> 1) | (fC << 7));
} // end RRCA()
//-----------------------------------------------------------------------
static inline uint8_t
(RLA)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	uint8_t prev_fC = fC;
	fC = (target >> 7);
	fZ = fN = fH = 0;
	return ((target << 1) | prev_fC);
} // end RLA()
//-----------------------------------------------------------------------
static inline uint8_t
(RRA)(struct gb_core* restrict core, uint8_t target, uint8_t) {
	uint8_t prev_fC = fC;
	fC = (target & 0x1);
	fZ = fN = fH = 0;
	return ((target >> 1) | (prev_fC << 7));
} // end RRA()
//-----------------------------------------------------------------------
static inline uint8_t
(RLC)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	fC = (arg >> 7);
	arg = (arg << 1) | fC;
	fZ = (arg == 0);
	fH = fN = 0;
	return arg;
} // end RLC()
//-----------------------------------------------------------------------
static inline uint8_t
(RRC)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	fC = (arg & 0x1);
	arg = (arg >> 1) | (fC << 7);
	fZ = (arg = 0);
	fH = fN = 0;
	return arg;
} // end RRC()
//-----------------------------------------------------------------------
static inline uint8_t
(RL)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	uint8_t prev_carry = fC;
	fC = (arg >> 7);
	arg = (arg << 1) | prev_carry;
	fZ = (arg == 0);
	fH = fN = 0;
	return arg;
} // end RL()
//-----------------------------------------------------------------------
static inline uint8_t
(RR)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	uint8_t prev_carry = fC;
	fC = (arg & 0x1);
	arg = (arg >> 1) | (prev_carry << 7);
	fZ = (arg == 0);
	fH = fN = 0;
	return arg;
} // end RR()
//-----------------------------------------------------------------------
static inline uint8_t
(SLA)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	fC = (arg >> 7);
	arg <<= 1;
	fZ == (arg == 0);
	fH = fN = 0;
	return arg;
} // end SLA()
//-----------------------------------------------------------------------
static inline uint8_t
(SRA)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	fC = (arg & 0x1);
	// Shift right 1 bit, maintaining bit 7's original value:
	arg = (arg & 0x80) | (arg >> 1);
	fZ = (arg == 0);
	fH = fN = 0;
	return arg;
} // end SRA()
//-----------------------------------------------------------------------
static inline uint8_t
(SWAP)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	arg = (arg << 4) | (arg >> 4);
	fZ = (arg == 0);
	fH = fN = fC = 0;
	return arg;
} // end SWAP()
//-----------------------------------------------------------------------
static inline uint8_t
(SRL)(struct gb_core* restrict core, uint8_t arg, uint8_t) {
	fC = (arg & 0x1);
	arg >>= 1;
	fZ = (arg == 0);
	fH = fN = 0;
	return arg;
} // end SRL()

//=======================================================================
// Bit operations
//=======================================================================
static inline void
(BIT)(struct gb_core* restrict core, uint8_t value, uint_fast8_t bit) {
	fZ = (value & (1 << bit)) == 0;
	fH = 1;
	fN = 0;
} // end BIT()
//-----------------------------------------------------------------------
static inline uint8_t
(RES)(struct gb_core* restrict, uint8_t value, uint_fast8_t bit) {
	return (value & ~(1 << bit));
} // end RES()
//-----------------------------------------------------------------------
static inline uint8_t
(SET)(struct gb_core* restrict, uint8_t value, uint_fast8_t bit) {
	return (value | (1 << bit));
} // end SET()

//=======================================================================
// Jump operations
//=======================================================================
// NOTE: No memory reads are performed for conditional jumps when the
// condition is false. This differs from actual hardware, which means
// that any untaken jumps to addresses pointing to registers with
// on-read effects will differ from hardware.
//
// This should only matter for the prohibited memory mapping 0xFEA0-0xFEFF,
// which will trigger OAM corruption when read during PPU mode 2 on
// specific models of Game Boy, which I don't care to replicate at this
// time.
static inline void
(JP)(struct gb_core* restrict core, uint_fast8_t flag) {
	if (flag) {
		rPC = READ_MEMu16(rPC+1);
		gb_sch_advance(core, 4);
	} else
		adv_cpu(core, 3, 3);
} // end JP()
//-----------------------------------------------------------------------
static inline void
(JR)(struct gb_core* restrict core, uint_fast8_t flag) {
	if (flag) {
		// Signed relative jump from -126 to 129
		rPC += READ_MEMs8(rPC+1);
		adv_cpu(core, 2, 3);
	} else
		adv_cpu(core, 2, 2);
} // end JR()
//-----------------------------------------------------------------------
static inline void
(CALL)(struct gb_core* restrict core, uint_fast8_t flag) {
	if (flag) {
		// Push address of instruction following this one to the stack.
		PUSH(core, rPC+3, 0);
		rPC = READ_MEMu16(rPC+1);
		gb_sch_advance(core, 6);
	} else
		adv_cpu(core, 3, 3);
} // end CALL()
//-----------------------------------------------------------------------
static inline void
(RET)(struct gb_core* restrict core, uint_fast8_t flag) {
	if (flag) {
		// Pop address from top of the stack, jump to it.
		rPC = POP(core, 0, 0);
		if (flag == 1) // Conditional
			gb_sch_advance(core, 5);
		else // Unconditional
			gb_sch_advance(core, 4);
	} else
		adv_cpu(core, 1, 2);
} // end RET()
//-----------------------------------------------------------------------
static inline void
(RST)(struct gb_core* restrict core, uint_fast8_t dst) {
	PUSH(core, rPC+1, 0); // Address of next instruction.
	rPC = dst;
	gb_sch_advance(core, 4);
} // end RST()

//=======================================================================
// Miscellaneous operations
//=======================================================================
// Decimal-Adjust Accumulator
static inline uint8_t
(DAA)(struct gb_core* restrict core, uint8_t value, uint8_t) {
	// If DAA is preceded by ADC A, 0x99
	// when C = 1 and A = 0x99
	// Then A = 0x33, H = 1, C = 1
	// So, when N = 0:
	// 1. It is impossible for both H = 1 AND (A & 0xF) >= 0xA
	// 2. It is impossible for both C = 1 AND A >= 0xA0
	//
	// If DAA is preceded by SBC A, 0x99
	// when C = 1 and A = 0x00
	// Then A = 0x66, H = 1, C = 1
	// So, when N = 1:
	// 1. It is impossible for both H = 1 AND (A & 0xF) < 0x6 (it cannot overflow below 0)
	// 2. It is impossible for both C = 1 AND A < 0x60 (it cannot overflow below 0)
	//
	// To summarize:
	// * Adjusting for a half-carry cannot trigger another half-carry.
	// * Adjusting for a carry cannot trigger another carry.
	if (!fN) { // Previous operation was addition
		if (fH || ((value & 0x0F) >= 0x0A)) {
			if (value >= 0xFA) // Adjustment of lower nybble will overflow top nybble.
				fC = 1;
			value += 0x06;
		}
		if (fC || (fC = (value >= 0xA0)))
			value += 0x60;
	} else { // Previous operation was subtraction
		// If the nybble-to-adjust lies within the 0xA-0xF range,
		// then the respective carry flag will already be set.
		// Also, fC can't change as the value cannot overflow past 0
		// (minimum value: 0x00 - 0x99 = 0x66).
		if (fH)
			value -= 0x06;
		if (fC)
			value -= 0x60;
	}

	fZ = (value == 0);
	fH = 0;
	return value;
} // end DAA()
//-----------------------------------------------------------------------
// Invert all bits in provided value, return modified value.
static inline uint8_t
(CPL)(struct gb_core* restrict core, uint8_t value, uint8_t) {
	fN = fH = 1;
	return ~value;
} // end CPL()
//-----------------------------------------------------------------------
// Set carry flag.
static inline void
(SCF)(struct gb_core* restrict core, uint8_t, uint8_t) {
	fN = fH = 0;
	fC = 1;
} // end SCF()
//-----------------------------------------------------------------------
// Complement (invert) carry flag.
static inline void
(CCF)(struct gb_core* restrict core, uint8_t, uint8_t) {
	fN = fH = 0;
	fC = !fC;
} // end CCF()

// Result handlers
#define WRITEBACK_REGISTER(dst, src) ((dst) = (src))
#define WRITEBACK_MEMORY(addr, src) WRITE_MEMu8(addr, src)
#define DISCARD_REGISTER(unused, src) (src)
#define DISCARD_MEMORY(unused, src) (src)

#define OPCASE_R(opcode, proc, lhs, rhs, result_handling, op_bytes, op_cycles) \
	case opcode: result_handling##_REGISTER(lhs, proc(CORE, lhs, rhs)); \
	             adv_cpu(CORE, op_bytes, op_cycles); \
	             break
#define OPCASE_M(opcode, proc, lhs, rhs, result_handling, op_bytes, op_cycles) \
	case opcode: result_handling##_MEMORY(lhs, proc(CORE, READ_MEMu8(lhs), rhs)); \
	             adv_cpu(CORE, op_bytes, op_cycles); \
	             break

// Used for 8-bit load (excluding LD [HL], r), arithmetic, and logical instructions.
#define OPCASES_lhs_7r8HLi8(B_opcode, n_opcode, proc, lhs, result_handling) \
	OPCASE_R((B_opcode)  , proc, lhs, rB, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+1, proc, lhs, rC, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+2, proc, lhs, rD, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+3, proc, lhs, rE, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+4, proc, lhs, rH, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+5, proc, lhs, rL, result_handling, 1, 1); \
	OPCASE_R((B_opcode)+6, proc, lhs, READ_MEMu8(rHL), result_handling, 1, 2); \
	OPCASE_R((B_opcode)+7, proc, lhs, rA, result_handling, 1, 1); \
	OPCASE_R(n_opcode, proc, lhs, READ_MEMu8(rPC+1), result_handling, 2, 2)

// Used for 8-bit INC and DEC
#define OPCASES_7r8HL(B_opcode, proc) \
	OPCASE_R((B_opcode)     , proc, rB, 0, WRITEBACK, 1, 1); \
	OPCASE_R((B_opcode)+0x08, proc, rC, 0, WRITEBACK, 1, 1); \
	OPCASE_R((B_opcode)+0x10, proc, rD, 0, WRITEBACK, 1, 1); \
	OPCASE_R((B_opcode)+0x18, proc, rE, 0, WRITEBACK, 1, 1); \
	OPCASE_R((B_opcode)+0x20, proc, rH, 0, WRITEBACK, 1, 1); \
	OPCASE_R((B_opcode)+0x28, proc, rL, 0, WRITEBACK, 1, 1); \
	OPCASE_M((B_opcode)+0x30, proc, rHL, 0, WRITEBACK, 1, 3); \
	OPCASE_R((B_opcode)+0x38, proc, rA, 0, WRITEBACK, 1, 1)

// Used for 3/4 of 16-bit LD, ADD_HL, INC, DEC, POP, and PUSH
#define OPCASES_3r16(BC_opcode, proc, rhs, result_handling, op_bytes, op_cycles) \
	OPCASE_R((BC_opcode)     , proc, rBC, rhs, result_handling, op_bytes, op_cycles); \
	OPCASE_R((BC_opcode)+0x10, proc, rDE, rhs, result_handling, op_bytes, op_cycles); \
	OPCASE_R((BC_opcode)+0x20, proc, rHL, rhs, result_handling, op_bytes, op_cycles)

// Used for 16-bit LD, INC, and DEC
#define OPCASES_4r16(BC_opcode, proc, rhs, result_handling, op_bytes, op_cycles) \
	OPCASES_3r16(BC_opcode, proc, rhs, result_handling, op_bytes, op_cycles); \
	OPCASE_R((BC_opcode)+0x30, proc, rSP, rhs, result_handling, op_bytes, op_cycles)

// Used for conditional JR, RET, JP, and CALL
// PC-modifying functions are responsible for advancing the cpu and scheduler.
// Reasoning: Flag must be evaluated to determine how many cycles have passed.
#define OPCASES_JP(NZ_opcode, uncond_opcode, proc) \
	case (NZ_opcode)     : proc(CORE,  !fZ); break; \
	case (NZ_opcode)+0x08: proc(CORE,   fZ); break; \
	case (NZ_opcode)+0x10: proc(CORE,  !fC); break; \
	case (NZ_opcode)+0x18: proc(CORE,   fC); break; \
	case (uncond_opcode) : proc(CORE, 0xFF); break

// Used for rotation, shift, and bit instructions (all CB-prefixed operations)
#define OPCASES_7r8HL_rhs(B_opcode, proc, rhs, result_handling, HL_cycles) \
	OPCASE_R((B_opcode)  , proc, rB, rhs, result_handling, 2, 2); \
	OPCASE_R((B_opcode)+1, proc, rC, rhs, result_handling, 2, 2); \
	OPCASE_R((B_opcode)+2, proc, rD, rhs, result_handling, 2, 2); \
	OPCASE_R((B_opcode)+3, proc, rE, rhs, result_handling, 2, 2); \
	OPCASE_R((B_opcode)+4, proc, rH, rhs, result_handling, 2, 2); \
	OPCASE_R((B_opcode)+5, proc, rL, rhs, result_handling, 2, 2); \
	OPCASE_M((B_opcode)+6, proc, rHL, rhs, result_handling, 2, HL_cycles); \
	OPCASE_R((B_opcode)+7, proc, rA, rhs, result_handling, 2, 2)

// Used for bit instructions.
#define OPCASES_7r8HL_8b(B_0_opcode, proc, result_handling, HL_cycles) \
	OPCASES_7r8HL_rhs((B_0_opcode)     , proc, 0, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x08, proc, 1, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x10, proc, 2, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x18, proc, 3, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x20, proc, 4, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x28, proc, 5, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x30, proc, 6, result_handling, HL_cycles); \
	OPCASES_7r8HL_rhs((B_0_opcode)+0x38, proc, 7, result_handling, HL_cycles)

//=======================================================================
// doc call_isr()
// TODO
//=======================================================================
// def call_isr()
static inline uint8_t
call_isr(struct gb_core* restrict core) {
	core->cpu.state &= ~CPUSTATE_HALTED;
	gb_mem_io_set_ime(core, 0);
	PUSH(core, rPC, 0);
	gb_sch_advance(core, 4);
	uint8_t pending = gb_mem_io_pending_interrupts(core);
	gb_sch_advance(core, 1);
	uint8_t interrupt;
	if (pending & IO_IFE_VBLANK) {
		rPC = 0x0040;
		interrupt = IO_IFE_VBLANK;
	} else if (pending & IO_IFE_STAT) {
		rPC = 0x0048;
		interrupt = IO_IFE_STAT;
	} else if (pending & IO_IFE_TIMER) {
		rPC = 0x0050;
		interrupt = IO_IFE_TIMER;
	} else if (pending & IO_IFE_SERIAL) {
		rPC = 0x0058;
		interrupt = IO_IFE_SERIAL;
	} else if (pending & IO_IFE_JOYP) {
		rPC = 0x0060;
		interrupt = IO_IFE_JOYP;
	} else { // Error: Should be impossible.
		interrupt = 0;
	}

	gb_mem_io_clear_interrupt(core, interrupt);
	return interrupt;
} // end call_isr()

//=======================================================================
// doc interpret_once()
// TODO
//=======================================================================
// def interpret_once()
static inline void
interpret_once(struct gb_core* restrict core) {
//	struct gb_opc_components opc;
//	gb_opc_current_components(&opc, core);
//	char buf[1024];
//	gb_opc_string(buf, sizeof(buf), &opc, core);
//	printf("%s\n", buf);
	switch (READ_MEMu8(rPC)) {
		//-------------------------------------------------------------------
		// 8-bit load instructions
		// LD r8, r8/[HL]/i8
		OPCASES_lhs_7r8HLi8(0x40, 0x06, LD8, rB, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x48, 0x0E, LD8, rC, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x50, 0x16, LD8, rD, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x58, 0x1E, LD8, rE, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x60, 0x26, LD8, rH, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x68, 0x2E, LD8, rL, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x78, 0x3E, LD8, rA, WRITEBACK);
		// LD [HL], r8/i8:
		case 0x70: WRITE_MEMu8(rHL, rB); adv_cpu(core, 1, 2); break;
		case 0x71: WRITE_MEMu8(rHL, rC); adv_cpu(core, 1, 2); break;
		case 0x72: WRITE_MEMu8(rHL, rD); adv_cpu(core, 1, 2); break;
		case 0x73: WRITE_MEMu8(rHL, rE); adv_cpu(core, 1, 2); break;
		case 0x74: WRITE_MEMu8(rHL, rH); adv_cpu(core, 1, 2); break;
		case 0x75: WRITE_MEMu8(rHL, rL); adv_cpu(core, 1, 2); break;
		case 0x77: WRITE_MEMu8(rHL, rA); adv_cpu(core, 1, 2); break;
		case 0x36: WRITE_MEMu8(rHL, READ_MEMu8(rPC+1)); adv_cpu(core, 2, 3); break;
		// LD [r16], A / LD A, [r16]
		case 0x02: WRITE_MEMu8(rBC, rA); adv_cpu(core, 1, 2); break;
		case 0x12: WRITE_MEMu8(rDE, rA); adv_cpu(core, 1, 2); break;
		case 0x0A: rA = READ_MEMu8(rBC); adv_cpu(core, 1, 2); break;
		case 0x1A: rA = READ_MEMu8(rDE); adv_cpu(core, 1, 2); break;
		// LD [HL+/-], A / LD A, [HL+/-]
		// Increments/decrements the value of HL _AFTER_
		// reading from/writing to the address it points to.
		case 0x22: WRITE_MEMu8(rHL++, rA); adv_cpu(core, 1, 2); break;
		case 0x32: WRITE_MEMu8(rHL--, rA); adv_cpu(core, 1, 2); break;
		case 0x2A: rA = READ_MEMu8(rHL++); adv_cpu(core, 1, 2); break;
		case 0x3A: rA = READ_MEMu8(rHL--); adv_cpu(core, 1, 2); break;
		// LD [FF00+i8], A / LD A, [FF00+i8]
		case 0xE0: WRITE_MEMFF(READ_MEMu8(rPC+1), rA); adv_cpu(core, 2, 3); break;
		case 0xF0: rA = READ_MEMFF(READ_MEMu8(rPC+1)); adv_cpu(core, 2, 3); break;
		// LD [FF00+C], A / LD A, [FF00+C]
		// (C refers to the C register, not the carry flag)
		case 0xE2: WRITE_MEMFF(rC, rA); adv_cpu(core, 1, 2); break;
		case 0xF2: rA = READ_MEMFF(rC); adv_cpu(core, 1, 2); break;
		// LD [i16], A / LD A, [i16]
		case 0xEA: WRITE_MEMu8(READ_MEMu16(rPC+1), rA); adv_cpu(core, 3, 4); break;
		case 0xFA: rA = READ_MEMu8(READ_MEMu16(rPC+1)); adv_cpu(core, 3, 4); break;
		//-------------------------------------------------------------------
		// 8-bit arithmetic and logical instructions
		// (ALU) A, r8/[HL]/i8
		OPCASES_lhs_7r8HLi8(0x80, 0xC6, ADD8, rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x88, 0xCE, ADC , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x90, 0xD6, SUB , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0x98, 0xDE, SBC , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0xA0, 0xE6, AND , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0xA8, 0xEE, XOR , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0xB0, 0xF6, OR  , rA, WRITEBACK);
		OPCASES_lhs_7r8HLi8(0xB8, 0xFE, SUB , rA, DISCARD); // CP
		OPCASES_7r8HL(0x04, INC8);
		OPCASES_7r8HL(0x05, DEC8);
		//-------------------------------------------------------------------
		// 16-bit load and arithmetic instructions
		// LD r16, i16
		OPCASES_4r16(0x01, LD16, READ_MEMu16(rPC+1), WRITEBACK, 3, 3);
		// PUSH r16
		OPCASES_3r16(0xC5, PUSH, 0, DISCARD, 1, 4);
		case 0xF5: pack_flags(&(core->cpu));
		           PUSH(core, rAF, 0); adv_cpu(core, 1, 4); break;
		// POP r16
		OPCASES_3r16(0xC1, POP, 0, WRITEBACK, 1, 3);
		case 0xF1: rAF = POP(core, 0, 0); adv_cpu(core, 1, 3);
		           unpack_flags(&(core->cpu)); break;
		// LD [i16], SP
		case 0x08: WRITE_MEMu16(rPC+1, rSP); adv_cpu(core, 3, 5); break;
		// LD HL, SP+si8
		case 0xF8: rHL = ADD_SP_si8(core); adv_cpu(core, 2, 3); break;
		// LD SP, HL
		case 0xF9: rSP = rHL; adv_cpu(core, 1, 2); break;
		// ADD HL, r16
		// Uses DISCARD handling because HL is modified within ADD_HL()
		OPCASES_4r16(0x09, ADD_HL, 0, DISCARD, 1, 2);
		// ADD SP, si8
		case 0xE8: rSP = ADD_SP_si8(core); adv_cpu(core, 2, 4); break;
		// INC/DEC r16
		OPCASES_4r16(0x03, INC16, 0, WRITEBACK, 1, 2);
		OPCASES_4r16(0x0B, DEC16, 0, WRITEBACK, 1, 2);
		//-------------------------------------------------------------------
		// Rotation instructions (A only)
		OPCASE_R(0x07, RLCA, rA, 0, WRITEBACK, 1, 1);
		OPCASE_R(0x0F, RRCA, rA, 0, WRITEBACK, 1, 1);
		OPCASE_R(0x17, RLA , rA, 0, WRITEBACK, 1, 1);
		OPCASE_R(0x1F, RRA , rA, 0, WRITEBACK, 1, 1);
		//-------------------------------------------------------------------
		// Jump, call, return, and reset instructions
		OPCASES_JP(0xC2, 0xC3, JP);
		OPCASES_JP(0x20, 0x18, JR);
		OPCASES_JP(0xC4, 0xCD, CALL);
		OPCASES_JP(0xC0, 0xC9, RET);
		case 0xE9: rPC = rHL; gb_sch_advance(core, 1); break; // JP HL
		case 0xD9: // RETI: Functionally identical to EI followed by RET
			RET(core, 0xFF);
			gb_mem_io_set_ime(core, 1);
			break;
		// RST $XX
		case 0xC7: RST(core, 0x00); break;
		case 0xCF: RST(core, 0x08); break;
		case 0xD7: RST(core, 0x10); break;
		case 0xDF: RST(core, 0x18); break;
		case 0xE7: RST(core, 0x20); break;
		case 0xEF: RST(core, 0x28); break;
		case 0xF7: RST(core, 0x30); break;
		case 0xFF: RST(core, 0x38); break;
		//-------------------------------------------------------------------
		// Miscellaneous instructions
		OPCASE_R(0x27, DAA, rA, 0, WRITEBACK, 1, 1);
		OPCASE_R(0x2F, CPL, rA, 0, WRITEBACK, 1, 1);
		OPCASE_R(0x37, SCF, 0, 0, DISCARD, 1, 1);
		OPCASE_R(0x3F, CCF, 0, 0, DISCARD, 1, 1);
		case 0x00: adv_cpu(core, 1, 1); break; // NOP
		case 0x10: adv_cpu(core, 2, 1); break; // TODO: STOP
		case 0x76: core->cpu.state |= CPUSTATE_HALTED; adv_cpu(core, 1, 1); break; // HALT
		case 0xCB: CB_interpret_once(core); break; // CB PREFIX
		case 0xF3: // DI: Clear IME (Interrupt Master Enable)
			gb_mem_io_set_ime(core, 0);
			adv_cpu(core, 1, 1);
			break;
		case 0xFB: // EI: Set IME (Interrupt Master Enable)
			execute_EI(core);
			break;
		default:
			fprintf(stderr, "Unimplemented opcode 0x%X at 0x%X", READ_MEMu8(rPC), rPC);
			break;
	} // end switch
} // end interpret_once()

//=======================================================================
// doc CB_interpret_once()
// TODO
//=======================================================================
// def CB_interpret_once()
static inline void
CB_interpret_once(struct gb_core* restrict core) {
	switch (READ_MEMu8(rPC+1)) {
		//-------------------------------------------------------------------
		// Rotation/Shift operations
		// `rhs` is unused, so `0` is passed to the macro.
		OPCASES_7r8HL_rhs(0x00, RLC , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x08, RRC , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x10, RL  , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x18, RR  , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x20, SLA , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x28, SRA , 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x30, SWAP, 0, WRITEBACK, 16);
		OPCASES_7r8HL_rhs(0x38, SRL , 0, WRITEBACK, 16);
		//-------------------------------------------------------------------
		// BIT, RES, and SET operations.
		OPCASES_7r8HL_8b(0x40, BIT, DISCARD, 12);
		OPCASES_7r8HL_8b(0x80, RES, WRITEBACK, 16);
		OPCASES_7r8HL_8b(0xC0, SET, WRITEBACK, 16);
	} // end switch
} // end CB_interpret_once()

//=======================================================================
// doc execute_EI()
// TODO
//=======================================================================
// def execute_EI()
static inline void
execute_EI(struct gb_core* restrict core) {
	// EI is delayed by 1 additional cycle, meaning 2 things:
	// 1. 1 instruction will run after EI before IME is set.
	// 2. If EI is followed by DI, then IME will NOT be set.
	//    In such a case, both EI and DI will have effectively been NOPs.
	if (!gb_mem_io_get_ime(core)) {
		uint8_t next_op = READ_MEMu8(rPC+1);
		if (next_op == 0xF3) { // DI
			adv_cpu(core, 2, 2); // Treat EI and DI as NOPs
		} else if (next_op == 0xFB) { // EI
			// Next instruction is redundant.
			// Treat following EI as NOP to prevent needless recursion.
			adv_cpu(core, 2, 2);
			gb_mem_io_set_ime(core, 1);
		} else {
			// Run next instruction, then enable interrupts.
			adv_cpu(core, 1, 1); // EI advancement
			interpret_once(core);
			gb_mem_io_set_ime(core, 1);
		}
	} else {
		// IME already set. Effectively a NOP.
		adv_cpu(core, 1, 1);
	} // end ifelse !ime
} // end execute_EI()

