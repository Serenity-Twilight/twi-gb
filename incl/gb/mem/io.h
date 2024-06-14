#ifndef GB_MEM_IO_H
#define GB_MEM_IO_H
#include <stdint.h>
#include "gb/core.h"

//=======================================================================
//-----------------------------------------------------------------------
// External constant definitions
//-----------------------------------------------------------------------
//=======================================================================
enum io_offset {
	IO_JOYP = 0xFF00, // Joypad status
	// --- Serial data transfer ---
	IO_SB   = 0xFF01, // Serial transfer data
	IO_SC   = 0xFF02, // Serial transfer control
	// --- Timer registers ---
	IO_DIV  = 0xFF04, // Divider register
	IO_TIMA = 0xFF05, // Timer counter
	IO_TMA  = 0xFF06, // Timer modulo
	IO_TAC  = 0xFF07, // Timer control
	// -----------------------
	IO_IF   = 0xFF0F, // Interrupt request register
	// --- Audio control registers ---
	// Channel 1 audio
	IO_NR10 = 0xFF10, // Channel 1 sweep
	IO_NR11 = 0xFF11, // Channel 1 length timer & duty cycle
	IO_NR12 = 0xFF12, // Channel 1 volume & envelope
	IO_NR13 = 0xFF13, // Channel 1 period (low bits)
	IO_NR14 = 0xFF14, // Channel 1 period (high bits) & control
	// Channel 2 audio
	IO_NR21 = 0xFF16, // Channel 2 length timer & duty cycle
	IO_NR22 = 0xFF17, // Channel 2 volume & envelope
	IO_NR23 = 0xFF18, // Channel 2 period (low bits)
	IO_NR24 = 0xFF19, // Channel 2 period (high bits) & duty cycle
	// Channel 3 audio
	IO_NR30 = 0xFF1A, // Channel 3 DAC enable
	IO_NR31 = 0xFF1B, // Channel 3 length timer
	IO_NR32 = 0xFF1C, // Channel 3 output level
	IO_NR33 = 0xFF1D, // Channel 3 period (low bits)
	IO_NR34 = 0xFF1E, // Channel 3 period (high bits) & control
	// Channel 4 audio
	IO_NR41 = 0xFF20, // Channel 4 length timer
	IO_NR42 = 0xFF21, // Channel 4 volumne & envelope
	IO_NR43 = 0xFF22, // Channel 4 frequency
	IO_NR44 = 0xFF23, // Channel 4 control
	// Global audio
	IO_NR50 = 0xFF24, // Master volume and VIN panning
	IO_NR51 = 0xFF25, // Sound panning
	IO_NR52 = 0xFF26, // Audio master control
	// Wave pattern RAM (samples for channel 3, 16 bytes)
	IO_WAV0 = 0xFF30, IO_WAV1, IO_WAV2, IO_WAV3,
	IO_WAV4, IO_WAV5, IO_WAV6, IO_WAV7,
	IO_WAV8, IO_WAV9, IO_WAVA, IO_WAVB,
	IO_WAVC, IO_WAVD, IO_WAVE, IO_WAVF,
	// --- Video control registers ---
	IO_LCDC = 0xFF40, // LCD control
	IO_STAT = 0xFF41, // LCD status
	IO_SCY  = 0xFF42, // Background viewport position Y
	IO_SCX  = 0xFF43, // Background viewport position X
	IO_LY   = 0xFF44, // LCD Y-coordinate
	IO_LYC  = 0xFF45, // LY Compare
	IO_DMA  = 0xFF46, // OAM DMA (direct memory access) source address & start
	IO_BGP  = 0xFF47, // DMG BG palette
	IO_OBP0 = 0xFF48, // DMG OBJ palette 0
	IO_OBP1 = 0xFF49, // DMG OBJ palette 1
	IO_WY   = 0xFF4A, // Window Y position
	IO_WX   = 0xFF4B, // Window X position
	IO_VBK  = 0xFF4F, // Select VRAM bank (CGB-only)
	// -----------------------------
	IO_NOBT = 0xFF50, // Disable boot ROM
	IO_IE   = 0xFFFF, // Interrupt enable register
}; // end enum io_offset

//===========
//| IO_JOYP |
//===========
enum io_joyp_bit {
	// 0-3: Read-only
	IO_JOYP_A_RIGHT      = 0x01,
	IO_JOYP_B_LEFT       = 0x02,
	IO_JOYP_SELECT_UP    = 0x04,
	IO_JOYP_START_DOWN   = 0x08,
	// 4-5: Read/Write
	IO_JOYP_READ_DPAD    = 0x10,
	IO_JOYP_READ_BUTTONS = 0x20,
	// 6-7: Unused (always high)
	IO_JOYP_UNUSED       = 0xC0,
	
	IO_JOYP_INPUTS    = IO_JOYP_A_RIGHT
	                  | IO_JOYP_B_LEFT
	                  | IO_JOYP_SELECT_UP
	                  | IO_JOYP_START_DOWN,
	IO_JOYP_MASKS     = IO_JOYP_READ_DPAD
	                  | IO_JOYP_READ_BUTTONS,
	IO_JOYP_WRITABLE  = IO_JOYP_MASKS
}; // end enum io_joyp_bit

