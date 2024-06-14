#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "gb/core.h"
#include "gb/cpu.h"
#include "gb/log.h"
#include "gb/mem.h"
#include "gb/mem/io.h"
#include "gb/mode.h"
#include "gb/pad.h"
#include "gb/ppu.h"

#define GB_LOG_MAX_LEVEL LVL_NONE

//=======================================================================
//-----------------------------------------------------------------------
// Internal type/constant definitions
//-----------------------------------------------------------------------
//=======================================================================
#define SELF (core->mem)

//=======================================================================
//-----------------------------------------------------------------------
// Internal function declarations
//-----------------------------------------------------------------------
//=======================================================================
static inline void
u8writef(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value);
static inline void
echo_ram_write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value);
static void
disable_audio(struct gb_core* restrict core);

//=======================================================================
//-----------------------------------------------------------------------
// External function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def gb_mem_init()
uint8_t
gb_mem_init(struct gb_core* restrict core) {
	FILE* tetris = fopen("tetris.gb", "rb");
	if (tetris == NULL) {
		fputs("Failed to open tetris.gb.\n", stderr);
		return 1;
	}
	if (fread(SELF.map, 1, 0x8000 /* 32 KiB */, tetris) != 0x8000) {
		fputs("Failed to read from tetris file.\n", stderr);
		return 1;
	}

	// Unmapped memory.
	SELF.stat_int = 0;
	SELF.ime = 0;
	// Other
	SELF.pak = NULL;
	SELF.pad = 0xFF; // Nothing pressed

#define IO(reg) (SELF.map[IO_##reg])
	IO(JOYP) = 0xCF;
	IO(SB)   = 0x00;
	IO(SC)   = 0x7E;
	IO(DIV)  = 0xAB;
	IO(TIMA) = 0x00;
	IO(TMA)  = 0x00;
	IO(TAC)  = 0xF8;
	IO(IF)   = 0xE1;
	// TODO: Audio registers
	IO(LCDC) = 0x91;
	IO(STAT) = 0x85;
	IO(SCY)  = 0x00;
	IO(SCX)  = 0x00;
	IO(LY)   = 0x00;
	IO(LYC)  = 0x00;
	IO(DMA)  = 0xFF;
	IO(BGP)  = 0xFC;
	// Object palette values are unpredictable.
	IO(WY)   = 0x00;
	IO(WX)   = 0x00;
	IO(IE)   = 0xE0;
#undef IO
	return 0;
} // end gb_mem_init()

//=======================================================================
// def gb_mem_direct_read()
uint8_t
gb_mem_direct_read(const struct gb_core* restrict core, uint16_t addr) {
	return core->mem.map[addr];
} // end gb_mem_direct_read()

//=======================================================================
// def gb_mem_u8read()
uint8_t
gb_mem_u8read(const struct gb_core* restrict core, uint16_t addr) {
	return core->mem.map[addr];
} // end gb_mem_u8read()

//=======================================================================
// def gb_mem_s8read()
int8_t
gb_mem_s8read(const struct gb_core* restrict core, uint16_t addr) {
	return *((int8_t*)(core->mem.map + addr));
} // end gb_mem_s8read()

//=======================================================================
// def gb_mem_u8readff()
uint8_t
gb_mem_u8readff(const struct gb_core* restrict core, uint16_t addr) {
	return core->mem.map[0xFF00|addr];
} // end gb_mem_u8readff()

//=======================================================================
// def gb_mem_u16read()
uint16_t
gb_mem_u16read(const struct gb_core* restrict core, uint16_t addr) {
	// TODO: Account for Big/Mixed Endian machines.
	// TODO: Account for proper memory write timings.
	return *((uint16_t*)(core->mem.map + addr));
} // end gb_mem_u16read()

