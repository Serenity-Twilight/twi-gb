#define PXM_TRUNCATE_PREFIX
#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GB_LOG_MAX_LEVEL LVL_TRC
#include "gb/log.h"
#include "gb/pak.h"
#include "gb/pak/header.h"
#include "gb/pak/typedef.h"
#define PRX_TRUNCATE_PREFIX 1
#include "prx/io.h"
#undef PRX_TRUNCATE_PREFIX

//=======================================================================
//-----------------------------------------------------------------------
// Loading a ROM+RAM
// a. ROM+RAM identified by a "pak identifier": A NUL-terminated string
//    matching the file names (w/o extension) of both the ROM and RAM
//    files.
//    So, if "some game" is a valid pak identifier, then there will exist
//    the file "./roms/some game.gb", and its RAM, if any, will be
//    "./saves/some game.sav".
//    * If the pak identifier contains an extension, then the ROM file
//      should match it exactly, and the extension then should be replaced
//      with ".sav" to search for RAM. If the pak identifier is without
//      extension, then append ".gb" to it to identify a ROM file.
//    * If the pak identifier contains no forward slashes, then prefix
//      it with "./roms/" to create a filepath. If the pak identifier
//      contains one or more forward slashes, then treat it as a path,
//      thus ignoring the "roms" folder. Saves should still always be
//      within "./saves/". Later, this should be configurable (i.e.
//      allow saves to be stored alongside ROMs)
//
// gb_pak creation steps:
// 1. Convert pak identifier to ROM and RAM filepaths, as described above.
// 2. Open the ROM file.
//   a. Measure it's true size (according to the file system).
//   b. Check if ROM's reported MBC uses battery-backed RAM.
//   c. Check ROM's reported size (in header).
//      If this does not match the true size within the file system,
//      emit a warning, then continue using true size.
//   d. Check RAM's reported size (in header)
// 3. If ROM uses battery-backed RAM, check for the existence of a save
//    file using utilizing the same pak identifier.
//   a. If found, open the file.
//      Measure the save file's true size (according to the file system).
//      If this does not match the size reported by ROM, emit a warning.
//      Proceed to use whichever size is bigger.
// 4. Allocate bytes for a gb_pak object + ROM array + RAM array.
//    While running, ROM and RAM will be contained entirely in memory.
//   a. If RAM is battery-backed but no file exists, use RAM size reported
//      by ROM. If RAM is battery-backed and a save file exists, use whichever
//      is larger between file size and RAM size reported by ROM header.
// 5. Load ROM file into memory, then close ROM file.
// 6. If save file exists, load it into memory, then close save file.
// 7. Perform further initialization of gb_pak object based on ROM header
//    data as necessary.
//
// NOTE: What if ROM/RAM does not use bank switching?
//       In that case, the file should be loaded directly to a memory
//       map instead of any backing array.
//       This would require gb_mem to be present at initialization,
//       which would complicate the design. Maybe it's best to just eat
//       the <=40KiB memory waste for the sake of simplicity?
//       This would also simplify saving, as it can be done without
//       gb_mem, even if the RAM is 8KiB
//-----------------------------------------------------------------------
// TODO:
// * Replace instances of magic numbers to indicate errors with
//   enums with accompanying error strings.
//=======================================================================

//enum {
//	ROM_BANK_SIZE = 16384, // bytes
//	RAM_BANK_SIZE = 8192, // bytes
//};

//struct pakfile_params {
//	size_t directory_count;
//	size_t extension_count;
//	char* id;
//	char** directories;
//	char** extensions;
//}; // end struct pakfile_params
//
//struct pakfile {
//	size_t filesize;
//	FILE* file;
//	size_t pathsize;
//	char path[];
//};

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static struct gb_pak*
fpcreate_pak(
		const char* restrict rom_fp,
		const char* restrict ram_fp);
//-------------------------------------
static struct gb_pak*
alloc_pak(
		const struct pakhdr_alloc_info* restrict ainfo,
		const char* save_filepath);
//-------------------------------------
static int
init_pak(
		struct gb_pak* restrict pak,
		const struct pakhdr_alloc_info* restrict ainfo,
		FILE* restrict rom_file,
		const char* restrict save_filepath);
