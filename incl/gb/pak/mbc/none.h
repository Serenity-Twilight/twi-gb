#ifndef GB_PAK_MBC_NONE_H
#define GB_PAK_MBC_NONE_H
#include <stdint.h>
#include "gb/pak.h"

//=======================================================================
// doc mbc_w8_none_rom()
//
// Executes behavior which occurs during ROM writes in paks which do
// not possess an MBC.
//
// Implements the `mbc_w8_proc` interface,
// defined in incl/gb/pak/mbc.h
//=======================================================================
void
mbc_w8_none_rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

//=======================================================================
// doc mbc_w8_none_ram()
//
// Executes behavior which occurs during RAM writes in paks which do
// not possess an MBC.
//
// Implements the `mbc_w8_proc` interface,
// defined in incl/gb/pak/mbc.h
//=======================================================================
void
mbc_w8_none_ram(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_NONE_H

