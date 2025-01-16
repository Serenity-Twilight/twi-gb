#ifndef GB_PAK_H
#define GB_PAK_H
#include <stdint.h>

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL TYPE DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct gb_pak
//
// Contains ROM & RAM content, pak feature data, and pak state data.
// See full definition in incl/gb/pak/typedef.h for more information.
//=======================================================================
struct gb_pak;

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc gb_pak_create()
//
// Creates a new `gb_pak` object from a pak identifier.
//
// At this time, pak identifiers are just ROM filepaths.
// The end design is for pak identifiers to be user-friendly unique
// identifiers for ROMs that combine with user-configured ROM directories
// to load the correct ROM files with minimal input strings.
//-----------------------------------------------------------------------
// Parameters:
// * pak_id:
//   Unique character string identifier for a game pak.
//
// Returns:
// On success, a newly created `gb_pak` object containing the specified
// pak's ROM data.
// On failure, returns NULL.
// Failure may occur under the following circumstances:
// - The function is unable to resolve `pak_id` into a specific pak.
// - Memory allocation fails when loading the specified pak.
//-----------------------------------------------------------------------
// Behavior is undefined if `pak_id == NULL`.
//=======================================================================
struct gb_pak*
gb_pak_create(const char* restrict pak_id);

//=======================================================================
// doc gb_pak_delete()
//
// Deletes an existing `gb_pak` object.
//
// TODO: At this time, battery-backed RAM is not saved to file.
//       Once this feature is implemented, this function will flush
//       unsaved data to file before deletion of the `gb_pak` object.
//
// After deletion, the deleted pak is no longer usable. Attempting to
// use a deleted pack will result in undefined behavior.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   The `gb_pak` object to delete.
//-----------------------------------------------------------------------
// Behavior is undefined if `pak` does not point to a previously
// created pak that has not yet been deleted, or if user code attempts
// to use a pak which has been deleted.
//=======================================================================
void
gb_pak_delete(struct gb_pak* restrict pak);

//=======================================================================
// doc gb_pak_insert()
// TODO: document
//=======================================================================
void
gb_pak_insert(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map);

//=======================================================================
// doc gb_pak_w8ram()
//
// Writes an 8-bit value (`val`) to the RAM of `pak` at the specified
// address `addr`. Writes and any side effects of them will be reflected
// both internally within `pak` and on the external `rom_map` and
// `ram_map`.
//
// Exact write behavior is dependent on the MBC utilized by `pak`
// (if any) and the current state of `pak`.
//-----------------------------------------------------------------------
// Parameters:
// TODO
//=======================================================================
void
gb_pak_w8ram(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

//=======================================================================
// doc gb_pak_w8rom()
// TODO: document
//=======================================================================
void
gb_pak_w8rom(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

#endif // GB_PAK_H

