#include <stdio.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/cpu/opc.h"

int main() {
	struct gb_core core;
	core.cpu.pc = 0;
	for (size_t i = 0; i < 256; ++i) {
		core.mem.map[0] = i;
		struct gb_opc_components cmp;
		gb_opc_current_components(&cmp, &core);
		char dasm[256];
		gb_opc_string(dasm, sizeof(dasm), &cmp, NULL);
		printf("%02X       : %s\n", i, dasm);
	}

	return 0;
}