//=======================================================================
// def gb_mem_u8write()
void
gb_mem_u8write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value) {
	//LOGT("gb_mem_u8write($%" PRIX16 ", $%" PRIX8 ")", addr, value);
	switch (addr >> 12) { // Address via upper nybble
		case 0x0: case 0x1: case 0x2: case 0x3: // ROM1
		case 0x4: case 0x5: case 0x6: case 0x7: // ROM2
		case 0xA: case 0xB: // SRAM
			// TODO: Pass to MBC
			return;
		case 0x8: case 0x9: // VRAM
			// TODO: On CGB, this needs to writethrough to backing VRAM array.
			// The backing array contains both banks of VRAM that CGB has.
			core->mem.map[addr] = value;
			return;
		case 0xC: case 0xD: // RAM1, RAM2
			// TODO: On CGB, must writethrough to backing RAM array that
			// contains all banks.
			core->mem.map[addr] = value;
			if (addr < MEM_E_ERAM - MEM_SZ_RAM) // Echo RAM range
				core->mem.map[addr + MEM_SZ_RAM] = value;
			return;
		case 0xE: // Echo RAM (most of it, rest shares 0xF range with many other registers).
			echo_ram_write(core, addr, value);
			return;
		case 0xF: // Range containing many different things.
			u8writef(core, addr, value);
			return;
	} // end switch 
	core->mem.map[addr] = value;
} // end gb_mem_u8write()

//=======================================================================
// TODO:
// * SC (not first revision)
// * Add input support.
// * Refuse writes to APU registers (other than NR52) when APU is off.
// * APU initial length timers are write-only, meaning that they will
//   not reflect written values when read. Length timer information
//   should be stored elsewhere, and the memory-mapped bits left
//   unchanged.
//=======================================================================
// def gb_mem_u8writeff()
void
gb_mem_u8writeff(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value) {
	addr |= 0xFF00;
#define IO_MASKED_WRITE(dst_addr, mask, src) \
	core->mem.map[dst_addr] &= ~(mask); \
	core->mem.map[dst_addr] |= ((src) & (mask))
	switch (addr) {
		// --- Simple writes ---
		// IO registers that do not contain read-only bits
		// nor invoke any special emulated behavior on write.
		case IO_SB:   // Serial transfer data
		case IO_TIMA: // Timer counter
		case IO_TMA:  // Timer modulo
		case IO_NR11: case IO_NR21: // Channel 1/2 length timer & duty cycle
		case IO_NR12: case IO_NR22: case IO_NR42: // Channel 1/2/4 volume & envelope
		case IO_NR13: case IO_NR23: case IO_NR33: // Channel 1-3 period (low bits)
		case IO_NR31: // Channel 3 length timer
		case IO_NR43: // Channel 4 frequency & randomness
		case IO_NR50: // Master volume & VIN panning
		case IO_NR51: // Sound panning
		// Wave pattern RAM
		case IO_WAV0: case IO_WAV1: case IO_WAV2: case IO_WAV3:
		case IO_WAV4: case IO_WAV5: case IO_WAV6: case IO_WAV7:
		case IO_WAV8: case IO_WAV9: case IO_WAVA: case IO_WAVB:
		case IO_WAVC: case IO_WAVD: case IO_WAVE: case IO_WAVF:
		case IO_SCY:  // Background viewport position Y
		case IO_SCX:  // Background viewport position X
		case IO_BGP:  // DMG BG palette
		case IO_OBP0: // DMG OBJ palette 0
		case IO_OBP1: // DMG OBJ palette 1
		case IO_WY:   // Window Y position
		case IO_WX:   // Window X position
		case IO_NOBT: // Disable boot ROM
			core->mem.map[addr] = value;
			return;
		// --- Complex writes ---
		// IO registers that either partially or wholly contain
		// read-only bits or trigger special behavior on write.
		case IO_JOYP: // Joypad status
			IO_MASKED_WRITE(addr, IO_JOYP_WRITABLE, value);
			gb_mem_io_update_joyp(core, core->mem.pad);
			return;
		case IO_SC: // Serial transfer control
			// TODO: Not implemented
			return;
		case IO_DIV: // Timer divider
			core->mem.map[IO_DIV] = 0; // Always reset, regardless of write value.
			gb_sch_on_div_reset(core);
			return;
		case IO_TAC: // Timer control
			gb_sch_on_tac_update(core, core->mem.map[IO_TAC], value);
			IO_MASKED_WRITE(addr, IO_TAC_READWRITE, value);
			return;
		case IO_IF: // Interrupt request
		case IO_IE: // Interrupt enable
			if (addr == IO_IF)
				LOGT("IO_IF = 0x%02" PRIX8, value);
			else
				LOGT("IO_IE = 0x%02" PRIX8, value);
			//fgetc(stdin);
			IO_MASKED_WRITE(addr, IO_IFE_WRITABLE, value);
			gb_mem_io_on_ifie_write(core);
			return;
		case IO_NR10: // Channel 1 sweep
			IO_MASKED_WRITE(addr, IO_NR10_WRITABLE, value);
			return;
		case IO_NR14: case IO_NR24: case IO_NR34: // Channel 1-3 period (high bits) & control
			IO_MASKED_WRITE(addr, IO_NRx4_WRITABLE, value);
			return;
		case IO_NR30: // Channel 3 DAC enable
			IO_MASKED_WRITE(addr, IO_NR30_DAC_ENABLE, value);
			return;
		case IO_NR32: // Channel 3 output level
			IO_MASKED_WRITE(addr, IO_NR32_OUTPUT_LEVEL, value);
			return;
		case IO_NR41: // Channel 4 initial length timer
			IO_MASKED_WRITE(addr, IO_NR41_LENGTH_TIMER, value);
			return;
		case IO_NR44: // Channel 4 control
			IO_MASKED_WRITE(addr, IO_NR44_WRITABLE, value);
			return;
		case IO_NR52: // Audio master control
			if (core->mem.map[addr] & IO_NR52_AUDIO_ENABLE) {
				if (!(value & IO_NR52_AUDIO_ENABLE))
					disable_audio(core);
			} else
				IO_MASKED_WRITE(addr, IO_NR52_AUDIO_ENABLE, value);
			return;
		case IO_LCDC: // LCD control
			gb_sch_on_lcdc_update(core, core->mem.map[IO_LCDC], value);
			core->mem.map[IO_LCDC] = value;
			return;
		case IO_STAT: // LCD status
			IO_MASKED_WRITE(addr, IO_STAT_WRITABLE, value);
			return;
		case IO_LY: // LCD Y-coordinate
			return; // Read-only
		case IO_LYC: // LY Compare
			gb_mem_io_set_lyc(core, value);
			return;
		case IO_DMA: // OAM DMA source address & start
			// TODO: This should prevent the game from using the data bus
			//       for 160 cycles.
			if (value > 0xDF)
				value = 0xDF; // TODO: Investigate and emulate proper behavior.
			memcpy(core->mem.map + MEM_B_OAM, core->mem.map + value * 0x100, MEM_SZ_OAM);
		default:
			if (addr >= MEM_B_HRAM)
				core->mem.map[addr] = value; // HRAM write, no side-effects
			return;
	} // end switch
} // end gb_mem_u8writeff()

