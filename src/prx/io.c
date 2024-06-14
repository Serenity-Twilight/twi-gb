#include <assert.h>
#include <stdio.h>
#include <string.h>

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static size_t
fload_internal(
		char* restrict dst,
		size_t dstsz,
		FILE* restrict file);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def prx_io_dirents_icontains()
size_t
prx_io_dirents_icontains() {
} // end prx_io_dirents_icontains()

//=======================================================================
// def prx_io_fload()
size_t
prx_io_fload(
		char* restrict dst,
		size_t dstsz,
		FILE* restrict file) {
	assert(file != NULL);

	// Load data from provided file:
	// 1. Save current file pointer position
	// 2. Copy contents
	// 3. Restore original file pointer position
	// 4. Return
	long original_fpos = ftell(file);
	size_t size = fload_internal(dst, dstsz, file);
	if (fseek(file, original_fpos, SEEK_SET))
		return (size_t)-1; // Failed to restore original position.
	return size;
} // end prx_io_fload()

//=======================================================================
// def prx_io_fpload()
size_t
prx_io_fpload(
		char* restrict dst,
		size_t dstsz,
		const char* restrict fp) {
	assert(fp != NULL);

	// Load data from file at provided filepath:
	// 1. Open file
	// 2. Copy contents
	// 3. Close file
	// 4. Return
	FILE* file = fopen(fp, "rb");
	if (file == NULL)
		return (size_t)-1; // Unable to open `fp`
	size_t size = fload_internal(dst, dstsz, file);
	fclose(file);
	return size;
} // end prx_io_fpload()

//=======================================================================
// def prx_io_fp_has_extension()
//int
//prx_io_fp_has_extension(const char* restrict fp) {
//	assert(fp != NULL);
//	
//	// Check if the final element of the filepath has a period.
//	// * If the period is the final character of the element, then it
//	//   doesn't count as the start of an extension.
//	// * If the period is the first character of the element, then its
//	//   purpose is to hide the file, and it doesn't count as the start
//	//   of an extension.
//	size_t pos = strlen(fp);
//	if (pos <= 2)
//		return 0; // Path is too short to have a valid extension.
//	pos -= 2; // Skip final character
//	do {
//
//	} while (pos > 0);
//	return 0; // String did not contain '.' or '/'
//} // end prx_io_fp_has_extension()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

static size_t
fload_internal(
		char* restrict dst,
		size_t dstsz,
		FILE* restrict file) {
	// Determine size of file.
	if (fseek(file, 0, SEEK_END))
		return (size_t)-1; // Unable to seek to end-of-file
	size_t filesize = (size_t)ftell(file);
	if (dst == NULL || dstsz == 0) // No destination buffer. Report file size.
		return filesize;

	// Read up to `dstsz` or `filesize` (whichever is smaller)
	// from `file` into `dst`.
	rewind(file);
	size_t readsize = (dstsz < filesize ? dstsz : filesize);
	if (fread(dst, 1, readsize, file) != readsize)
		return (size_t)-1; // fread failure
	// Report real file size (even if it's bigger than dstsz).
	return filesize;
} // end fload_internal()

