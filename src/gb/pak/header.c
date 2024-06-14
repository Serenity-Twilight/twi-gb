#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define GB_LOG_MAX_LEVEL LVL_TRC
#include "gb/log.h"
#include "gb/pak/const.h"
#include "gb/pak/header.h"
#include "gb/pak/mbc.h"
#define PRX_TRUNCATE_PREFIX 1
#include "prx/incbuf.h"

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc enum pakhdr_addr
// TODO
//=======================================================================
// def enum pakhdr_addr
enum pakhdr_addr {
	// Header content addresses
	PAKHDR_LOGO            = 0x0104,
	PAKHDR_TITLE           = 0x0134,
	PAKHDR_MANUCODE        = 0x013F,
	PAKHDR_CGB_FLAG        = 0x0143,
	PAKHDR_NEW_LICENSEE    = 0x0144,
	PAKHDR_SGB_FLAG        = 0x0146,
	PAKHDR_PAK_TYPE        = 0x0147,
	PAKHDR_ROM_SIZE        = 0x0148,
	PAKHDR_RAM_SIZE        = 0x0149,
	PAKHDR_DST_CODE        = 0x014A,
	PAKHDR_OLD_LICENSEE    = 0x014B,
	PAKHDR_VERSION_NUM     = 0x014C,
	PAKHDR_PAKHDR_CHECKSUM = 0x014D,
	PAKHDR_GBL_CHECKSUM    = 0x014E,

	// Multi-byte header content ending addresses
	PAKHDR_E_LOGO         = PAKHDR_TITLE,
	PAKHDR_E_TITLEv0      = PAKHDR_NEW_LICENSEE,
	PAKHDR_E_TITLEv1      = PAKHDR_CGB_FLAG,
	PAKHDR_E_TITLEv2      = PAKHDR_MANUCODE,
	PAKHDR_E_NEW_LICENSEE = PAKHDR_SGB_FLAG,
	PAKHDR_E_GBL_CHECKSUM = 0x0150,

	// Multi-byte header content sizes
	PAKHDR_LOGO_SZ         = PAKHDR_E_LOGO - PAKHDR_LOGO,
	PAKHDR_TITLE_SZv0      = PAKHDR_E_TITLEv0 - PAKHDR_TITLE,
	PAKHDR_TITLE_SZv1      = PAKHDR_E_TITLEv1 - PAKHDR_TITLE,
	PAKHDR_TITLE_SZv2      = PAKHDR_E_TITLEv2 - PAKHDR_TITLE,
	PAKHDR_NEW_LICENSEE_SZ = PAKHDR_E_NEW_LICENSEE - PAKHDR_NEW_LICENSEE,
	PAKHDR_GBL_CHECKSUM_SZ = PAKHDR_E_GBL_CHECKSUM - PAKHDR_GBL_CHECKSUM,
}; // end enum pakhdr_addr

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static const char*
cgb_flag_string(uint8_t cgb_flag);
//-------------------------------------
static int
decode_alloc_info(struct pakhdr_alloc_info* restrict ainfo);
//-------------------------------------
static int
decode_pak_type(struct pakhdr_alloc_info* restrict ainfo);
//-------------------------------------
static inline int
decode_ram_size(struct pakhdr_alloc_info* restrict ainfo);
//-------------------------------------
static inline int
decode_rom_size(struct pakhdr_alloc_info* restrict ainfo);
//-------------------------------------
static void
dump_logo_check(struct incbuf* restrict ibuf, const void* restrict rom);
//-------------------------------------
static void
dump_title(struct incbuf* restrict ibuf, const void* restrict rom);
//-------------------------------------
static int
fread_alloc_info(
		struct pakhdr_alloc_info* restrict dst,
		FILE* restrict rom_file);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
