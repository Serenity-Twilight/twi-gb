//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
// Defines the struct which contains the state of the emulated
// Picture Processing Unit (PPU) as well as the interfaces for managing
// its state and drawing images to a window.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_PPU_H
#define TWI_GB_PPU_H

#include <stdint.h>
#include <SDL2/SDL_video.h>
#include <twi/gb/mem.h>

//=======================================================================
//-----------------------------------------------------------------------
// Public type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb_ppu
// def struct twi_gb_ppu
// 
// Owns and manages and window and renderer to facilitate drawing
// to the user's screen.
//
// No function outside of the interfaces declared in this file, as well
// as the helper functions they utilize, should read or write from any
// member of an object of this type. No guarantees are made as to the
// values of object variables read, and it is undefined behavior for
// an external user to write to any of this object's variables.
//-----------------------------------------------------------------------
// Members (ALL INTERNAL):
// * window: The window handle owned by a PPU object.
// * gl: The rendering context owned by a PPU object.
// * fbo: A handle to a framebuffer object used for offscreen rendering.
// * vram_buf: A handle to a buffer object used for communicating
//   Game Boy VRAM data to the GPU.
// * oamctl_buf: A handle to a buffer object used for communicating
//   Game Boy OAM and CTL register data to the GPU.
// * winx0: The x-coordinate of the leftmost column of drawable area on
//   the window, inclusive.
// * winx1: The x-coordinate of the rightmost column of drawable area on
//   the window, exclusive.
// * winy0: The y-coordinate of the bottommost row of drawable area on
//   the window, inclusive.
// * winy1: The y-coordinate of the topmost row of drawable area on the
//   window, exclusive.
// * vram_seg: The next memory segment of `vram_buf` to be utilized for
//   buffering VRAM data onto the GPU.
// * oamctl_seg: The next memory segment of `oamctl_buf` to be utilized
//   for buffering OAM + CTL data onto the GPU.
// * vram_num_segs: The total number of equally-sized segments which
//   `vram_buf` is split into.
// * oamctl_num_segs: The total number of equally-sized segments which
//   `oamctl_buf` is split into.
// * flags: Internal state flags.
//=======================================================================
struct twi_gb_ppu {
	SDL_Window* window;
	SDL_GLContext gl;
	uint32_t fbo;
	uint32_t vram_buf;
	uint32_t oamctl_buf;
	uint16_t winx0;
	uint16_t winx1;
	uint16_t winy0;
	uint16_t winy1;
	uint8_t vram_seg;
	uint8_t oamctl_seg;
	uint8_t vram_num_segs;
	uint8_t oamctl_num_segs;
	uint8_t flags;
}; // end struct twi_gb_ppu

//=======================================================================
//-----------------------------------------------------------------------
// Public function declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl twi_gb_ppu_init()
// 
// Initializes the PPU object pointed to by `ppu` to a default state.
//
// Behavior is undefined if `ppu` does not point to an unitialized
// memory block of equal or greater size of a PPU object and with
// matching alignment to a PPU object.
//-----------------------------------------------------------------------
// Parameters:
// * ppu: Pointer to an unitialized PPU object.
//  TODO: Needs to know if DMG or CGB.
//  TODO: Needs access to user display settings.
//-----------------------------------------------------------------------
// Returns: 0 on success, or non-zero on failure
//=======================================================================
int_fast8_t
twi_gb_ppu_init(struct twi_gb_ppu* ppu);

//=======================================================================
// decl twi_gb_ppu_destroy()
//
// Destroys a previously initialized PPU object pointed to by `ppu`.
//
// Behavior is undefined if `ppu` does not point to a previously
// initialized PPU object that has not yet been destroyed.
//-----------------------------------------------------------------------
// Parameters:
// * ppu: Pointer to a previously initialized, undestroyed PPU object.
//=======================================================================
void
twi_gb_ppu_destroy(struct twi_gb_ppu* ppu);

//=======================================================================
// decl twi_gb_ppu_draw()
// TODO: This signature is not final.
//=======================================================================
void
twi_gb_ppu_draw(struct twi_gb_ppu* ppu, const struct twi_gb_mem* mem);

//=======================================================================
// decl twi_gb_ppu_onchange_resolution()
//
// Notifies the PPU object that its window has changed resolution.
//
// Behavior is undefined if `ppu` does not point to a previously
// initialized PPU object that has not yet been destroyed.
//-----------------------------------------------------------------------
// Parameters:
// * ppu: Pointer to a previously initialized, undestroyed PPU object.
// TODO: Accept window ID to ensure that this window is the one that
// gets updated.
//=======================================================================
void
twi_gb_ppu_onchange_resolution(struct twi_gb_ppu* ppu);

#endif // TWI_GB_PPU_H