//-------------------------------------
static inline size_t
pad_size_for_alignment(size_t unpadded_size);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
// def gb_pak_create()
struct gb_pak*
gb_pak_create(const char* restrict pak_id) {
	assert(pak_id != NULL);

	// TODO: For initial tests, assume pak_id is the ROM filepath.
	//       For saves, just append .sav
	size_t pak_id_len = strlen(pak_id);
	size_t savepath_len = pak_id_len + 5; // ".sav" + NUL = 5
	char* save_fp = malloc(savepath_len); // ".sav" + NUL = 5
	if (save_fp == NULL) {
		LOGF("Unable to allocate %zu bytes for save filepath.", savepath_len);
		return NULL;
	}
	strcpy(save_fp, pak_id);
	strcpy(save_fp + pak_id_len, ".sav");

	struct gb_pak* pak = fpcreate_pak(pak_id, save_fp);
	free(save_fp);
	return pak;
} // end gb_pak_create()

//=======================================================================
// def gb_pak_delete()
void
gb_pak_delete(struct gb_pak* restrict pak) {
	if (pak == NULL)
		return;

	if (pak->battery)
		LOGW("Saving on pak object deletion not-yet-implemented.");
	free(pak);
} // end gb_pak_delete()

//=======================================================================
// def gb_pak_insert()
void
gb_pak_insert(
		struct gb_pak* restrict pak,
		uint8_t* restrict rom_map,
		uint8_t* restrict ram_map) {
	assert(pak != NULL);
	assert(rom_map != NULL);
	assert(ram_map != NULL);


} // end gb_pak_insert()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
//static int
//fpalloc_pak(const char* restrict pak_id) {
//	assert(pak_id != NULL);
//	FILE* rom = open_romfile(pak_id);
//	if (rom == NULL)
//		return 1;
//	
//	size_t romsz = io_fload(NULL, 0, rom);
//	if (romsz == (size_t)-1) {
//		LOGF("Unable to measure size of ROM by id='%s'", pak_id);
//		return 1;
//	}
//
//	size_t ramsz = get_ramsz
//} // end allocate_pak()

//=======================================================================
//=======================================================================
//static struct pakfile*
//open_pakfile(const char* restrict pak_id, const char* fp_suffix) {
//	assert(pak_id != NULL);
//	size_t pak_id_len = strlen(pak_id);
//	assert(pak_id_len > 0);
//	assert(fp_suffix != NULL);
//	size_t fp_suffix_size = strlen(fp_suffix) + 1;
//
//	// Compose filepath from ID and suffix:
//	char pakfp[pak_id_len + fp_suffix_size];
//	strncpy(pakfp, pak_id, pak_id_len);
//	strncpy(pakfp + pak_id_len, fp_suffix, fp_suffix_size);
//	FILE* rom = fopen(pakfp, "rb");
//	if (rom == NULL) {
//		LOGF("Unable to access ROM using id='%s' with suffix='%s'",
//				pak_id, fp_suffix);
//		return NULL;
//	}
//} // end open_pakfile()

//=======================================================================
//=======================================================================
//static inline struct pakfile*
//open_romfile(const char* restrict pak_id) {
//	static const char* directories[] = { "./roms/" };
//	static const char* extensions[] = { "gb" };
//	struct pakfile_params params = {
//		.directory_count = 1,
//		.extension_count = 1,
//		.id = pak_id,
//		.directories = directories,
//		.extensions = extensions
//	};
//	return open_pakfile(&params);
//} // end open_romfile()

//=======================================================================
//=======================================================================
//static inline FILE*
//open_ramfile(const char* restrict pak_id) {
//	static const char* directories[] = { "./saves/" };
//	static const char* extensions[] = { "sav" };
//	struct pakfile_params params = {
//		.directory_count = 1,
//		.extension_count = 1,
//		.id = pak_id,
//		.directories = directories,
//		.extensions = extensions
//	};
//	return open_pakfile(&params);
//} // end open_ramfile()

