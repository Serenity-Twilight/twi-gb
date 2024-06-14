#ifndef GB_PAK_MBC_NONE_H
#define GB_PAK_MBC_NONE_H
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"

//=======================================================================
// decl mbc_write8_rom_none()
// TODO: documentation
//=======================================================================
void
mbc_write8_rom_none(
		struct gb_mem* restrict,
		struct gb_pak* restrict,
		uint16_t, uint8_t);

//=======================================================================
// decl mbc_write8_ram_none()
// TODO: documentation
//=======================================================================
void
mbc_write8_ram_none(
		struct gb_mem* restrict mem,
		struct gb_pak* restrict pak,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_NONE_H

