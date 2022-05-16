//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
//=======================================================================
#include <../src/twi/gb/cpu.c>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <twi/std/assertf.h>
#include <twi/std/test.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private macro definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
//-----------------------------------------------------------------------
// Private constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Operand index sets
//=======================================================================
static const uint8_t reg8_set[] = {
	IDX_B, IDX_C, IDX_D, IDX_E, IDX_H, IDX_L, IDX_A
};

//=======================================================================
// Test programs
//=======================================================================
static const uint8_t LD_r_r_ops[] = { // rhs: B,C,D,E,H,L,A
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, // LD B, r
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4F, // LD C, r
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, // LD D, r
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5F, // LD E, r
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, // LD H, r
	0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6F, // LD L, r
	0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7F  // LD A, r
};

//=======================================================================
//-----------------------------------------------------------------------
// Private type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def enum twi_gb_cpu_test_optype
// 
// Defines operand type for use in testing of 8-bit operands.
//=======================================================================
enum twi_gb_test_cpu_optype8 {
	TWI_GB_TEST_CPU_OPTYPE_REG8, // Indicates index of 8-bit register
	TWI_GB_TEST_CPU_OPTYPE_REG16PTR, // Indicates index of 16-bit register for use as a pointer to memory
	TWI_GB_TEST_CPU_OPTYPE_RBIT, // Indicates a literal bit offset that will NOT modify the other argument
	TWI_GB_TEST_CPU_OPTYPE_WBIT, // Indicates a literal bit offset that will modify the other argument
	TWI_GB_TEST_CPU_OPTYPE_IMMED8, // Indicates use of unsigned 8-bit immediate
	TWI_GB_TEST_CPU_OPTYPE_NONE // Indicates that there is no operand
}; // end enum twi_gb_cpu_test_optype

//=======================================================================
// def struct twi_gb_test_cpu_args8
// 
// Arguments passed to cputest8, a polymorphic function for testing
// CPU operations which accept 8-bit operands.
//-----------------------------------------------------------------------
// Members:
// * name: The name of the test. Used when reporting failures.
// * prog_sz: The size of the test program in bytes.
// * prog: The test program to run.
// * op: The operation function to use in calculating expected result.
//   Behavior is undefined if `op` does not point to a function of
//   the specified signature.
// * lhs_type: The type of the left-hand side argument.
//   Can be any of REG8, REG16PTR, RBIT, or WBIT.
//   Behavior is undefined if IMMED8 or NONE are specified.
// * lhs_sz: The number of elements in the array pointed to by `lhs`.
// * lhs: Pointer to an array of register indices to use as arguments.
//   Behavior is undefined if `lhs` does not point to an array of indices
//   with at least `lhs_sz` number of elements.
// * rhs_type: The type of the right-hand side arguments.
//   Can be any of REG8, REG16PTR, IMMED8, or NONE.
//   Behavior is undefined if RBIT or WBIT is specified.
//   If either IMMED8 or NONE, `rhs_sz` and `rhs` are ignored.
// * rhs_sz: The number of elements in the array pointed to by `rhs`.
// * rhs: Pointer to an array of register indices to use as arguments.
// * pc_per_op: The number of instructions to expect the instruction
//   pointer to advance after each instruction.
// * div_per_op: The number of clock cycles to expect the emulated CPU
//   to advance after each instruction.
//=======================================================================
struct twi_gb_test_cpu_args8 {
	const char* name;
	size_t prog_sz;
	const uint8_t* prog;
	uint8_t (*op)(uint8_t* restrict, uint8_t, uint8_t);
	enum twi_gb_test_cpu_optype8 lhs_type;
	uint8_t lhs_sz;
	const uint8_t* lhs;
	enum twi_gb_test_cpu_optype8 rhs_type;
	uint8_t rhs_sz;
	const uint8_t* rhs;
	uint8_t pc_per_op;
	uint8_t div_per_op;
}; // end struct twi_gb_test_cpu_args8

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
// Helpers
static void set_program(struct twi_gb_mem* restrict, uint16_t, const void* restrict);
static void test_cpu_reset(struct twi_gb_cpu* restrict);
// Tests
static int_fast8_t test_register_coherency();
static int_fast8_t cputest8(const struct twi_gb_test_cpu_args8* restrict);

//=======================================================================
//-----------------------------------------------------------------------
// Public function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_test_cpu_all()
//=======================================================================
size_t
twi_gb_test_cpu_all() {
	struct twi_gb_cpu cpu;
	struct twi_gb_mem mem;
	twi_gb_mem_init(&mem, NULL, NULL, TWI_GB_MODE_DMG);
	size_t fail_count = 0;

	fail_count += test_register_coherency();

	// Tests for instructions which use 8-bit operands
	struct twi_gb_test_cpu_args8 args8;
	
	// LD r, r
	args8.name = "LD r, r";
	args8.prog_sz = sizeof(LD_r_r_ops);
	args8.prog = LD_r_r_ops;
	args8.op = LD8;
	args8.lhs_type = args8.rhs_type = TWI_GB_TEST_CPU_OPTYPE_REG8;
	args8.lhs_sz = args8.rhs_sz = sizeof(reg8_set);
	args8.lhs = args8.rhs = reg8_set;
	args8.pc_per_op = 1;
	args8.div_per_op = 4;
	fail_count += cputest8(&args8);

	return fail_count;
} // end twi_gb_test_cpu_all()

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def getval8()
// TODO
//=======================================================================

