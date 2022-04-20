#include <stdint.h>
#include <twi/std/assertf.h>
#include <../src/twi/gb/cpu.c>

#define TWI_ASSERT_ACTION (return 1)

test_twi_gb_cpu_init(struct twi_gb_cpu* restrict cpu) {
	for (uint_fast8_t i = 0; i < sizeof(cpu->reg); ++i)
		cpu->reg[i] = 0;
	cpu->sp = 0;
	cpu->pc = 0;
	cpu->div = 0;
} // end test_twi_gb_cpu_init()

// Test: Register-to-register loads
int_fast8_t
test_twi_gb_cpu_LD_R_R(
		struct twi_gb_cpu* CPU_PTR,
		struct twi_gb_mem* MEM_PTR
) {
	static const uint_fast8_t begin_op = 0x40; // LD B, B
	static const uint_fast8_t end_op = 0x80;
	uint8_t PROG_PTR[64];
	uint_fast16_t prog_pos = 0;

	// Populate program with opcodes.
	for (uint_fast8_t op = begin_op; op < end_op; ++op) {
		for (uint_fast8_t mem_op = op + 6; op < mem_op; ++op)
			PROG_PTR[prog_pos++] = op;
		++op; // Skip over memory-to-register loads
		PROG_PTR[prog_pos++] = op;
	}

	// All 8-bit registers, minus F.
	uint8_t* reg[] = {&REG_B, &REG_C, &REG_D, &REG_E, &REG_H, &REG_L, &REG_A};
	uint16_t expected_pc = 0;
	uint16_t expected_div = 0;
	uint8_t num = 1; // Arbitrary number.

	test_twi_gb_cpu_init(CPU_PTR);
	for (uint_fast8_t reg_oidx = 0; reg_oidx < sizeof(reg); ++reg_oidx) {
		for (uint_fast8_t reg_iidx = 0; reg_iidx < sizeof(reg); ++reg_iidx) {
			// Test instruction pointer and CPU clock counter.
			twi_assert_test(CPU_PTR->pc == expected_pc, PRIu16, expected_pc, CPU_PTR->pc);
			twi_assert_test(CPU_PTR->div == expected_div, PRIu16, expected_div, CPU_PTR->pc);
			*(reg[reg_oidx]) = 0;
			*(reg[reg_iidx]) = num;
			cpu_interpret(CPU_PTR, MEM_PTR, PROG_PTR);
			// Ensure value was loaded, and the source has not changed.
			twi_assert_test(*(reg[reg_oidx]) == num, PRIu8, num, *(reg[reg_oidx]));
			twi_assert_test(*(reg[reg_iidx]) == num, PRIu8, num, *(reg[reg_iidx]));
			// Increment expectations.
			++expected_pc;
			expected_div += 4;
			num += expected_pc + expected_div; // Arbitrary, non-linear increment.
			if (num == 0) {
				num += expected_pc + expected_div;
				if (num == 0)
					num = 1;
			}
		} // end inner iteration over registers
	} // end outer iteration over registers
} // end test_twi_gb_cpu_LD()
