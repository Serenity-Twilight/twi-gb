#ifndef GB_MEM_TYPEDEF_H
#define GB_MEM_TYPEDEF_H

//=======================================================================
// doc struct gb_mem
// TODO: document
//=======================================================================
// def struct gb_mem
struct gb_mem {
	struct gb_pak* pak;
	uint8_t map[0x10000]; // 64 KiB
	uint8_t stat_int;
	uint8_t ime;
	uint8_t pad;
}; // end struct gb_mem

#endif // GB_MEM_TYPEDEF_H

