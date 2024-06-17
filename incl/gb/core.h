#ifndef GB_CORE_H
#define GB_CORE_H
#include <stdint.h>
#include "gb/ppu.h"

//=========================================================================
//-------------------------------------------------------------------------
// EXTERNAL TYPE DECLARATIONS
// (defined in incl/gb/core/typedef.h)
//-------------------------------------------------------------------------
//=========================================================================
struct gb_core;

//=========================================================================
//-------------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-------------------------------------------------------------------------
//=========================================================================
uint8_t
gb_core_init(struct gb_core* restrict core);
void
gb_core_run(struct gb_core* restrict core, struct gb_ppu* restrict ppu);
void
gb_core_set_pad(struct gb_core* restrict core, uint8_t gb_pad);

#endif // GB_CORE_H

