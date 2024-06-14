#include <stdint.h>
#include <stdio.h>

typedef uint8_t (*change_detector)(uint8_t old, uint8_t new);

static uint8_t
detect_low_to_high_fast(uint8_t old, uint8_t new) {
	// Bits in `old` that are high in `new` invert.
	// The `~old` mask then removes bits that were already
	// high in `old`.
	return (old ^ new) & ~old;
}

static uint8_t
detect_low_to_high_easy(uint8_t old, uint8_t new) {
	uint8_t change = 0;
	for (uint8_t b = 1; b < 0x10; b <<= 1) {
		if (!(old & b) && (new & b))
			change |= b;
	}
	return change;
}

static uint8_t
detect_high_to_low_fast(uint8_t old, uint8_t new) {
	// If...
	// old bit=0, new bit=0: ^ = 0 & new=1 = 0
	// old bit=0, new bit=1: ^ = 1 & new=0 = 0
	// old bit=1, new bit=0: ^ = 1 & new=1 = 1
	// old bit=1, new bit=1: ^ = 0 & new=0 = 0
	return (old ^ new) & ~new;
}

static uint8_t
detect_high_to_low_easy(uint8_t old, uint8_t new) {
	uint8_t change = 0;
	for (uint8_t b = 1; b < 0x10; b <<= 1) {
		if ((old & b) && !(new & b))
			change |= b;
	}
	return change;
}

static unsigned int
test_change_detectors(change_detector easy_func, change_detector fast_func) {
	unsigned int mismatches = 0;
	for (uint8_t old = 0; old < 16; ++old) {
		for (uint8_t new = 0; new < 16; ++new) {
			uint8_t easy = easy_func(old, new);
			uint8_t fast = fast_func(old, new);
			if (easy != fast) {
				printf("old=%X, new=%X, fast=%X, easy=%X\n",
						old, new, fast, easy);
				++mismatches;
			}
		}
	}
	return mismatches;
} // end test_change_detectors()

int main() {
	unsigned int mismatches =
		test_change_detectors(detect_low_to_high_easy, detect_low_to_high_fast);
	printf("%d low-to-high mismatches\n", mismatches);
	mismatches =
		test_change_detectors(detect_high_to_low_easy, detect_high_to_low_fast);
	printf("%d high-to-low mismatches\n", mismatches);
	return 0;
} // end main()