//=======================================================================
//=======================================================================
static struct gb_pak*
fpcreate_pak(
		const char* restrict rom_fp,
		const char* restrict ram_fp) {
	assert(rom_fp != NULL);

	FILE* rom_file = fopen(rom_fp, "rb");
	// TODO: Need specific error-reporting mechanism.
	//       Wrapper function around fopen, probably.
	if (rom_file == NULL) {
		LOGF("Unable to open file: %s", rom_fp);
		return NULL;
	}

	struct pakhdr_alloc_info ainfo;
	if (pakhdr_get_alloc_info(&ainfo, rom_file))
		goto fail_close_rom_file;
	struct gb_pak* pak = alloc_pak(&ainfo, ram_fp);
	if (pak == NULL)
		goto fail_close_rom_file;
	if (init_pak(pak, &ainfo, rom_file, ram_fp))
		goto fail_free_pak;

	fclose(rom_file);
	return pak; // success
fail_free_pak:
	free(pak);
fail_close_rom_file:
	fclose(rom_file);
	return NULL;
} // end fpcreate_pak()

//=======================================================================
//=======================================================================
static struct gb_pak*
alloc_pak(
		const struct pakhdr_alloc_info* restrict ainfo,
		const char* save_filepath) {
	assert(ainfo != NULL);

	// Total allocation size is the sum of:
	// 1. The members of `struct gb_pak`
	// 2. A copy of the save filepath, including terminating NUL
	// 3. A copy of the pak's ROM image
	// 4. A copy of the pak's RAM region
	//
	// Each item is placed after the prior item, meaning that all
	// pak memory is stored in one contiguous memory region.
	size_t alloc_size = sizeof(struct gb_pak);
	if (ainfo->battery) {
		assert(save_filepath != NULL);
		alloc_size += strlen(save_filepath) + 1;
	}
	//--- Allocate buffer for ROM ---
	// 1. Pad end of pak to align rom as max_align_t.
	//    The intent is to (hopefully) speed up copies from the ROM region.
	alloc_size = pad_size_for_alignment(alloc_size);
	// 2. Save offset and reserve space
	size_t rom_offset = alloc_size;
	alloc_size += ainfo->rom_size;
	//--- Allocate buffer for RAM ---
	// Same as above
	size_t ram_offset;
	if (ainfo->ram_size != 0) {
		alloc_size = pad_size_for_alignment(alloc_size);
		ram_offset = alloc_size;
		alloc_size += ainfo->ram_size;
	}

	struct gb_pak* pak = malloc(alloc_size);
	if (pak == NULL) {
		LOGF("Unable to allocate %zu bytes.", alloc_size);
		return NULL; // allocation failure
	}

	// Initialize internal pointers:
	pak->save_filepath = (ainfo->battery ? (char*)pak + sizeof(struct gb_pak) : NULL);
	pak->rom = (char*)pak + rom_offset;
	pak->ram = (ainfo->ram_size != 0 ? (char*)pak + ram_offset : NULL);

	return pak; // success
} // end alloc_pak()