//=======================================================================
// def gb_mem_u16write()
void
gb_mem_u16write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint16_t value) {
	// TODO: Account for Big/Mixed Endian machines.
	// TODO: Lookup destination address efficiently.
	// TODO: Account for proper memory write timings.
	gb_mem_u8write(core, addr  ,  value       & 0xFF);
	gb_mem_u8write(core, addr+1, (value >> 8) & 0xFF);
} // end gb_mem_u16write()

//=======================================================================
// def gb_mem_copy_ppu_state()
void
gb_mem_copy_ppu_state(
		struct gb_core* restrict core,
		struct gb_ppu_state* restrict dst) {
	LOGT("start: core=%p, dst=%p", core, dst);
	dst->mode = GBMODE_DMG;
	memcpy(dst->vram, core->mem.map + MEM_B_VRAM, MEM_SZ_VRAM);
	memcpy(dst->oam, core->mem.map + MEM_B_OAM, MEM_SZ_OAM);
	static_assert(sizeof(uint8_t*) >= 3);
//	LOGD("Before palette copy.");
//	intptr_t intpal;
//	uint8_t* palette;
//	intpal = (((intptr_t)(core->mem.map[IO_BGP])) << 16);
//	LOGD("intpal (1) = %p", intpal);
//	palette = (uint8_t*)intpal;
//	LOGD("palette (1) = %p", palette);
//	palette |= (uint8_t*)(((intptr_t)(core->mem.map[IO_OBP1])) << 8);
//	LOGD("palette (2) = %p", palette);
//	palette |= (uint8_t*)((intptr_t)(core->mem.map[IO_OBP0]));
//	LOGD("palette (3) = %p", palette);
	dst->palette = (uint8_t*)(
			(((intptr_t)(core->mem.map[IO_BGP ])) << 16) |
			(((intptr_t)(core->mem.map[IO_OBP1])) <<  8) |
			  (intptr_t)(core->mem.map[IO_OBP0])
	);
	dst->lcdc = core->mem.map[IO_LCDC];
	dst->scy = core->mem.map[IO_SCY];
	dst->scx = core->mem.map[IO_SCX];
	dst->wy = core->mem.map[IO_WY];
	dst->wx = core->mem.map[IO_WX];
	LOGT("returning");
} // end gb_mem_copy_ppu_state()

