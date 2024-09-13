#ifndef GB_PAK_MBC_NONE_H
#define GB_PAK_MBC_NONE_H
#include <stdint.h>
#include "gb/pak.h"

//=======================================================================
// decl mbc_write8_rom_none()
//
// Executes behavior which occurs during ROM writes in paks which do
// not possess an MBC.
//
// Implements the `mbc_write8_proc` interface,
// defined in incl/gb/pak/mbc.h
//=======================================================================
void
mbc_write8_rom_none(
		struct gb_pak* restrict,
		uint8_t* restrict,
		uint16_t, uint8_t);

//=======================================================================
// decl mbc_write8_ram_none()
//
// Executes behavior which occurs during RAM writes in paks which do
// not possess an MBC.
//
// Implements the `mbc_write8_proc` interface,
// defined in incl/gb/pak/mbc.h
//=======================================================================
void
mbc_write8_ram_none(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_NONE_H

