#ifndef GB_PAK_HEADER
#define GB_PAK_HEADER
#include <stdint.h>
#include <stdio.h>

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc struct pakhdr_alloc_info
// TODO: documentation
//=======================================================================
struct pakhdr_alloc_info {
	// Raw alloc info bytes from ROM header:
	uint8_t pak_type_code;
	uint8_t rom_size_code;
	uint8_t ram_size_code;
	// Decoded meaning behind raw alloc bytes:
	uint8_t mbc_id;
	unsigned int battery : 1;
	uint8_t rom_bank_count;
	uint8_t ram_bank_count;
	// Raw sizes of ROM and RAM in bytes, rather than banks:
	size_t rom_size;
	size_t ram_size;
}; // end struct pakhdr_alloc_info

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
int
pakhdr_get_alloc_info(
		struct pakhdr_alloc_info* restrict dst,
		FILE* restrict rom_file);
size_t
pakhdr_dump(char* restrict buf, size_t bufsz,
		const void* restrict rom,
		const struct pakhdr_alloc_info* restrict ainfo);

#endif // GB_PAK_HEADER

