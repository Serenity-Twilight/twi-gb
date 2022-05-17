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
#include <../src/twi/gb/mem.c>
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
//-----------------------------------------------------------------------
// Private type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def enum testcpu_type8
// 
// Defines operand type for use in testing of 8-bit operands.
//=======================================================================
enum testcpu_type8 {
	TESTCPU_TYPE_REG8, // Indicates index of 8-bit register
	TESTCPU_TYPE_REG8_RO, // Indicates index of 8-bit register that should NOT be written to.
	TESTCPU_TYPE_REG16PTR, // Indicates index of 16-bit register for use as a pointer to memory
	TESTCPU_TYPE_BIT, // Indicates a literal bit offset
	TESTCPU_TYPE_IMMED8, // Indicates use of unsigned 8-bit immediate
	TESTCPU_TYPE_NONE // Indicates that there is no operand
}; // end enum testcpu_type8

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
// Tests
static int_fast8_t test_register_coherency();
static int_fast8_t testcpu8(
		const char* restrict, uint_fast16_t, const uint8_t* restrict,
		uint8_t (*)(uint8_t* restrict, uint8_t, uint8_t),
		uint_fast8_t, uint_fast8_t,
		enum testcpu_type8, uint_fast8_t, const uint8_t[],
		enum testcpu_type8, uint_fast8_t, const uint8_t[]
);

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
	// Operand sets.
	srand(2);
	static const uint8_t reg8_set[] = {
		IDX_B, IDX_C, IDX_D, IDX_E, IDX_H, IDX_L, IDX_A
	};
	static const uint8_t HL_set[] = { IDX_HL };

	size_t fail_count = 0;
	fail_count += test_register_coherency();
	
	//-------------------------------------------------------
	// LD8
	{ // LD r, r
		static const uint8_t prog[] = { // rhs: B,C,D,E,H,L,A
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, // LD B, r
			0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4F, // LD C, r
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, // LD D, r
			0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5F, // LD E, r
			0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, // LD H, r
			0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6F, // LD L, r
			0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7F  // LD A, r
		};
		fail_count += testcpu8(
				"LD r, r", sizeof(prog), prog, LD8, 1, 4,
				TESTCPU_TYPE_REG8, sizeof(reg8_set), reg8_set,
				TESTCPU_TYPE_REG8, sizeof(reg8_set), reg8_set
		);
	} // end LD r, r
	
	{ // LD r, (HL)
		static const uint8_t prog[] = { // lhs: B,C,D,E,H,L,A
			0x46, 0x4E, 0x56, 0x5E, 0x66, 0x6E, 0x7E // LD r, (HL)
		};
		fail_count += testcpu8(
				"LD r, (HL)", sizeof(prog), prog, LD8, 1, 8,
				TESTCPU_TYPE_REG8, sizeof(reg8_set), reg8_set,
				TESTCPU_TYPE_REG16PTR, sizeof(HL_set), HL_set
		);
	} // end LD r, (HL)

	{ // LD r, n
		static const uint8_t prog[] = { // lhs: B,C,D,E,H,L,A
			0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x3E // LD r, n
		};
		fail_count += testcpu8(
				"LD r, n", sizeof(prog), prog, LD8, 2, 8,
				TESTCPU_TYPE_REG8, sizeof(reg8_set), reg8_set,
				TESTCPU_TYPE_IMMED8, 0, NULL
		);
	} // end LD r, n
	
	{ // LD (HL), r
		static const uint8_t prog[] = { // rhs: B,C,D,E,H,L,A
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x77 // LD (HL), r
		};
		fail_count += testcpu8(
				"LD (HL), r", sizeof(prog), prog, LD8, 1, 8,
				TESTCPU_TYPE_REG16PTR, sizeof(HL_set), HL_set,
				TESTCPU_TYPE_REG8, sizeof(reg8_set), reg8_set
		);
	}

	{ // LD (HL), n
		static const uint8_t prog = 0x36;
		fail_count += testcpu8(
				"LD (HL), n", 1, &prog, LD8, 2, 12,
				TESTCPU_TYPE_REG16PTR, sizeof(HL_set), HL_set,
				TESTCPU_TYPE_IMMED8, 0, NULL
		);
	}

	return fail_count;
} // end twi_gb_test_cpu_all()

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def setrand8()
// TODO
//=======================================================================
uint8_t
testcpu_setrand8(
		struct twi_gb_cpu* restrict cpu1,
		struct twi_gb_cpu* restrict cpu2,
		struct twi_gb_mem* restrict mem1,
		struct twi_gb_mem* restrict mem2,
		enum testcpu_type8 type,
		uint8_t idx
) {
	twi_assert_notnull(cpu1);
	twi_assert_notnull(cpu2);
	twi_assert_notnull(mem1);
	twi_assert_notnull(mem2);

	uint8_t val = rand() % 0x100;
	if (type == TESTCPU_TYPE_REG8 || type == TESTCPU_TYPE_REG8_RO) {
		REG8(cpu1, idx) = REG8(cpu2, idx) = val;
	} else if (type == TESTCPU_TYPE_REG16PTR) {
		// The randomized value will need to be assigned to
		// a randomized address in working RAM (WRAM).
		// The address should be assigned to the specified 16-bit register.
		uint16_t addr = rand() % TWI_GB_MEM_SZ_WRAM_DMG + FX_WRAM_BEGIN;
		REG16(cpu1, idx) = REG16(cpu2, idx) = addr;
		twi_gb_mem_write8(mem1, addr, val);
		twi_gb_mem_write8(mem2, addr, val);
	}

	return val;
} // end setrand8()

