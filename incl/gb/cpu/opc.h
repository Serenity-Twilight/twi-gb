#ifndef GB_CPU_OPC_H
#define GB_CPU_OPC_H
#include <stdint.h>
#include "gb/core.h"

//=======================================================================
//-----------------------------------------------------------------------
// External type definitions
//-----------------------------------------------------------------------
//=======================================================================
// def struct gb_opc_components
struct gb_opc_components {
	uint8_t oper_id;
	uint8_t opnd1_id;
	uint8_t opnd2_id;
}; // end struct gb_opc_components

//=======================================================================
//-----------------------------------------------------------------------
// External function declarations
//-----------------------------------------------------------------------
//=======================================================================
void
gb_opc_current_components(
		struct gb_opc_components* restrict dst,
		const struct gb_core* restrict core);
void
gb_opc_components_at(
		struct gb_opc_components* restrict dst,
		const struct gb_core* restrict core,
		uint16_t offset);
size_t
gb_opc_string(
		char* restrict buf, size_t bufsz,
		const struct gb_opc_components* restrict opc,
		const struct gb_core* restrict core);

#endif // GB_CPU_OPC_H

