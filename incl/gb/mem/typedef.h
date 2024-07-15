#ifndef GB_MEM_TYPEDEF_H
#define GB_MEM_TYPEDEF_H
#include <stdalign.h>

//=======================================================================
// doc struct gb_mem
// TODO: document
//=======================================================================
// def struct gb_mem
struct gb_mem {
	alignas(max_align_t) uint8_t map[0x10000]; // 64 KiB
	struct gb_pak* pak; // Currently-inserted game pak.
	uint8_t stat_int; // STAT interrupt line
	uint8_t ime; // Interrupt Master Enable
	uint8_t pad; // D-Pad + button input buffer
}; // end struct gb_mem

#endif // GB_MEM_TYPEDEF_H