//=======================================================================
//=======================================================================
static int
init_pak(
		struct gb_pak* restrict pak,
		const struct pakhdr_alloc_info* restrict ainfo,
		FILE* restrict rom_file,
		const char* restrict save_filepath) {
	assert(pak != NULL);
	assert(ainfo != NULL);
	assert(rom_file != NULL);

	//--------------------------------------
	//--- Copy ROM from file into memory ---
	size_t filesize = io_fload(pak->rom, ainfo->rom_size, rom_file);
	if (filesize == (size_t)-1) {
		LOGF("Failed to copy ROM from file into memory.");
		return 1; // Failed to copy ROM.
	}
	LOGI("ROM: expected filesize=%zu, actual filesize=%zu", ainfo->rom_size, filesize);
	if (filesize != ainfo->rom_size) {
		LOGW("ROM filesize=%zu does not match expected size=%zu!", filesize, ainfo->rom_size);
		if (filesize > ainfo->rom_size)
			LOGW("ROM data beyond reported size won't be loaded.");
		else {
			LOGW("ROM bytes beyond actual filesize will be padded with 0xFF.");
			memset(pak->rom + filesize, 0xFF, ainfo->rom_size - filesize);
		}
	}

	//----------------------------------------------------
	//--- Copy RAM from file into memory, if it exists ---
	if (ainfo->battery) {
		assert(save_filepath != NULL);
		assert(pak->save_filepath != NULL);
		LOGI("Pak supports battery-backed saves. Attempting to load save file...");
		// Might be good to check for existence of save file.
		// Save file not existing is normal for first runs of a game,
		// and thus is not a fatal error, but there are many other
		// fatal errors that could occur unrelated to file existence.
		filesize = io_fpload(pak->ram, ainfo->ram_size, save_filepath);
		if (filesize == (size_t)-1) {
			LOGI("SAV: No existing savefile identified by '%s'", save_filepath);
		} else {
			LOGI("SAV: Expected filesize=%zu, actual filesize=%zu", ainfo->ram_size, filesize);
			if (filesize != ainfo->ram_size) {
				LOGW("SAV: Filesize=%zu does not match expected size=%zu!", filesize, ainfo->ram_size);
				if (filesize > ainfo->rom_size)
					LOGW("SAV: Data beyond reported size won't be loaded.");
				else
					LOGW("SAV: Region bytes beyond actual filesize won't be initialized.");
			} // end if (filesize != ainfo->ram_size)
		} // end ifelse (unable to load file)

		// Copy save_filepath into the pak, for later saves.
		strcpy(pak->save_filepath, save_filepath);
	} else {
		LOGI("Pak does NOT support battery-backed saves.");
	}// end ifelse (pak contains battery)
	
	//-------------------------------------------
	//--- Initialize remaining gb_pak members ---
	// Pak information fields:
	pak->rom_bank_count = ainfo->rom_bank_count;
	pak->ram_bank_count = ainfo->ram_bank_count;
	pak->mbc_id = ainfo->mbc_id;
	pak->battery = ainfo->battery;
	// Pak state fields:
	pak->rom_bank_curr = pak->ram_bank_curr = 0;
	pak->dirty_ram = 0;

	return 0; // success
} // end init_pak()

//static inline int
//decode_alloc_info(
//		struct decoded_alloc_info* restrict dinfo,
//		const struct encoded_alloc_info* restrict einfo) {
//	assert(dinfo != NULL);
//	assert(einfo != NULL);
//	if (decode_pak_type(&(dinfo->feat), einfo->pak_type_code))
//		return 1; // Unrecognized pak type code
//	dinfo->rom_bank_count = decode_rom_bank_count(einfo->rom_size_code);
//	if (dinfo->rom_bank_count == (uint16_t)-1)
//		return 2; // Unrecognized rom size code
//	dinfo->ram_bank_count = decode_ram_bank_count(einfo->ram_size_code);
//	if (dinfo->ram_bank_count == (uint8_t)-1)
//		return 3; // Unrecognized ram size code
//	return 0; // Success
//} // end decode_alloc_info()

//static inline int
//load_header_byte(FILE* restrict romfile, enum header_addr address) {
//	if (fseek(romfile, address, SEEK_SET))
//		return -1;
//	int byte = fgetc(romfile);
//	if (byte == EOF)
//		return -2;
//	return byte;
//} // end load_header_byte()

//=======================================================================
// Pads the end of the provided size so that whatever data follows it
// has a minimum alignment of `alignof(max_align_t)`.
//=======================================================================
static inline size_t
pad_size_for_alignment(size_t unpadded_size) {
	size_t padding = alignof(max_align_t) - (unpadded_size % alignof(max_align_t));
	if (padding != alignof(max_align_t))
		return unpadded_size + padding;
	else
		return unpadded_size; // already aligned, no padding necessary
} // end pad_size_for_alignment()

//=======================================================================
//=======================================================================
//static inline struct pak_region_size
//sizeof_extmem(const struct decoded_alloc_info* restrict ainfo) {
//	assert(ainfo != NULL);
//	struct pak_region_size size = {
//		.rom = ainfo->rom_bank_count * ROM_BANK_SIZE,
//		.ram = ainfo->ram_bank_count * RAM_BANK_SIZE };
//	return size;
//} // end size_extmem()

