#ifndef GB_CORE_TYPEDEF_H
#define GB_CORE_TYPEDEF_H
#include "gb/cpu.h"
#include "gb/mem/typedef.h"
#include "gb/sch.h"

// TODO: Move CPU documentation to cpu.h
//=========================================================================
// doc struct gb_core
// Core structure for emulating the Game Boy.
// Contains all state data required for maintaining the Game Boy
// between instructions.
//
// Game Boy state is divided into 3 sub-structures, described below:
// * cpu: GB cpu registers
// ** r: 8 8-bit general-purpose registers available to the GB CPU:
//       	A, F, B, C, D, E, H, L
//       These registers are also paired by certain operations
//       of the GB CPU, forming 16-bit registers:
//       	AF, BC, DE, HL
//       This has shaped the following aspects of the design of
//       their emulated representation:
//       * In C, arrays are the only way to guarantee no padding
//         is placed between variables by the compiler. Defining
//         all registers under a single array guarantees that
//         pairs of registers can be cast to (uint16_t) under all
//         Standard-conforming implementations.
//       * The array `r` is overaligned to 2-bytes, to guarantee
//         no alignment issues can occur when accessing register
//         pairs.
// ** sp: 16-bit stack pointer register. Primary specified purpose
//        is recording the address of the top of the executing ROM's
//        stack memory.
// ** pc: 16-bit program counter register. Points to the address of
//        the next instruction to be executed. Incremented by
//        instruction execution and overwritten by jump operations.
// ** fz, fh, fn, fc: Flag bits. They mirror the intended purpose
//        of the upper 4 bits of the F register. The emulator stores
//        them as distinct bytes in order to speed up modification
//        and access operations on them.
//
//        The following are descriptions of typical flag bit
//        behavior, but note that the actual affect each operation
//        has on flag bits may not be intuitive. Refer to individual
//        operations to confirm flag bit behavior.
//        * fz: Zero bit. Set when the result of an operation
//          is zero, and reset otherwise.
//        * fh: Half-carry bit. Set on an operation which
//          performs a carry from bit 3 to bit 4 or vice versa.
//          Reset otherwise.
//        * fn: Negative bit. Set on a subtraction operation.
//          Reset on an addition operation.
//        * fc: Carry bit: Set when the target register of an
//          operation overflows (carry to or from bit 7),
//          reset otherwise.
// * mem: Internal and external memory accessible by the Game Boy.
// ** TODO
// * sched: Emulation scheduling metadata.
// ** TODO
//
// Note: The splitting into sub-structures is done for logical
// division of component purpose to aid in understanding the core.
// In actual execution, the components interact with one another
// constantly, making it practical to define them all within a single
// core structure.
//=========================================================================
// def struct gb_core
struct gb_core {
	struct gb_cpu cpu;
	struct gb_mem mem;
	struct gb_sch sch;
}; // end struct gb_core

#endif // GB_CORE_TYPEDEF_H

