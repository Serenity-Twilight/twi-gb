#ifndef GB_PAK_MBC_COMMON_H
#define GB_PAK_MBC_COMMON_H
#include <stdint.h>
#include "gb/pak/typedef.h"

//=======================================================================
// doc mbc_ram_enable()
//
// Enables/disables R/W access to the RAM contained in `pak`.
//
// RAM reads (which are handled internally within the `gb/mem` module
// for performance reasons) when disabled should always return `0xFF`.
// This is simulated by setting the bytes within `ram_map` to 0xFF when
// `enable == 0`.
//
// If this function is called for a pak with no internal RAM, then this
// function does nothing.
//
// If this function is called with `enable != 0` when RAM access is
// already enabled or `enable == 0` when RAM access is already disabled,
// then this function does nothing.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `ram_map` does not point to the start of a block of memory of at
//   least `PAK_RAM_BANK_SIZE` bytes in size
//   (defined in incl/gb/pak/const.h)
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object to enable or disable the RAM of,
//   should it possess any internal RAM.
// * ram_map:
//   Pointer to an array of at least `PAK_RAM_BANK_SIZE` bytes,
//   assumed to be an active, memory-mapped copy of the active RAM bank.
//   When RAM is enabled, the currently active RAM bank is copied to this
//   address. When RAM is disabled, the bytes at this address are all set
//   to `0xFF`.
// * enable:
//   This function attempts to enable RAM when `enable != 0`,
//   and attempts to disable RAM when `enable == 0`.
//=======================================================================
void
mbc_ram_enable(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint_fast8_t enable);

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
// doc mbc_swap_rom1_bank()
//
// Selects the ROM bank specified by `new_bank_id` to be the new active
// ROM bank for `pak` in the ROM1 memory region, and copies the new
// active ROM bank's contents to the memory region pointed to by
// `rom_map`.
//
// This function implements niche behavior used only in specified MBCs.
// The `gb_pak` object does not define a member for tracking the
// currently selected bank in the ROM1 region, and this function will
// perform unnecessary writes to `rom_map` if requested by the caller.
// It's expected the caller, possessing superior context, will not call
// this function unnecessarily.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `rom_map` does not point to the start of a block of memory at least
//   `PAK_ROM_BANK_SIZE` bytes in size.
//   (defined in incl/gb/pak/const.h)
// - `new_bank_id` exceeds the number of ROM banks contained in `pak`.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
//   Source of the copy of the newly-selected ROM bank.
// * rom_map:
//   Pointer to the start of an external copy of the ROM map.
//   When this function returns, the `PAK_ROM_BANK_SIZE` bytes starting
//   at the address `rom_map` will reflect the contents of the mapped ROM
//   bank identified by `new_bank_id`.
// * new_bank_id:
//   Unique index of the specified ROM bank to select.
//=======================================================================
void
mbc_swap_rom1_bank(
		const struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t new_bank_id);

//=======================================================================
// doc mbc_swap_rom2_bank()
//
// Selects the ROM bank specified by `new_bank_id` to be the new active
// ROM bank for `pak` in the ROM2 memory region, and copies the new
// active ROM bank's contents to the memory region pointed to by
// `rom_map + PAK_ROM_BANK_SIZE`.
// (`PAK_ROM_BANK_SIZE` is defined in incl/gb/pak/const.h)
//
// If `new_bank_id` matches the currently selected ROM bank active in
// the ROM2 memory region, this function does nothing.
//
// Behavior is undefined if any of the following are true:
// - `pak` does not point to a valid `gb_pak` object.
// - `rom_map` does not point to the start of a block of memory of
//   at least `PAK_ROM_BANK_SIZE * 2` bytes in size
//   (defined in incl/gb/pak/const.h).
// - `new_bank_id` exceeds the number of ROM banks contained in `pak`.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
//   Source of the copy of the newly-selected ROM bank.
//   The value of its `rom_bank_curr` member is changed to match
//   `new_bank_id` by the time this function returns.
// * rom_map:
//   Pointer to the start of an external copy of the ROM map.
//   When this function returns, the `PAK_ROM_BANK_SIZE` bytes starting
//   at the address `rom_map + PAK_ROM_BANK_SIZE` will reflect the contents
//   of the mapped ROM bank identified by `new_bank_id`.
//   The first `PAK_ROM_BANK_SIZE` bytes (starting at `rom_map`) will
//   never be modified by this function.
// * new_bank_id:
//   Unique index of the specified ROM bank to select.
//=======================================================================
void
mbc_swap_rom2_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint16_t new_bank_id);

//=======================================================================
// doc mbc_swap_ram_bank()
//
// Selects the RAM bank specified by `new_bank_id` to be the new active
// RAM bank for `pak`, and copies the new active RAM bank's contents
// to the memory region pointed to by `ram_map`.
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
// - `ram_map` does not point to the start of a block of memory of
//   at least `PAK_RAM_BANK_SIZE` bytes in size
//   (defined in incl/gb/pak/const.h).
// - `new_bank_id` exceeds the number of RAM banks contained in `pak`.
//-----------------------------------------------------------------------
// Parameters:
// * pak:
//   Pointer to a valid `gb_pak` object.
//   Source of the copy of the newly-selected RAM bank.
// * ram_map:
//   Pointer to the start of an external copy of the RAM map.
//   When this function returns, the `PAK_RAM_BANK_SIZE` bytes starting
//   at the address `ram_map + PAK_RAM_BANK_SIZE` will reflect the contents
//   of the mapped RAM bank identified by `new_bank_id`.
//   The first `PAK_RAM_BANK_SIZE` bytes (starting at `ram_map`) will
//   never be modified by this function.
// * new_bank_id:
//   Unique index of the specified RAM bank to select.
//=======================================================================
void
mbc_swap_ram_bank(
		struct gb_pak* restrict pak,
		uint8_t* restrict ram_map,
		uint8_t new_bank_id);

#endif // GB_PAK_MBC_COMMON_H

