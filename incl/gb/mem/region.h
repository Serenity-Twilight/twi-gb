#ifndef GB_MEM_REGION_H
#define GB_MEM_REGION_H

//=======================================================================
// doc enum gb_mem_region
// TODO
//=======================================================================
// def enum gb_mem_region
enum gb_mem_region {
	// Beginning addresses for memory-mapped regions
	MEM_B_ROM1 = 0x0000, // Fixed ROM bank
	MEM_B_ROM2 = 0x4000, // Swappable ROM bank
	MEM_B_VRAM = 0x8000, // Video RAM
	MEM_B_SRAM = 0xA000, // Save RAM
	MEM_B_RAM1 = 0xC000, // Fixed Working RAM bank
	MEM_B_RAM2 = 0xD000, // Swappable Working RAM bank
	MEM_B_ERAM = 0xE000, // Echo RAM
	MEM_B_OAM  = 0xFE00, // Object Attribute Memory
	MEM_B_FORB = 0xFEA0, // Forbidden memory region
	MEM_B_IO   = 0xFF00, // I/O Registers
	MEM_B_HRAM = 0xFF80, // High RAM

	// Ending addresses for memory-mapped regions
	MEM_E_ROM1 = MEM_B_ROM2,
	MEM_E_ROM2 = MEM_B_VRAM,
	MEM_E_VRAM = MEM_B_SRAM,
	MEM_E_SRAM = MEM_B_RAM1,
	MEM_E_RAM1 = MEM_B_RAM2,
	MEM_E_RAM2 = MEM_B_ERAM,
	MEM_E_ERAM = MEM_B_OAM,
	MEM_E_OAM  = MEM_B_FORB,
	MEM_E_FORB = MEM_B_IO,
	MEM_E_IO   = MEM_B_HRAM,
	MEM_E_HRAM = 0xFFFF,
	
	// Memory region sizes
	MEM_SZ_ROM1 = MEM_E_ROM1 - MEM_B_ROM1,
	MEM_SZ_ROM2 = MEM_E_ROM2 - MEM_B_ROM2,
	MEM_SZ_ROM  = MEM_SZ_ROM1 + MEM_SZ_ROM2,
	MEM_SZ_VRAM = MEM_E_VRAM - MEM_B_VRAM,
	MEM_SZ_SRAM = MEM_E_SRAM - MEM_B_SRAM,
	MEM_SZ_RAM1 = MEM_E_RAM1 - MEM_B_RAM1,
	MEM_SZ_RAM2 = MEM_E_RAM2 - MEM_B_RAM2,
	MEM_SZ_RAM  = MEM_SZ_RAM1 + MEM_SZ_RAM2,
	MEM_SZ_ERAM = MEM_E_ERAM - MEM_B_ERAM,
	MEM_SZ_OAM  = MEM_E_OAM  - MEM_B_OAM,
	MEM_SZ_FORB = MEM_E_FORB - MEM_B_FORB,
	MEM_SZ_IO   = MEM_E_IO   - MEM_B_IO,
	MEM_SZ_HRAM = MEM_E_HRAM - MEM_B_HRAM,
}; // end enum gb_mem_region

#endif // GMEM_B_MEM_REGION_H