//=======================================================================
// def testcpu_setprog()
//
// Unsafely force-assigns a program into a twi-gb-mem object.
// This action is not supported by the twi-gb-mem interface, and thus
// all attempts to do this for testing should go through here, in order
// for this action to be adaptable to change.
//
// Note that final program size (after addition of immediate bytes) can
// not exceed 32,768 bytes. Therefore, the following should assert true:
// (program_sz * (immed_bytes + 1)) <= 32,768
// Otherwise, behavior is undefined.
//-----------------------------------------------------------------------
// Parameters:
// * mem: Target of the program assignment.
// * program_sz: The number of bytes of which the program consists.
// * program: The program to load.
// * immed_bytes: Each byte of `program` written to `mem` will be prefixed
//   by a number equal to `immed_bytes` of random-value bytes.
//   These simulate immediates that the instructions would otherwise
//   accept.
//   Must be less than or equal to 2, else behavior is undefined.
//=======================================================================
static void
testcpu_setprog(
		struct twi_gb_mem* restrict mem,
		uint_fast16_t program_sz,
		const uint8_t* restrict program,
		uint_fast8_t immed_bytes
) {
	twi_assert_notnull(mem);
	twi_assert_lteqi((program_sz * (immed_bytes + 1)), 32768);
	twi_assert_notnull(program);
	twi_assert_lteqi(immed_bytes, 2);

	if (immed_bytes == 0)
		memcpy(mem, program, program_sz);
	else {
		// Add random immediate bytes after each instruction
		for (uint_fast16_t i = 0, m	= 0; i < program_sz; ++i) {
			mem->map[m++] = program[i];
			for (uint_fast8_t r = 0; r < immed_bytes; ++r)
				mem->map[m++] = rand() % 0x100;
		} // end iteration through program
	} // end if immediate bytes must be inserted
} // end testcpu_setprog()