//=======================================================================
// def setval8()
// TODO
//=======================================================================
void
setval8(
		struct twi_gb_cpu* restrict cpu,
		struct twi_gb_mem* restrict mem,
		enum twi_gb_test_cpu_optype8 type,
		uint8_t idx,
		uint8_t val
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);

	if (type == TWI_GB_TEST_CPU_OPTYPE_REG8)
		REG8(cpu, idx) = val;
	else if (type == TWI_GB_TEST_CPU_OPTYPE_REG16PTR)
		twi_gb_mem_write8(mem, REG16(cpu, idx), val);
	else if (type == TWI_GB_TEST_CPU_OPTYPE_IMMED8)
		mem->map[cpu->pc + 1] = val;
} // end setval8()

//=======================================================================
// def set_program()
//
// Unsafely force-assigns a program into a twi-gb-mem object.
// This action is not supported by the twi-gb-mem interface, and thus
// all attempts to do this for testing should go through here, in order
// for this action to be adaptable to change.
//-----------------------------------------------------------------------
// Parameters:
// * mem: Target of the program assignment.
// * program_sz: The number of bytes of which the program consists.
//   Should not exceed 32,768.
// * program: The program to load.
//=======================================================================
static void
set_program(
		struct twi_gb_mem* restrict mem,
		uint16_t program_sz,
		const void* restrict program
) {
	twi_assert_notnull(mem);
	twi_assert_lteqi(program_sz, 32768);
	twi_assert_notnull(program);
	memcpy(mem, program, program_sz);
} // end set_program()

//=======================================================================
// def twi_gb_test_cpu_init()
// TODO
//=======================================================================
static void test_cpu_reset(struct twi_gb_cpu* restrict cpu) {
	twi_assert_notnull(cpu);

	for (uint_fast8_t i = 0; i < sizeof(cpu->reg); ++i)
		cpu->reg[i] = 0;
	cpu->sp = 0;
	cpu->pc = 0;
	cpu->div = 0;
} // end test_twi_gb_cpu_init()

// Redefine PTR macros defined in twi-gb to use local variables.
#undef CPU_PTR
#undef MEM
#define CPU_PTR (&cpu)
#define MEM (&mem)

//=======================================================================
// def test_register_coherency()
//=======================================================================
static int_fast8_t
test_register_coherency() {
	struct twi_gb_cpu cpu;
	test_cpu_reset(&cpu);
	// Test r-to-rr coherency. Values are arbitrary.
	REG_A = 0xA6;
	REG_F = 0x11;
	REG_B = 0xE2;
	REG_C = 0x39;
	REG_D = 0x57;
	REG_E = 0x85;
	REG_H = 0x80;
	REG_L = 0x0;
	twi_test(REG_AF == 0xA611);
	twi_test(REG_BC == 0xE239);
	twi_test(REG_DE == 0x5785);
	twi_test(REG_HL == 0x8000);
	// Test rr-to-r coherency. Values are arbitrary.
	REG_AF = 0x1234;
	REG_BC = 0x9581;
	REG_DE = 0xABC9;
	REG_HL = 0x82F7;
	twi_test(REG_A == 0x12);
	twi_test(REG_F == 0x34);
	twi_test(REG_B == 0x95);
	twi_test(REG_C == 0x81);
	twi_test(REG_D == 0xAB);
	twi_test(REG_E == 0xC9);
	twi_test(REG_H == 0x82);
	twi_test(REG_L == 0xF7);
	return 0;

test_fail:
	fprintf(stderr,
		"REG_A = 0x%" PRIX8 "\n"
		"REG_F = 0x%" PRIX8 "\n"
		"REG_B = 0x%" PRIX8 "\n"
		"REG_C = 0x%" PRIX8 "\n"
		"REG_D = 0x%" PRIX8 "\n"
		"REG_E = 0x%" PRIX8 "\n"
		"REG_H = 0x%" PRIX8 "\n"
		"REG_L = 0x%" PRIX8 "\n"
		"REG_AF = 0x%" PRIX16 "\n"
		"REG_BC = 0x%" PRIX16 "\n"
		"REG_DE = 0x%" PRIX16 "\n"
		"REG_HL = 0x%" PRIX16 "\n",
		REG_A, REG_F, REG_B, REG_C,
		REG_D, REG_E, REG_H, REG_L,
		REG_AF, REG_BC, REG_DE, REG_HL
	);
	return 1;
} // end test_register_coherency()

