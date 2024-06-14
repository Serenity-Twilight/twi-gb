#ifndef GB_PAK_TYPE_H
#define GB_PAK_TYPE_H
#include <assert.h>
#include <stdint.h>
#include "gb/pak/mbc.h"

//=======================================================================
// def struct gb_pak
// TODO: documentation
//=======================================================================
// Assert that all MBC IDs can be uniquely-represented by uint8_t:
static_assert(PAKMBC_COUNT < UINT8_MAX);
struct gb_pak {
	void* rom;
	void* ram;
	char* save_filepath;
	uint16_t rom_bank_curr;
	uint16_t rom_bank_count;
	uint8_t ram_bank_curr;
	uint8_t ram_bank_count;
	uint8_t mbc_id;
	// Feature flags:
	unsigned int battery : 1;
	// State flags:
	unsigned int dirty_ram : 1;
}; // end struct gb_pak

#endif // GB_PAK_TYPE_H

