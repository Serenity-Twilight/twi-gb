#ifndef GB_PAK_MBC_COMMON_H
#define GB_PAK_MBC_COMMON_H
#include <stdint.h>
#include "gb/pak/typedef.h"

//=======================================================================
// doc mbc_ram_write()
//
// Writes `val` to `ram_map` and `pak`'s backing `ram` member at
// the address specified by `addr`.
//
// If `pak` does not support RAM, then this function does nothing.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `ram_map` does not point to the start of a block of memory of
//   at least `PAK_RAM_BANK_SIZE` bytes in size
//   (defined in incl/gb/pak/const.h).
// - The value of `addr` does not point to an address lying within the
//   external RAM (SRAM) region of the Game Boy's memory map.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object through which to write to
//   its RAM, if any.
// * ram_map:
//   Pointer to an array of at least `PAK_RAM_BANK_SIZE` bytes,
//   serving as a copy of the active RAM bank.
// * addr:
//   16-bit address pointing to a byte in the Game Boy's external
//   RAM region.
// * val:
//   An 8-bit value to write to the simulated external RAM.
//=======================================================================
void
mbc_ram_write(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint16_t addr, uint8_t val);

//=======================================================================
// doc mbc_swap_rom_bank()
//
// Selects the ROM bank specified by `new_bank_id` to be the new active
// ROM bank for `pak`, and copies the new active ROM bank's contents
// to the memory region pointed to by `mapping_dst`.
//
// If `new_bank_id` matches the currently selected ROM bank, this
// function does nothing.
//
// If `new_bank_id` exceeds the number of ROM banks contained in `pak`,
// then `new_bank_id` will be treated as if it were the modulus (%) of
// `new_bank_id`.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `mapping_dst` does not point to the start of a block of memory of
//   at least `PAK_ROM_BANK_SIZE` bytes in size
//   (defined in incl/gb/pak/const.h).
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
//   Source of the copy of the newly-selected ROM bank.
// * mapping_dst:
//   Pointer to memory, destination of the copy of the newly-selected
//   ROM bank.
// * new_bank_id:
//   Unique index of the specified ROM bank to select.
//=======================================================================
void
mbc_swap_rom_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict mapping_dst,
		uint16_t new_bank_id);

//=======================================================================
// doc mbc_swap_ram_bank()
//
// Selects the RAM bank specified by `new_bank_id` to be the new active
// RAM bank for `pak`, and copies the new active RAM bank's contents
// to the memory region pointed to by `mapping_dst`.
//
// If `pak` does not support RAM, this function does nothing.
// If `new_bank_id` matches the currently selected RAM bank, this
// function does nothing.
//
// If `new_bank_id` exceeds the number of RAM banks contained in `pak`,
// then `new_bank_id` will be treated as if it were the modulus (%) of
// `new_bank_id`.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `mapping_dst` does not point to the start of a block of memory of
//   at least `PAK_RAM_BANK_SIZE` bytes in size
//   (defined in incl/gb/pak/const.h).
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
//   Source of the copy of the newly-selected RAM bank.
// * mapping_dst:
//   Pointer to memory, destination of the copy of the newly-selected
//   RAM bank.
// * new_bank_id:
//   Unique index of the specified RAM bank to select.
//=======================================================================
void
mbc_swap_ram_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict mapping_dst,
		uint8_t new_bank_id);

#endif // GB_PAK_MBC_COMMON_H