//===========
//| IO_TAC  |
//===========
enum io_tac_bit {
	// Read/Write bits
	IO_TAC_CLOCK_SELECT_LSB = 0x01,
	IO_TAC_CLOCK_SELECT_MSB = 0x02,
	IO_TAC_CLOCK_SELECT = IO_TAC_CLOCK_SELECT_LSB
	                    | IO_TAC_CLOCK_SELECT_MSB,
	IO_TAC_ENABLE = 0x04,

	IO_TAC_READWRITE = IO_TAC_CLOCK_SELECT
	                 | IO_TAC_ENABLE
}; // end enum io_tac_bit

//===========
//| IO_IF   |
//| IO_IE   |
//===========
enum io_ife_bit {
	// 0-4: read/write
	IO_IFE_VBLANK = 0x01,
	IO_IFE_STAT   = 0x02,
	IO_IFE_TIMER  = 0x04,
	IO_IFE_SERIAL = 0x08,
	IO_IFE_JOYP   = 0x10,
	// 5-7: Unused (always high)
	IO_IFE_UNUSED = 0xE0,
	
	IO_IFE_WRITABLE = ~IO_IFE_UNUSED
}; // end enum io_ife_bit

//===========
//| IO_NRxy |
//===========
enum io_nrxy_bit {
	IO_NR10_WRITABLE = 0x7F,
	IO_NRx4_WRITABLE = 0xC7,
	IO_NR30_DAC_ENABLE = 0x80,
	IO_NR32_OUTPUT_LEVEL = 0x60,
	IO_NR41_LENGTH_TIMER = 0x3F,
	IO_NR44_WRITABLE = 0xC0,
	IO_NR52_AUDIO_ENABLE = 0x80
};

//===========
//| IO_LCDC |
//===========
enum io_lcdc_bit {
	// 0-7: Read/Write

	// On DMG:
	// 	0: Background+Window blanked (drawn using white pixels)
	// 	1: Background+Window drawn normally
	// On CGB:
	// 	0: Background+Window layer always drawn behind object layer
	// 	   Ignores any other priority settings
	// 	1: Background+Window layer priority defers to priority settings
	IO_LCDC_BG_ENABLED  = 0x01,
	IO_LCDC_OBJ_ENABLED = 0x02, // 0: Object layer not drawn | 1: Object layer drawn
	IO_LCDC_OBJ_SIZE    = 0x04,
	// Background tile map address:
	// 0: Tile mapping used from 0x9800-0x9BFF
	// 1: Tile mapping used from 0x9C00-0x9FFF
	IO_LCDC_BG_TILEMAP  = 0x08,
	// Background/Window tile data address:
	// 0: Indices 0-127 mapped to 0x9000-0x97FF
	// 1: Indices 0-127 mapped to 0x8000-0x87FF (shared with OBJ tile data)
	// Note that indices 128-255 are ALWAYS mapped to 0x8800-0x8FFF
	IO_LCDC_BG_TILEDATA = 0x10,
	IO_LCDC_WND_ENABLED = 0x20, // 0: Window not drawn | 1: Window drawn
	// Window tile map address:
	// 0: Tile mapping used from 0x9800-0x9BFF
	// 1: Tile mapping used from 0x9C00-0x9FFF
	IO_LCDC_WND_TILEMAP = 0x40,
	IO_LCDC_PPU_ENABLED = 0x80,
}; // end enum io_lcdc_bit

//===========
//| IO_STAT |
//===========
enum io_stat_bit {
	// 0-2: Read-only
	IO_STAT_MODE_LSB  = 0x01,
	IO_STAT_MODE_MSB  = 0x02,
	IO_STAT_MODE      = IO_STAT_MODE_MSB | IO_STAT_MODE_LSB,
	IO_STAT_LYMATCH   = 0x04,
	// 3-6: Read/Write
	IO_STAT_INT_MODE0 = 0x08,
	IO_STAT_INT_MODE1 = 0x10,
	IO_STAT_INT_MODE2 = 0x20,
	IO_STAT_INT_LYC   = 0x40,
	// 7: Unused (always high)
	IO_STAT_UNUSED    = 0x80,

	IO_STAT_WRITABLE = IO_STAT_INT_MODE0
	                 | IO_STAT_INT_MODE1
	                 | IO_STAT_INT_MODE2
	                 | IO_STAT_INT_LYC
}; // end enum io_stat_bit

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================
void
gb_mem_io_request_interrupt(
		struct gb_core* restrict core,
		enum io_ife_bit interrupt);
void
gb_mem_io_clear_interrupt(
		struct gb_core* restrict core,
		enum io_ife_bit interrupt);
uint8_t
gb_mem_io_pending_interrupts(struct gb_core* restrict core);
void
gb_mem_io_set_ime(struct gb_core* restrict core, uint8_t enable);
uint8_t
gb_mem_io_get_ime(const struct gb_core* restrict core);
void
gb_mem_io_set_lyc(struct gb_core* restrict core, uint8_t new_lyc);
uint8_t
gb_mem_io_advance_ppu(struct gb_core* restrict core);
void
gb_mem_io_increment_div(struct gb_core* restrict core);
void
gb_mem_io_increment_tima(struct gb_core* restrict core);
void
gb_mem_io_on_ifie_write(struct gb_core* restrict core);
void
gb_mem_io_update_joyp(struct gb_core* restrict core, uint8_t gb_pad);

#endif // GB_MEM_IO_H

