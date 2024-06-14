#ifndef GB_PAK_MBC_COMMON_H
#define GB_PAK_MBC_COMMON_H
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"

//=======================================================================
// decl mbc_sram_write()
// TODO: documentation
//=======================================================================
void
mbc_sram_write(
		struct gb_mem* restrict mem,
		struct gb_pak* restrict pak,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_COMMON_H

