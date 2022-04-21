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
#include <inttypes.h>
#include <stdint.h>
#include <../src/twi/gb/cpu.c>
#include <twi/std/test.h> // MUST COME AFTER SRC FILE INCLUDE!

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
static int_fast8_t test_LD_R_R();

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
	size_t fail_count = 0;
	fail_count += test_LD_R_R();
	return fail_count;
} // end twi_gb_test_cpu_all()

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def twi_gb_test_cpu_init()
// TODO
//=======================================================================
static void twi_gb_test_cpu_init(struct twi_gb_cpu* restrict cpu) {
	for (uint_fast8_t i = 0; i < sizeof(cpu->reg); ++i)
		cpu->reg[i] = 0;
	cpu->sp = 0;
	cpu->pc = 0;
	cpu->div = 0;
} // end test_twi_gb_cpu_init()

//=======================================================================
// def reg_str()
//
// Returns a character string representation of the indicated
// 8-bit register. 
//-----------------------------------------------------------------------
// Parameters:
// * cpu: The CPU that owns the register indicated by `reg`.
// * reg: The register to identify.
//
// Returns: A pointer to a constant character string, or NULL if the
// provided register didn't match any of the registers owned by `cpu`.
//=======================================================================
static const char*
r_str(const struct twi_gb_cpu* cpu, const uint8_t* reg) {
	if (reg == &REG_A)
		return "REG_A";
	else if (reg == &REG_F)
		return "REG_F";
	else if (reg == &REG_B)
		return "REG_B";
	else if (reg == &REG_C)
		return "REG_C";
	else if (reg == &REG_D)
		return "REG_D";
	else if (reg == &REG_E)
		return "REG_E";
	else if (reg == &REG_H)
		return "REG_H";
	else if (reg == &REG_L)
		return "REG_L";
	else
		return NULL;
}

// Redefine PTR macros defined in twi-gb to use local variables.
#undef CPU_PTR
#undef MEM_PTR
#define CPU_PTR (&cpu)
#define MEM_PTR (&mem)

//=======================================================================
// def test_LD_R_R()
//
// Tests the functionality of register-to-register loads.
//-----------------------------------------------------------------------
// Returns: 0 on success, 1 on failure
//=======================================================================
static int_fast8_t
test_LD_R_R() {
	static const uint_fast8_t begin_op = 0x40; // LD B, B
	static const uint_fast8_t HLM_ops = 0x70; // Skip these
	static const uint_fast8_t A_HLM_op = 0x7E; // Skip this
	uint8_t PROG_PTR[64];
	uint_fast16_t prog_pos = 0;

	//--------------------------------
	// Populate program with opcodes.
	uint_fast8_t op;
	for (op = begin_op; op < HLM_ops; ++op) {
		for (uint_fast8_t mem_op = op + 6; op < mem_op; ++op)
			PROG_PTR[prog_pos++] = op;
		++op; // Skip over memory-to-register loads
		PROG_PTR[prog_pos++] = op;
	}
	// Skip register-to-memory loads
	for (op += 0x8; op < A_HLM_op; ++op)
		PROG_PTR[prog_pos++] = op;
	++op; // Skip over memory-to-register load
	PROG_PTR[prog_pos] = op;
	//--------------------------------

	struct twi_gb_cpu cpu;
	struct twi_gb_mem mem;
	twi_gb_test_cpu_init(&cpu);
	// All 8-bit registers, minus F.
	uint8_t* reg[] = {&REG_B, &REG_C, &REG_D, &REG_E, &REG_H, &REG_L, &REG_A};
	uint16_t expected_pc = 1;
	uint16_t expected_div = 4;
	uint8_t num = expected_pc * expected_div; // Arbitrary number.
	uint_fast8_t reg_oidx, reg_iidx; // Outer/Inner register index.

	const size_t reg_len = sizeof(reg) / sizeof(uint8_t*);
	for (reg_oidx = 0; reg_oidx < reg_len; ++reg_oidx) {
		for (reg_iidx = 0; reg_iidx < reg_len; ++reg_iidx) {
			// Test instruction pointer and CPU clock counter.
			*(reg[reg_oidx]) = !num;
			*(reg[reg_iidx]) = num;
			interpret_once(CPU_PTR, MEM_PTR, PROG_PTR);
			// Ensure value was loaded, and the source has not changed.
			twi_test(cpu.pc == expected_pc);
			twi_test(cpu.div == expected_div);
			twi_test(*(reg[reg_oidx]) == num);
			twi_test(*(reg[reg_iidx]) == num);
			// Increment expectations.
			++expected_pc;
			expected_div += 4;
			num += expected_pc * expected_div; // Arbitrary, non-linear increment.
		} // end inner iteration over registers
	} // end outer iteration over registers
	
	return 0;
test_fail: // Jumped to by twi_test().
	fprintf(stderr, "Previous opcode: 0x%" PRIx8 "\n"
			"lhs: %s = %" PRIu8 " (expected %" PRIu8 ")\n"
			"rhs: %s = %" PRIu8 " (expected %" PRIu8 ")\n"
			"Expected PC = %" PRIu16 "; Actual PC = %" PRIu16 "\n"
			"Expected DIV = %" PRIu16 "; Actual DIV = %" PRIu16 "\n",
			PROG_PTR[cpu.pc - 1],
			r_str(&cpu, reg[reg_oidx]), *(reg[reg_oidx]), num,
			r_str(&cpu, reg[reg_iidx]), *(reg[reg_iidx]), num,
			expected_pc, cpu.pc, expected_div, cpu.div
	);
	return 1;
} // end test_LD_R_R()

