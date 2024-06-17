#ifndef GB_MEM_H
#define GB_MEM_H
#include <stdint.h>
#include "gb/core.h"
#include "gb/ppu.h"

// TODO: Structure containing bank-swappable memory + metadata.
// All models: ROM, SRAM
// CGB-only: VRAM, RAM
// NOTE: ROM & SRAM behavior dependent on MBC
// Delegate to MBC, which is owned by the pak, which is owned by mem.
//struct gb_bankmem {
//}; // end struct gb_bankmem

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL TYPE DECLARATIONS
// (defined in gb/mem/typedef.h)
//-----------------------------------------------------------------------
//=======================================================================
struct gb_mem;

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL GLOBAL VARIABLE DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
// FIXME: This is a hack to get custom ROM loading available immediately.
// Replace once the pak loader is ready to integrate.
extern const char* gb_mem_rom_filepath;

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================
uint8_t
gb_mem_init(struct gb_core* restrict core);
uint8_t
gb_mem_direct_read(const struct gb_core* restrict core, uint16_t addr);
uint8_t
gb_mem_u8read(const struct gb_core* restrict core, uint16_t addr);
int8_t
gb_mem_s8read(const struct gb_core* restrict core, uint16_t addr);
uint8_t
gb_mem_u8readff(const struct gb_core* restrict core, uint16_t addr);
uint16_t
gb_mem_u16read(const struct gb_core* restrict core, uint16_t addr);
void
gb_mem_u8write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value);
void
gb_mem_u8writeff(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value);
void
gb_mem_u16write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint16_t value);
void
gb_mem_copy_ppu_state(
		struct gb_core* restrict core,
		struct gb_ppu_state* restrict dst);
void
gb_mem_set_pad(struct gb_core* restrict core, uint8_t gb_pad);

#endif // GB_MEM_H

