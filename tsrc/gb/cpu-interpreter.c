#include <stdio.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/mem.h"
#include "src/gb/cpu-interpreter.c"

struct u8range {
	uint8_t begin;
	uint8_t end;
}; // end struct u8range

struct DAA_test_case {
	struct u8range msrange;
	struct u8range lsrange;
	uint8_t init_N;
	uint8_t init_H;
	uint8_t init_C;
	int8_t expected_adjustment;
	uint8_t expected_C;
}; // end struct DAA_test_case

static size_t
test_DAA() {
	static const struct DAA_test_case tests[] = {
		{ {0x0, 0xA}, {0x0, 0xA}, 0, 0, 0, 0x00, 0 },
		{ {0x0, 0x9}, {0xA, 0x10}, 0, 0, 0, 0x06, 0 },
		{ {0x0, 0xA}, {0x0, 0x4}, 0, 1, 0, 0x06, 0 },
		{ {0xA, 0x10}, {0x0, 0xA}, 0, 0, 0, 0x60, 1 },
		{ {0x9, 0x10}, {0xA, 0x10}, 0, 0, 0, 0x66, 1 },
		{ {0xA, 0x10}, {0x0, 0x4}, 0, 1, 0, 0x66, 1 },
		{ {0x0, 0x3}, {0x0, 0xA}, 0, 0, 1, 0x60, 1 },
		{ {0x0, 0x3}, {0xA, 0x10}, 0, 0, 1, 0x66, 1 },
		{ {0x0, 0x4}, {0x0, 0x4}, 0, 1, 1, 0x66, 1 },
		{ {0x0, 0xA}, {0x0, 0xA}, 1, 0, 0, 0x00, 0 },
		{ {0x0, 0x9}, {0x6, 0x10}, 1, 1, 0, -0x06, 0 },
		{ {0x7, 0x10}, {0x0, 0xA}, 1, 0, 1, -0x60, 1 },
		{ {0x6, 0x10}, {0x6, 0x10}, 1, 1, 1, -0x66, 1 }
	};
	struct gb_core core = {0};
	gb_mem_write8(&core, 0, 0x27); // DAA opcode

	size_t num_tests = (sizeof tests) / sizeof(struct DAA_test_case);
	printf("num_tests = %zu\n", num_tests);
	for (size_t t = 0; t < num_tests; ++t) {
		// msn = more-significant nybble
		uint8_t msn = tests[t].msrange.begin;
		for (; msn < tests[t].msrange.end; ++msn) {
			// lsn = less-significant nybble
			uint8_t lsn = tests[t].lsrange.begin;
			for (; lsn < tests[t].lsrange.end; ++lsn) {
				core.cpu.pc = 0; // Point to DAA opcode
				uint8_t A_begin = ((msn << 4) | lsn);
				core.cpu.r[iA] = A_begin;
				core.cpu.fn = tests[t].init_N;
				core.cpu.fh = tests[t].init_H;
				core.cpu.fc = tests[t].init_C;
				
				printf("[%4zu]: A = 0x%02zX, N = %hhu, H = %hhu, C = %hhu\n",
						t+1, core.cpu.r[iA], core.cpu.fn, core.cpu.fh, core.cpu.fc);
				interpret_once(&core);
				uint8_t A_expected = A_begin + tests[t].expected_adjustment;
				printf("     -> A = 0x%02zX,               C = %hhu\n"
						   "Expect: A = 0x%02zX,               C = %hhu\n",
						core.cpu.r[iA], core.cpu.fc, A_expected, tests[t].expected_C);
				if (core.cpu.r[iA] != A_expected || core.cpu.fc != tests[t].expected_C)
					return t+1;
			} // end iteration over lsrange
		} // end iteration over msrange
	} // end iteration over tests[]
	return 0;
} // end test_DAA

int main() {
	size_t error;
	if (error = test_DAA())
		fprintf(stderr, "test_DAA(): Error on case %zu\n", error);
	else
		fprintf(stdout, "DAA: OK\n");
} // end main()

