#ifndef GB_PAK_MBC_COMMON_H
#define GB_PAK_MBC_COMMON_H
#include <stdint.h>
#include "gb/pak/typedef.h"

//=======================================================================
// doc mbc_ram_write()
// TODO: documentation
//=======================================================================
void
mbc_ram_write(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_MBC_COMMON_H