//=======================================================================
// def testcpu_reset()
// TODO
//=======================================================================
static void testcpu_reset(struct twi_gb_cpu* restrict cpu) {
	twi_assert_notnull(cpu);

	for (uint_fast8_t i = 0; i < sizeof(cpu->reg); ++i)
		cpu->reg[i] = 0;
	cpu->sp = 0;
	cpu->pc = 0;
	cpu->div = 0;
} // end testcpu_reset()

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
	testcpu_reset(&cpu);
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
// def testcpu8()
//
// Polymorphic testing function for testing the correctness of
// the implementation of twi-gb-cpu instructions which accept 8-bit
// operands.
//-----------------------------------------------------------------------
// Parameters:
// * name: The name of the test. Used when reporting failures.
// * prog_sz: The size of the test program in bytes.
// * prog: The test program to run.
// * op: The operation function to use in calculating expected result.
//   Behavior is undefined if `op` does not point to a function of
//   the specified signature.
//   * First argument should be a pointer to the flag register.
//   * Second argument should be the left-hand side value.
//   * Third argument should be the right-hand side value.
//   * Should return the value resulting from operating on the
//     second and third arguments.
// * pc_per_op: The number of instructions to expect the instruction
//   pointer to advance after each instruction.
// * div_per_op: The number of clock cycles to expect the emulated CPU
//   to advance after each instruction.
// * lhs_type: The type of the left-hand side argument.
//   Can be any of REG8, REG8_RO, or REG16PTR.
//   Behavior is undefined if any other operand type is specified.
// * lhs_sz: The number of elements in the array pointed to by `lhs`.
// * lhs: Pointer to an array of register indices to use as arguments.
//   Behavior is undefined if `lhs` does not point to an array of indices
//   with at least `lhs_sz` number of elements.
// * rhs_type: The type of the right-hand side arguments.
//   If either IMMED8 or NONE, `rhs_sz` and `rhs` are ignored.
// * rhs_sz: The number of elements in the array pointed to by `rhs`.
// * rhs: Pointer to an array of register indices to use as arguments.
//   If `rhs_type` is not IMMED8 or NONE, then behavior is undefined
//   if `rhs` does not point to an array of indices with at least
//   `rhs_sz` number of elements
//
// Returns: 0 on success, 1 on failure
//=======================================================================
static int_fast8_t
testcpu8(
		const char* restrict name,
		uint_fast16_t prog_sz,
		const uint8_t* restrict prog,
		uint8_t (*op)(uint8_t* restrict, uint8_t, uint8_t),
		uint_fast8_t pc_per_op,
		uint_fast8_t div_per_op,
		enum testcpu_type8 lhs_type,
		uint_fast8_t lhs_sz,
		const uint8_t lhs[lhs_sz],
		enum testcpu_type8 rhs_type,
		uint_fast8_t rhs_sz,
		const uint8_t rhs[rhs_sz]
) {
	twi_assertf(lhs_type != TESTCPU_TYPE_BIT,
			"Left-hand side argument(s) cannot be bit offsets.");
	twi_assertf(lhs_type != TESTCPU_TYPE_IMMED8,
			"Left-hand side argument(s) cannot be immediates.");
	twi_assertf(lhs_type != TESTCPU_TYPE_NONE,
			"Left-hand side argument(s) must be present for this test format.");

	// The control and test CPU and memory map.
	struct twi_gb_cpu expected_cpu, actual_cpu;
	struct twi_gb_mem expected_mem, actual_mem;
	testcpu_reset(&expected_cpu);
	testcpu_setprog(&expected_mem, prog_sz, prog, rhs_type == TESTCPU_TYPE_IMMED8 ? 1 : 0);
	memcpy(&actual_cpu, &expected_cpu, sizeof(struct twi_gb_cpu));
	memcpy(&actual_mem, &expected_mem, sizeof(struct twi_gb_mem));

	// If `rhs_type` is IMMED8 or NONE, then `rhs` is unused,
	// then the inner loop should run once.
	// Loop logic will not utilize `rhs` values when inappropriate.
	if (rhs_type == TESTCPU_TYPE_IMMED8 || rhs_type == TESTCPU_TYPE_NONE)
		rhs_sz = 1;

	for (uint8_t lhs_pos = 0; lhs_pos < lhs_sz; ++lhs_pos) {
		for (uint8_t rhs_pos = 0; rhs_pos < rhs_sz; ++rhs_pos) {
			// Use pseudorandom test values
			// Prepare expected and actual components for test.
			uint8_t lhs_val = testcpu_setrand8(
					&expected_cpu, &actual_cpu,
					&expected_mem, &actual_mem,
					lhs_type, lhs[lhs_pos]
			);

			uint8_t rhs_val;
			if (rhs_type == TESTCPU_TYPE_REG8
			 || rhs_type == TESTCPU_TYPE_REG8_RO
			 || rhs_type == TESTCPU_TYPE_REG16PTR) {
				rhs_val = testcpu_setrand8(
						&expected_cpu, &actual_cpu,
						&expected_mem, &actual_mem,
						rhs_type, rhs[rhs_pos]
				);
			} else if (rhs_type == TESTCPU_TYPE_BIT) {
				// rhs is hard-coded into the instruction itself
				rhs_val = rhs[rhs_pos];
			} else if (rhs_type == TESTCPU_TYPE_IMMED8) {
				// rhs was randomly-generated when setting the program
				rhs_val = twi_gb_mem_read8(&expected_mem, expected_cpu.pc + 1);
			} else { // TESTCPU_TYPE_NONE
				// rhs value will be discarded by instruction and thus is irrelevant.
				rhs_val = 0;
			}

			// Evaluate result, and simulate in expected components.
			//fprintf(stderr, "%s: %" PRIX8 " %" PRIX8"\n", name, lhs_val, rhs_val);
			uint8_t result = op(&REG8(&expected_cpu, IDX_F), lhs_val, rhs_val);
			if (lhs_type == TESTCPU_TYPE_REG8)
				REG8(&expected_cpu, lhs[lhs_pos]) = result;
			else if (lhs_type == TESTCPU_TYPE_REG16PTR)
				twi_gb_mem_write8(&expected_mem, REG16(&expected_cpu, lhs[lhs_pos]), result);
			expected_cpu.pc += pc_per_op;
			expected_cpu.div += div_per_op;

			// Execute instruction in actual components.
			interpret_once(&actual_cpu, &actual_mem);

			// Compare (twi_test will jump to the `test_fail` label on failure).
			twi_test(!memcmp(&expected_cpu, &actual_cpu, sizeof(struct twi_gb_cpu)));
			twi_test(!memcmp(&expected_mem, &actual_mem, sizeof(struct twi_gb_mem)));
		} // end iteration through `rhs`
	} // end iteration through `lhs`

	return 0;
test_fail: // Jumped to by twi_test().
	fprintf(stderr, "Test name: %s\nOpcode: 0x%" PRIX8 "\n",
			name, twi_gb_mem_read8(&expected_mem, expected_cpu.pc - pc_per_op));
	twi_gb_cpu_diffdump(stderr, "expected", &expected_cpu, "actual", &actual_cpu);
	// TODO: mem diffdump
	return 1;
} // end testcpu8()

