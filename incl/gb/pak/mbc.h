//=======================================================================
//-----------------------------------------------------------------------
// incl/gb/mbc.h
//
// The primary header for MBC (Memory Bank Controller) emulation.
//
// All versions of the Game Boy utilize a 64 KiB memory map addressable
// by 16-bit values. Of these 64 KiB, 32 KiB is allocated to the ROM
// and 8 KiB is allocated to the pak's external RAM. For games with
// larger ROM/RAM requirements than this, MBCs exist.
//
// MBCs, or Memory Bank Controllers, are devices included in most
// Game Boy paks that allow for swapping out active "banks" of ROM/RAM.
// When a bank is swapped in and becomes active, its contents are mapped
// into the Game Boy's memory map and become accessible. Therefore,
// games with larger ROM images or external RAM capacity can be run
// by swapping out banks at runtime.
//
// More information available from the PanDocs at:
// https://gbdev.io/pandocs/MBCs.html
//-----------------------------------------------------------------------
//=======================================================================
#ifndef GB_PAK_MBC_H
#define GB_PAK_MBC_H
#include <stdint.h>
#include "gb/mem.h"
#include "gb/pak.h"
//---------------------------
// MBC handler declarations:
#include "gb/pak/mbc/none.h"
//---------------------------

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL CONSTANT DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc enum mbc_id
//
// Listing of all known & supported MBCs (Memory Bank Controllers).
// The following enumeration provides an localized index for uniquely
// defining each MBC that is indendepent from their native identification
// in the Game Boy header.
//=======================================================================
// def enum mbc_id
enum mbc_id {
	PAKMBC_UNKNOWN = -1,
	PAKMBC_NONE = 0,
	PAKMBC_SUPPORTED_COUNT,
	// The following MBCs are currently unsupported:
	PAKMBC_MBC1,
	PAKMBC_MBC2,
	PAKMBC_MMM01,
	PAKMBC_MBC3,
	PAKMBC_MBC5,
	PAKMBC_MBC6,
	PAKMBC_MBC7,
	PAKMBC_M161, // what is the pak_type code for this?
	PAKMBC_POCKETCAM,
	PAKMBC_TAMA5,
	PAKMBC_HuC3,
	PAKMBC_HuC1,
	PAKMBC_COUNT
	// TODO: Populate as support for more MBCs are added.
}; // end enum mbc_id

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

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
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc mbc_id_tostring()
// TODO
//=======================================================================
const char*
mbc_id_tostring(enum mbc_id id);

#endif // GB_PAK_MBC_H