//=======================================================================
// def cputest8()
//
// Polymorphic testing function for testing the correctness of
// the implementation of twi-gb-cpu instructions which accept 8-bit
// operands.
//-----------------------------------------------------------------------
// Parameters:
// * args: Test arguments.
//   See description of struct twi_gb_test_cpu_args8 for details.
//
// Returns: 0 on success, 1 on failure
//=======================================================================
static int_fast8_t
cputest8(const struct twi_gb_test_cpu_args8* restrict args) {
	twi_assertf(args->lhs_type != TWI_GB_TEST_CPU_OPTYPE_RBIT
			     && args->lhs_type != TWI_GB_TEST_CPU_OPTYPE_WBIT,
			"Left-hand side argument(s) cannot be bit offsets.");
	twi_assertf(args->lhs_type != TWI_GB_TEST_CPU_OPTYPE_IMMED8,
			"Left-hand side argument(s) cannot be immediates.");
	twi_assertf(args->lhs_type != TWI_GB_TEST_CPU_OPTYPE_NONE,
			"Left-hand side argument(s) must be present for this test format.");

	// The control and test CPU and memory map.
	struct twi_gb_cpu expected_cpu, actual_cpu;
	struct twi_gb_mem expected_mem, actual_mem;
	test_cpu_reset(&expected_cpu);
	set_program(&expected_mem, args->prog_sz, args->prog);
	memcpy(&actual_cpu, &expected_cpu, sizeof(struct twi_gb_cpu));
	memcpy(&actual_mem, &expected_mem, sizeof(struct twi_gb_mem));

	if (args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_REG8
	 || args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_REG16PTR
	 || args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_RBIT
	 || args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_WBIT) {
		// `rhs` is used

		uint8_t lhs_val, rhs_val;
		for (uint8_t lhs_pos = 0; lhs_pos < args->lhs_sz; ++lhs_pos) {
			for (uint8_t rhs_pos = 0; rhs_pos < args->rhs_sz; ++rhs_pos) {
				// Use pseudorandom test values.
				lhs_val = rand() % 0x100;
				if (args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_REG8
				 || args->rhs_type == TWI_GB_TEST_CPU_OPTYPE_REG16PTR)
					rhs_val = rand() % 0x100;
				else // rhs is hard-coded and never random, so use the actual value
					rhs_val = args->rhs[rhs_pos];

				// Prepare expected and actual components for test.
				setval8(&expected_cpu, &expected_mem, args->lhs_type, args->lhs[lhs_pos], lhs_val);
				setval8(&expected_cpu, &expected_mem, args->rhs_type, args->rhs[rhs_pos], rhs_val);
				setval8(&actual_cpu, &actual_mem, args->lhs_type, args->lhs[lhs_pos], lhs_val);
				setval8(&actual_cpu, &actual_mem, args->rhs_type, args->rhs[rhs_pos], rhs_val);

				// Evaluate result, and simulate in expected components.
				uint8_t result = args->op(&REG8(&expected_cpu, IDX_F), lhs_val, rhs_val);
				if (args->rhs_type != TWI_GB_TEST_CPU_OPTYPE_RBIT)
					setval8(&expected_cpu, &expected_mem, args->lhs_type, args->lhs[lhs_pos], result);
				expected_cpu.pc += args->pc_per_op;
				expected_cpu.div += args->div_per_op;

				// Execute instruction in actual components.
				interpret_once(&actual_cpu, &actual_mem);

				// Compare (twi_test will jump to the `test_fail` label on failure).
				twi_test(!memcmp(&expected_cpu, &actual_cpu, sizeof(struct twi_gb_cpu)));
				twi_test(!memcmp(&expected_mem, &actual_mem, sizeof(struct twi_gb_mem)));
			} // end iteration through `rhs`
		} // end iteration through `lhs`
	} else {
		// `rhs` is unused
	} // end if/else `rhs` is used

	return 0;
test_fail: // Jumped to by twi_test().
	fprintf(stderr, "Test name: %s\nOpcode: 0x%" PRIX8 "\n",
			args->name, args->prog[expected_cpu.pc - args->pc_per_op]);
	twi_gb_cpu_diffdump(stderr, "expected", &expected_cpu, "actual", &actual_cpu);
	// TODO: mem diffdump
	return 1;
} // end cputest8()

//=======================================================================
// def test_LD_R_HL()
//
// Tests the functionality of memory-to-register loads.
//-----------------------------------------------------------------------
// Returns: 0 on success, 1 on failure
//=======================================================================
//static int_fast8_t
//test_LD_R_HL() {
//	struct twi_gb_cpu cpu;
//	struct twi_gb_mem mem;
//	// LD B/C/D/E/H/L/A, (HL)
//	test_cpu_init(&cpu);
//	twi_gb_mem_init(&mem, NULL, NULL, TWI_GB_MODE_DMG);
//	uint8_t program[] = {0x46, 0x4E, 0x56, 0x5E, 0x66, 0x6E, 0x7E};
//	set_program(&mem, sizeof(program), program);
//	uint8_t* reg[] = {&REG_B, &REG_C, &REG_D, &REG_E, &REG_H, &REG_L, &REG_A};
//
//	uint8_t num = 107; // Arbitrary.
//	uint32_t addr = 0xC000; // Arbitary, but must exist inside WRAM.
//	return 0; // TODO
//}

