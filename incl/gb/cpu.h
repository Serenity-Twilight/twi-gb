#ifndef GB_CPU_H
#define GB_CPU_H
#include <stdalign.h>
#include <stdint.h>
#include "gb/core/decl.h"

//=======================================================================
//-----------------------------------------------------------------------
// External constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc enum gb_cpu_state
// Enumerates all the states that the emulated CPU can be in.
// CPU state serves 2 purposes:
// 1. To inform the CPU that it should stop executing.
//    Relevant states: STOPPED, HALTED
// 2. To indicate where the CPU left off before it stopped running,
//    which serves to identify what the CPU should start doing upon
//    resuming.
//    Relevant states: TODO
//-----------------------------------------------------------------------
// Enumerations:
// * CPUSTATE_RUNNING
//   Indicates that the CPU is running. Standard mode.
//   Defined by the absence of any other state.
// * CPUSTATE_INTERRUPTED
//   Indicates that an enabled interrupt has requested to be handled.
//   This bit will only be set if IME is set and the requesting
//   interrupt has been enabled in the IE I/O register.
// * CPUSTATE_HALTED
//   Indicates that the last instruction run was HALT.
//   TODO: Describe HALT mode.
// * CPUSTATE_STOPPED
//   Indicates that the last instruction run was STOP.
//   TODO: Describe STOP mode. Don't forget to explain speed-switching.
//=======================================================================
enum gb_cpu_state {
	CPUSTATE_RUNNING     = 0x00,
	CPUSTATE_INTERRUPTED = 0x01,
	CPUSTATE_HALTED      = 0x02,
	CPUSTATE_STOPPED     = 0x04,
	CPUSTATE_TIMEDOUT    = 0x08
}; // end enum gb_cpu_state

//=======================================================================
//-----------------------------------------------------------------------
// External type definitions
//-----------------------------------------------------------------------
//=======================================================================
// TODO: Move CPU documentation here from gb/core.h.
struct gb_cpu {
	alignas(uint16_t) uint8_t r[8];
	uint16_t sp;
	uint16_t pc;
	uint8_t fz;
	uint8_t fn;
	uint8_t fh;
	uint8_t fc;
	uint8_t state;
};

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================
void
gb_cpu_init(struct gb_core* restrict core);

//=======================================================================
// doc gb_cpu_interrupt()
// TODO
//-----------------------------------------------------------------------
// Parameters:
// * core
//   The gb_core which owns the targeted CPU.
// * request
//   Whether or not to interrupt the CPU.
//=======================================================================
void
gb_cpu_interrupt(struct gb_core* restrict core, uint8_t request);

#endif // GB_CPU_H

