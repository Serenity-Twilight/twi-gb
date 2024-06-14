#ifndef GB_PAK_MBC_H
#define GB_PAK_MBC_H
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"

enum mbc_ids {
	PAKMBC_UNKNOWN = -1,
	PAKMBC_NONE = 0,
	PAKMBC_MBC1,
	PAKMBC_MBC2,
	PAKMBC_MMM01,
	PAKMBC_MBC3,
	PAKMBC_MBC5,
	PAKMBC_MBC6,
	PAKMBC_MBC7,
	//PAKMBC_M161, what is the pak_type code for this?
	PAKMBC_POCKETCAM,
	PAKMBC_TAMA5,
	PAKMBC_HuC3,
	PAKMBC_HuC1,
	// TODO: Populate as support for more MBCs are added.
	PAKMBC_COUNT
}; // end enum mbc_ids

typedef void (*mbc_write8_proc)(
		struct gb_mem* restrict mem,
		struct gb_pak* restrict pak,
		uint16_t addr, uint8_t val);

extern const struct mbc_write8_pair {
	mbc_write8_proc rom;
	mbc_write8_proc ram;
} mbc_write8[];

#endif // GB_PAK_MBC_H