void
gb_mem_set_pad(struct gb_core* restrict core, uint8_t gb_pad) {
	core->mem.pad = gb_pad;
	gb_mem_io_update_joyp(core, gb_pad);
} // end gb_mem_set_pad()

//=======================================================================
//-----------------------------------------------------------------------
// Internal function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc u8write()
// TODO
//=======================================================================
// def u8writef()
static inline void
u8writef(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value) {
	switch (addr >> 8) { // Address via upper byte
		case 0xF0: case 0xF1: case 0xF2: case 0xF3:
		case 0xF4: case 0xF5: case 0xF6: case 0xF7:
		case 0xF8: case 0xF9: case 0xFA: case 0xFB:
		case 0xFC: case 0xFD: // Echo RAM range
			echo_ram_write(core, addr, value);
			return;
		case 0xFE: // OAM (or prohibited region following it)
			// Ignore writes to prohibited region
			// -OR- to OAM during PPU modes 2 and 3.
			if (addr < MEM_E_OAM && (core->mem.map[IO_STAT] & IO_STAT_MODE) < 2)
				core->mem.map[addr] = value;
			return;
		case 0xFF: // I/O registers + HRAM
			gb_mem_u8writeff(core, addr, value);
			return;
	} // end switch
} // end u8writef()

//=======================================================================
// doc echo_ram_write()
// TODO
//=======================================================================
// def echo_ram_write()
static inline void
echo_ram_write(
		struct gb_core* restrict core,
		uint16_t addr,
		uint8_t value) {
	core->mem.map[addr] = value;              // ECHO RAM write
	core->mem.map[addr - MEM_SZ_RAM] = value; // RAM write
} // end echo_ram_write()

//=======================================================================
//=======================================================================
// def disable_audio()
static void
disable_audio(struct gb_core* restrict core) {
	// Clear channel 1 register bits
	core->mem.map[IO_NR10] = ~IO_NR10_WRITABLE;
	core->mem.map[IO_NR11] = 0;
	core->mem.map[IO_NR12] = 0;
	core->mem.map[IO_NR13] = 0;
	core->mem.map[IO_NR14] = (uint8_t)~IO_NRx4_WRITABLE;
	// Clear channel 2 register bits
	core->mem.map[IO_NR21] = 0;
	core->mem.map[IO_NR22] = 0;
	core->mem.map[IO_NR23] = 0;
	core->mem.map[IO_NR24] = (uint8_t)~IO_NRx4_WRITABLE;
	// Clear channel 3 register bits
	core->mem.map[IO_NR30] = (uint8_t)~IO_NR30_DAC_ENABLE;
	core->mem.map[IO_NR31] = 0;
	core->mem.map[IO_NR32] = ~IO_NR32_OUTPUT_LEVEL;
	core->mem.map[IO_NR33] = 0;
	core->mem.map[IO_NR34] = (uint8_t)~IO_NRx4_WRITABLE;
	// Clear channel 4 register bits
	core->mem.map[IO_NR41] = ~IO_NR41_LENGTH_TIMER;
	core->mem.map[IO_NR42] = 0;
	core->mem.map[IO_NR43] = 0;
	core->mem.map[IO_NR44] = (uint8_t)~IO_NR44_WRITABLE;
	// Clear channel 5 register bits
	core->mem.map[IO_NR50] = 0;
	core->mem.map[IO_NR51] = 0;
	core->mem.map[IO_NR52] = 0;
	// Clear wave pattern RAM
	memset(core->mem.map + IO_WAV0, 0, (IO_WAVF+1) - IO_WAV0);
} // end disable_audio()

