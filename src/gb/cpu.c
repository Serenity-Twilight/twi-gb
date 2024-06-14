#include <inttypes.h>
#include <stdint.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/cpu/reg.h"
#include "gb/log.h"
#include "gb/mem.h"

//=======================================================================
// def gb_cpu_init()
void
gb_cpu_init(struct gb_core* restrict core) {
	core->cpu.r[iA] = 0x01;
	core->cpu.r[iF] = 0x00;
	core->cpu.r[iB] = 0x00;
	core->cpu.r[iC] = 0x13;
	core->cpu.r[iD] = 0x00;
	core->cpu.r[iE] = 0xD8;
	core->cpu.r[iH] = 0x01;
	core->cpu.r[iL] = 0x4D;
	core->cpu.sp = 0xFFFE;
	core->cpu.pc = 0x0100;
	core->cpu.fz = 0;
	core->cpu.fn = 0;
	core->cpu.fh = 0;
	core->cpu.fc = 0;
	core->cpu.state = 0;
} // end gb_cpu_init()

//=======================================================================
// def gb_cpu_interrupt()
void
gb_cpu_interrupt(struct gb_core* restrict core, uint8_t request) {
	LOGT("core=%p, request=%" PRIu8, core, request);
	if (request) core->cpu.state |= CPUSTATE_INTERRUPTED;
	else         core->cpu.state &= ~CPUSTATE_INTERRUPTED;
} // end gb_cpu_interrupt()

