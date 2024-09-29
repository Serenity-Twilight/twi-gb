#include <assert.h>
#include "gb/pak/mbc.h"

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def mbc_id_tostring()
const char*
mbc_id_tostring(enum mbc_id id) {
	switch (id) {
		case PAKMBC_NONE: return "none";
		case PAKMBC_MBC1: return "mbc1";
		case PAKMBC_MBC2: return "mbc2";
		case PAKMBC_MMM01: return "mmm01";
		case PAKMBC_MBC3: return "mbc3";
		case PAKMBC_MBC5: return "mbc5";
		case PAKMBC_MBC6: return "mbc6";
		case PAKMBC_MBC7: return "mbc7";
		case PAKMBC_M161: return "m161";
		case PAKMBC_POCKETCAM: return "pocketcam";
		case PAKMBC_TAMA5: return "tama5";
		case PAKMBC_HuC3: return "huc3";
		case PAKMBC_HuC1: return "huc1";
		default: return "unknown";
	} // end switch (id)
} // end mbc_tostring()

