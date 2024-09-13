//=======================================================================
//-----------------------------------------------------------------------
// incl/gb/pak.h
//
// The primary header for MBC (Memory Bank Controller) emulation.
//
//
//-----------------------------------------------------------------------
//=======================================================================
#ifndef GB_PAK_MBC_H
#define GB_PAK_MBC_H
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"

//=======================================================================
// doc enum mbc_ids
//
// Listing of all known & supported MBCs (Memory Bank Controllers).
// The following enumeration provides an localized index for uniquely
// defining each MBC that is indendepent from their native identification
// in the Game Boy header.
//=======================================================================
// def enum mbc_ids
enum mbc_ids {
	PAKMBC_UNKNOWN = -1,
	PAKMBC_NONE = 0,
	// Commented out MBCs are currently unsupported:
	//PAKMBC_MBC1,
	//PAKMBC_MBC2,
	//PAKMBC_MMM01,
	//PAKMBC_MBC3,
	//PAKMBC_MBC5,
	//PAKMBC_MBC6,
	//PAKMBC_MBC7,
	//PAKMBC_M161, what is the pak_type code for this?
	//PAKMBC_POCKETCAM,
	//PAKMBC_TAMA5,
	//PAKMBC_HuC3,
	//PAKMBC_HuC1,
	// TODO: Populate as support for more MBCs are added.
	PAKMBC_COUNT
}; // end enum mbc_ids

//=======================================================================
// doc typedef mbc_write8_proc()
// 
// Abstract interface definition for writing to an MBC-controlled region
// of memory.
//
// Each MBC must have a pair of functions implementing this interface
// (one for ROM writes, one for RAM writes) so that each MBC may
// have its specific behavior emulated.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
// * map:
//   Pointer to a region of memory representing a mapped copy of either
//   a ROM bank or a RAM bank owned by `pak`.
// * addr:
//   16-bit address pointing to a region in either ROM or RAM.
// * val:
//   8-bit value being written to the byte pointed to by `addr`.
//=======================================================================
// def typedef mbc_write8_proc()
typedef void (*mbc_write8_proc)(
		struct gb_pak* restrict pak,
		uint8_t* restrict map,
		uint16_t addr, uint8_t val);

//=======================================================================
// doc struct mbc_write8_pair
// doc mbc_write8[]
//
// Array of ROM+RAM write functions.
// Each object in the array represents the write functions of its
// index's respective `mbc_id`.
//
// For example:
// `mbc_write8[PAKMBC_NONE]` contains the write functions for paks
// which possess no MBC.
//=======================================================================
// def struct mbc_write8_pair
extern const struct mbc_write8_pair {
	mbc_write8_proc rom;
	mbc_write8_proc ram;
} mbc_write8[];

#endif // GB_PAK_MBC_H

