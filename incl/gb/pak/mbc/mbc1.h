//=======================================================================
//-----------------------------------------------------------------------
// gb/pak/mbc/mbc1.h
//
// Contains functions for handling ROM and RAM write behavior for
// paks containing the MBC1 chip.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef GB_PAK_MBC_MBC1_H
#define GB_PAK_MBC_MBC1_H
#include <stdint.h>
#include "gb/pak.h"

//=======================================================================
// doc mbc_w8_mbc1_rom()
//
// Executes behavior which occurs during ROM writes in paks containing
// an MBC1 chip.
//
// Implements the `mbc_w8_proc` interface, defined in incl/gb/pak/mbc.h
//-----------------------------------------------------------------------
// Parameters:
// Detailed by the `mbc_w8_proc` documentation,
// available in incl/gb/pak/mbc.h
//-----------------------------------------------------------------------
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `ram_map` does not point to an array of at least PAK_ROM_BANK_SIZE
//   bytes in size (defined in incl/gb/pak/const.h)
// - `ram_map` does not point to an array of at least PAK_RAM_BANK_SIZE
//   bytes in size (defined in incl/gb/pak/const.h)
// - `addr >= PAK_ROM_BANK_SIZE * 2`
//=======================================================================
void
mbc_w8_mbc1_rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

//=======================================================================
// doc mbc_w8_mbc1_ram()
//
// Executes behavior which occurs during RAM writes in paks containing
// an MBC1 chip.
//
// Implements the `mbc_w8_proc` interface, defined in incl/gb/pak/mbc.h
//
// The argument `rom_map` is never accessed, and may be NULL without error.
//-----------------------------------------------------------------------
// Parameters:
// Detailed by the `mbc_w8_proc` documentation,
// available in incl/gb/pak/mbc.h
//-----------------------------------------------------------------------
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `ram_map` does not point to an array of at least PAK_RAM_BANK_SIZE
//   bytes in size (defined in incl/gb/pak/const.h)
// - `addr >= PAK_RAM_BANK_SIZE`
//=======================================================================
void
mbc_w8_mbc1_ram(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_MBC1_H