// def pakhdr_get_alloc_info()
int
pakhdr_get_alloc_info(
		struct pakhdr_alloc_info* restrict dst,
		FILE* restrict rom_file) {
	LOGT("call(dst=%p, rom_file=%p)", dst, rom_file);
	int status;
	if (status = fread_alloc_info(dst, rom_file)) {
		LOGT("return %d", status);
		return status; // failure
	}

	status = decode_alloc_info(dst);
	LOGT("return %d", status);
	return status;
} // end pakhdr_get_alloc_info()

//=======================================================================
// def pakhdr_log_dump()
size_t
pakhdr_dump(char* restrict buf, size_t bufsz,
		const void* restrict rom,
		const struct pakhdr_alloc_info* restrict ainfo) {
	assert(rom != NULL);
	struct incbuf ibuf = { 
		.buf = buf,
		.bufsz = (buf != NULL ? bufsz : 0),
		.pos = 0
	};

	dump_logo_check(&ibuf, rom);
	dump_title(&ibuf, rom);

	incbuf_terminate(&ibuf);
	return ibuf.pos;
} // end pakhdr_dump()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
// def cgb_flag_string()
static const char*
cgb_flag_string(uint8_t cgb_flag) {
	if (cgb_flag == 0xC0)
		return "required";
	else if (cgb_flag == 0x80)
		return "enhanced";
	else
		return "compatible";
} // end cgb_flag_string()

//=======================================================================
// def decode_alloc_info()
static int
decode_alloc_info(struct pakhdr_alloc_info* restrict ainfo) {
	LOGT("call(ainfo=%p)", ainfo);
	if (decode_pak_type(ainfo))
		goto failure;
	if (decode_rom_size(ainfo))
		goto failure;
	if (decode_ram_size(ainfo))
		goto failure;

	LOGT("return 0");
	return 0;
failure:
	LOGT("return 1");
	return 1;
} // end decode_alloc_info()

//=======================================================================
// def decode_pak_type()
static int
decode_pak_type(struct pakhdr_alloc_info* restrict ainfo) {
	LOGT("call(ainfo=%p)", ainfo);
	assert(ainfo != NULL);
	switch (ainfo->pak_type_code) {
		case 0x00: // ROM ONLY
		case 0x08: // ROM+RAM
		case 0x09: // ROM+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_NONE;
			ainfo->battery = (ainfo->pak_type_code == 0x09);
			break;
		case 0x01: // MBC1
		case 0x02: // MBC1+RAM
		case 0x03: // MBC1+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_MBC1;
			ainfo->battery = (ainfo->pak_type_code == 0x03);
			break;
		case 0x05: // MBC2
		case 0x06: // MBC2+BATTERY
			ainfo->mbc_id = PAKMBC_MBC2;
			ainfo->battery = (ainfo->pak_type_code == 0x06);
			break;
		case 0x0B: // MMM01
		case 0x0C: // MMM01+RAM
		case 0x0D: // MMM01+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_MMM01;
			ainfo->battery = (ainfo->pak_type_code == 0x0D);
			break;
		case 0x0F: // MBC3+TIMER+BATTERY
		case 0x10: // MBC3+TIMER+RAM+BATTERY
		case 0x11: // MBC3
		case 0x12: // MBC3+RAM
		case 0x13: // MBC3+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_MBC3;
			ainfo->battery = !(ainfo->pak_type_code == 0x11
			                || ainfo->pak_type_code == 0x12);
			break;
		case 0x19: // MBC5
		case 0x1A: // MBC5+RAM
		case 0x1B: // MBC5+RAM+BATTERY
		case 0x1C: // MBC5+RUMBLE
		case 0x1D: // MBC5+RUMBLE+RAM
		case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_MBC5;
			ainfo->battery = (ainfo->pak_type_code == 0x1B
			               || ainfo->pak_type_code == 0x1E);
			break;
		case 0x20: // MBC6
			ainfo->mbc_id = PAKMBC_MBC6;
			ainfo->battery = 1;
			break;
		case 0x22: // MBC7+SENSOR+RUMBLE+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_MBC7;
			ainfo->battery = 1;
			break;
		case 0xFC: // POCKET CAMERA
			ainfo->mbc_id = PAKMBC_POCKETCAM;
			ainfo->battery = 0; // FIXME: Unknown
		case 0xFD: // BANDAI TAMA5
			ainfo->mbc_id = PAKMBC_TAMA5;
			ainfo->battery = 0; // FIXME: Unknown
		case 0xFE: // HuC3
			ainfo->mbc_id = PAKMBC_HuC3;
			ainfo->battery = 0; // FIXME: Unknown
			break;
		case 0xFF: // HuC1+RAM+BATTERY
			ainfo->mbc_id = PAKMBC_HuC1;
			ainfo->battery = 1;
			break;
		default: // Unknown pak type
			LOGF("Unrecognized pak_type_code=%u", ainfo->pak_type_code);
			LOGT("return 1");
			return 1; // failure
	} // end switch (ainfo->pak_type_code)
	
	LOGT("return 0");
	return 0; // success
} // end decode_pak_type()

