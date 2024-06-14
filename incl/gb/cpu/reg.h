#ifndef GB_CPU_REG_H
#define GB_CPU_REG_H

#define CPUOBJ (core->cpu)

enum {
// 8-bit register indices
#if !defined(__BYTE_ORDER__) \
 || __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#	if !defined(__BYTE_ORDER__) \
  || (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ \
	 && __BYTE_ORDER__ != __ORDER_PDP_ENDIAN__)
#		warning "Machine endianness is not known. Assuming little endian."
#		warning "This may result in erroneous program behavior."
#	endif
	// Little/Mixed Endian
	iA = 1, iF = 0,
	iB = 3, iC = 2,
	iD = 5, iE = 4,
	iH = 7, iL = 6,
#else
	// Big Endian
	iA = 0, iF = 1,
	iB = 2, iC = 3,
	iD = 4, iE = 5,
	iH = 6, iL = 7,
#endif // end 8-bit register indices

// 16-bit register pair indices
	iAF = 0,
	iBC = 2,
	iDE = 4,
	iHL = 6
}; // end enum

// 8-bit register macros
#define r8(index) (CPUOBJ.r[index])
#define rA r8(iA)
#define rF r8(iF)
#define rB r8(iB)
#define rC r8(iC)
#define rD r8(iD)
#define rE r8(iE)
#define rH r8(iH)
#define rL r8(iL)

// 16-bit register pair macros
#define r16(index) (*((uint16_t*)(&(CPUOBJ.r[index]))))
#define rAF r16(iAF)
#define rBC r16(iBC)
#define rDE r16(iDE)
#define rHL r16(iHL)
#define rSP (CPU.sp)
#define rPC (CPU.pc)

#endif // GB_CPU_REG_H

