// Circular buffer
#include <assert.h>
#include <limits.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
struct prx_cbuf {
	alignas(max_align_t) struct {
		size_t item_size;
		size_t item_count;
		size_t usage_bytes;
		size_t curr_item;
	} m; // metadata
	uint8_t b[]; // buffer
}; // end struct prx_cbuf

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================
static inline size_t
calc_usage_bytes(size_t item_count);
static inline size_t
cbuf_getsize_internal(
		size_t item_size, size_t item_count, size_t* restrict usage_bytes);
static inline struct prx_cbuf*
cbuf_init_internal(
		struct prx_cbuf* restrict cbuf,
		size_t item_size, size_t item_count, size_t usage_bytes);
static inline void*
get_item_address(struct prx_cbuf* restrict cbuf, size_t item);
static inline size_t
get_item_id(struct prx_cbuf* restrict cbuf, void* addr);
static inline uint8_t
next_open_buffer(struct prx_cbuf* restrict cbuf);
static inline void
release_item(struct prx_cbuf* restrict cbuf, size_t item);
static inline void
reserve_item(struct prx_cbuf* restrict cbuf, size_t item);

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
struct prx_cbuf*
prx_cbuf_create(size_t item_size, size_t item_count) {
	size_t usage_bytes;
	struct prx_cbuf* cbuf =
		malloc(cbuf_getsize_internal(item_size, item_count, &usage_bytes));
	if (cbuf == NULL)
		return NULL;
	cbuf_init_internal(cbuf, item_size, item_count, usage_bytes);
	return cbuf;
} // end prx_cbuf_create()

//=======================================================================
void
prx_cbuf_init(struct prx_cbuf* restrict cbuf, size_t item_size, size_t item_count) {
	size_t usage_bytes = calc_usage_bytes(item_count);
	cbuf_init_internal(cbuf, item_size, item_count, usage_bytes);
} // end prx_cbuf_init()

//=======================================================================
size_t
prx_cbuf_getsize(size_t item_size, size_t item_count) {
	size_t usage_bytes; // Not exposed to external users.
	return cbuf_getsize_internal(item_size, item_count, &usage_bytes);
} // end prx_cbuf_getsize()

//=======================================================================
void*
prx_cbuf_alloc(struct prx_cbuf* restrict cbuf, void* addr) {
	if (addr == NULL) {
		if (next_open_buffer(cbuf))
			return NULL;
		assert(cbuf->b[cbug->m.curr_item] == 0); // Is not already reserved.
		reserve_item(cbuf, cbuf->m.curr_item);
		return get_item_address(cbuf, cbuf->m.curr_item);
	} else {
		size_t item_id = get_item_id(cbuf, addr);
		assert(cbuf->b[item_id] != 0); // Has at least 1 reservation.
		reserve_item(cbuf, item_id);
		return addr;
	}
} // end prx_cbuf_alloc()

//=======================================================================
void
prx_cbuf_free(struct prx_cbuf* restrict cbuf, void* addr) {
	release_item(cbuf, get_item_id(cbuf, addr));
} // end prx_cbuf_free()

//=======================================================================
//-----------------------------------------------------------------------
// INTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================
static inline size_t
calc_usage_bytes(size_t item_count) {
	// Each item has 1 usage byte.

	// Round byte count up to nearest max_align_t size.
	// This ensures that user data is aligned to max_align_t.
	size_t unaligned_bytes = item_count % sizeof(max_align_t);
	if (unaligned_bytes != 0)
		item_count += sizeof(max_align_t) - unaligned_bytes;

	return item_count;
} // end calc_usage_bytes()

//=======================================================================
static inline size_t
cbuf_getsize_internal(
		size_t item_size, size_t item_count, size_t* restrict usage_bytes) {
	assert(usage_bytes != NULL);
	*usage_bytes = calc_usage_bytes(item_count);
	return sizeof(prx_cbuf) + *usage_bytes + item_size * item_count;
} // end cbuf_getsize_internal

//=======================================================================
static inline struct prx_cbuf*
cbuf_init_internal(
		struct prx_cbuf* restrict cbuf,
		size_t item_size, size_t item_count, size_t usage_bytes) {
	cbuf->m.item_size = item_size;
	cbuf->m.item_count = item_count;
	cbuf->m.usage_bytes = usage_bytes;
	cbuf->m.curr_item = 0;

	// Clear all usage bytes in buffer.
	memset(cbuf->b, 0, cbuf->m.usage_bytes);
} // end cbuf_init_internal()

//=======================================================================
static inline void*
get_item_address(struct prx_cbuf* restrict cbuf, size_t item_id) {
	return
		cbuf->b + cbuf->m.usage_bytes + // item buffer base address
		cbuf->m.item_size * item_id; // offset-per-index * index
} // end get_item_address()

//=======================================================================
static inline size_t
get_item_id(struct prx_cbuf* restrict cbuf, void* addr) {
	void* itembuf_addr = addr - (cbuf->b + cbuf->m.usage_bytes);
	assert(itembuf_addr % cbuf->m.item_size == 0);
	return itembuf_addr / cbuf->m.item_size;
} // end get_item_id()

//=======================================================================
static inline uint8_t
next_open_buffer(struct prx_cbuf* restrict cbuf) {
	size_t curr = cbuf->m.curr_item;
	while (cbuf->b[curr]) { // buffer already reserved?
		if (++curr >= cbuf->m.item_count) // goto next buffer
			curr -= cbuf->m.item_count; // wrap
		if (curr == cbuf->m.curr_item)
			return 1; // All buffers already reserved.
	}

	cbuf->m.curr_item = curr;
	return 0; // success
} // end next_open_buffer()

//=======================================================================
static inline void
release_item(struct prx_cbuf* restrict cbuf, size_t item) {
	assert(cbuf->b[item] > 0);
	cbuf->b[item] -= 1;
} // end release_item()

//=======================================================================
static inline void
reserve_item(struct prx_cbuf* restrict cbuf, size_t item) {
	assert(cbuf->b[item] < 255);
	cbuf->b[item] += 1;
} // end reserve_item()