//=======================================================================
// def decode_ram_size()
static inline int
decode_ram_size(struct pakhdr_alloc_info* restrict ainfo) {
	LOGT("call(ainfo=%p)", ainfo);
	assert(ainfo != NULL);
	// TODO: MBC2 should report ram_bank_count as 0,
	//       but contain 512 half-bytes of RAM.
	if (ainfo->mbc_id == PAKMBC_MBC2) {
		LOGF("Pak reports using MBC2, which this emulator does not currently support.");
		return 1; // failure
	}

	switch (ainfo->ram_size_code) {
		case 0x00:
		case 0x01: ainfo->ram_bank_count = 0; break;
		case 0x02: ainfo->ram_bank_count = 1; break;
		case 0x03: ainfo->ram_bank_count = 4; break;
		case 0x04: ainfo->ram_bank_count = 16; break;
		case 0x05: ainfo->ram_bank_count = 8; break;
		default:
			LOGF("Unrecognized ram_size_code=%u", ainfo->ram_size_code);
			LOGT("return 1");
			return 1; // unrecognized ram size code
	} // end switch (ram_size_code)
	
	ainfo->ram_size = (size_t)(ainfo->ram_bank_count) * PAK_RAM_BANK_SIZE;
	LOGT("return 0");
	return 0; // success
} // end decode_ram_size()

//=======================================================================
// def decode_rom_size()
static inline int
decode_rom_size(struct pakhdr_alloc_info* restrict ainfo) {
	LOGT("call(ainfo=%p)", ainfo);
	assert(ainfo != NULL);
	// For values less than or equal to 8:
	// bank_count = 2^(rom_size_code + 1)
	// Where `^` is exponentiation, not an XOR.
	if (ainfo->rom_size_code <= 8) {
		ainfo->rom_bank_count = 2 << ainfo->rom_size_code;
		ainfo->rom_size = (size_t)(ainfo->rom_bank_count) * PAK_ROM_BANK_SIZE;
		LOGD("ainfo->rom_bank_count=%u, ainfo->rom_size=%u",
				ainfo->rom_bank_count, ainfo->rom_size);
		LOGT("return 0");
		return 0; // success
	} else {
		LOGF("Unrecognized rom_size_code=%u", ainfo->rom_size_code);
		LOGT("return 1");
		return 1; // unknown rom_size_code
	}
} // end decode_rom_bank_count()

//=======================================================================
// def dump_logo_check
static void
dump_logo_check(struct incbuf* restrict ibuf, const void* restrict rom) {
	static const uint8_t logo[] = {
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
		0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
		0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
		0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
	}; // end logo[]
	static_assert(PAKHDR_E_LOGO - PAKHDR_LOGO == sizeof(logo));

	uint8_t mismatches = 0;
	for (size_t hdrpos = PAKHDR_LOGO; hdrpos < PAKHDR_E_LOGO; ++hdrpos) {
		if (((uint8_t*)rom)[hdrpos] != logo[hdrpos - PAKHDR_LOGO])
			++mismatches;
	}
	incbuf_printf(ibuf, "\tHeader: %s (%u mismatches)\n",
			mismatches == 0 ? "OK" : "BAD", mismatches);
} // end dump_logo_check()

//=======================================================================
// def dump_title()
static void
dump_title(struct incbuf* restrict ibuf, const void* restrict rom) {
	// Title is up to 16 characters, without guaranteed NUL-termination.
	// It must be copied into a separate, NUL-terminated buffer for printing.
	uint8_t title_buf[PAKHDR_TITLE_SZv0];
	memcpy(title_buf, ((uint8_t*)rom) + PAKHDR_TITLE, PAKHDR_TITLE_SZv0);
	// Backup CGB flag, NUL-terminate 4-byte manufacturer code:
	uint8_t cgb_flag = title_buf[PAKHDR_CGB_FLAG - PAKHDR_TITLE];
	title_buf[PAKHDR_CGB_FLAG - PAKHDR_TITLE] = 0; // NUL
	size_t title_len = strlen(title_buf);
	if (title_len <= PAKHDR_TITLE_SZv2) {
		// Actual title is 10-or-fewer characters long (excluding NUL)
		// Bytes 11-13 MIGHT be a manufacturer code.
		// Byte 14 MIGHT indicate CGB compatibility.
		incbuf_printf(ibuf,
				"\tTitle: \"%s\"\n\tManufacturer code: \"%s\"\n\tCGB flag: %s (0x%02X)\n",
				title_buf, title_buf + (PAKHDR_MANUCODE - PAKHDR_TITLE),
				cgb_flag_string(cgb_flag), cgb_flag);
	} else {
		// Title is 16-or-fewer characters long (excluding optional NUL).
		// Title may be 11 characters long exactly, but the lack of
		// NUL-termination makes it inscrutable from the manufacturer code.
		incbuf_printf(ibuf, "\tTitle/Manufacturer code: \"%s", title_buf);
		if (cgb_flag < 0x20 || cgb_flag >= 0x7F) {
			// Not printable, so can't be part of title.
			incbuf_printf(ibuf, "\"\n\tCGB flag: %s\n", cgb_flag_string(cgb_flag));
		} else {
			// Printable, MIGHT be part of the title.
			incbuf_printf(ibuf, "%c\"\n\tCGB flag?: compatible (0x%02X)\n",
					cgb_flag, cgb_flag);
		}
	} // end ifelse (title is v2)
} // end dump_title()

//=======================================================================
// def fread_alloc_info()
static int
fread_alloc_info(
		struct pakhdr_alloc_info* restrict dst,
		FILE* restrict rom_file) {
	assert(dst != NULL);
	assert(rom_file != NULL);

	if (fseek(rom_file, PAKHDR_PAK_TYPE, SEEK_SET)) {
		LOGF("Unable to seek to address 0x%X in rom_file.", PAKHDR_PAK_TYPE);
		return 1; // Seek failure
	}
	
	//--- Pak type ---
	int byte = fgetc(rom_file);
	if (byte == EOF) {
		LOGF("Failed to read pak_type_code byte.");
		return 2; // Read failure
	}
	dst->pak_type_code = (uint8_t)byte;
	//--- ROM size ---
	byte = fgetc(rom_file);
	if (byte == EOF) {
		LOGF("Failed to read rom_size_code byte.");
		return 2; // Read failure
	}
	dst->rom_size_code = (uint8_t)byte;
	//--- RAM size ---
	byte = fgetc(rom_file);
	if (byte == EOF) {
		LOGF("Failed to read ram_size_code byte.");
		return 2; // Read failure
	}
	dst->ram_size_code = (uint8_t)byte;

	return 0; // Success
} // end fread_alloc_info()

//=======================================================================
// def new_licensee_string()
static const char*
new_licensee_string(uint8_t new_licensee_code[2]) {
	return NULL; // TODO: check pandocs for sample list
} // end new_licensee_string()

