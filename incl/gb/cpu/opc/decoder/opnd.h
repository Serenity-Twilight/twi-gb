#ifndef GB_CPU_OPC_DECODER_OPND_H
#define GB_CPU_OPC_DECODER_OPND_H

//=======================================================================
// doc enum gb_opc_opnd_id
// Unique identifiers for instruction operands with meaningfully-
// encoded bit values, described in the following graphic:
//
// Note: 0 is reserved for OPND_NONE
// -----------------[7]: 0 = register/flag, 1 = immediate
// | ---------------[6]: 0 = literal, 1 = pointer
// | | -------------[5]: 0 = 8-bit, 1 = 16-bit
// | | |
// | | | -----------[4]: if ([7] == 0 (register/flag)
// | | | |               	if ([5] == 0 (8-bit))
// | | | |               		Always 1
// | | | |               	else ([5] == 1 (16-bit))
// | | | |               		0 = read-only, 1 = increment/decrement after read
// | | | |               else ([7] == 1 (immediate))
// | | | |               	0 = within opcode, 1 = after opcode
// | | | |
// | | | | |--------[3]: if ([7] == 0 (register/flag))
// | | | | |             	if ([5] == 0 (8-bit))
// | | | | |             		0 = 8-bit register, 1 = flag
// | | | | |             	else ([5] == 1 (16-bit))
// | | | | |             		if ([4] == 0 (read-only))
// | | | | |             			unused
// | | | | |             		else ([4] == 1 (increment/decrement after read))
// | | | | |             			0 = decrement, 1 = increment
// | | | | |             else ([7] == 1 (immediate))
// | | | | |             	if ([4] == 0 (within opcode))
// | | | | |             		0 = bit, 1 = RST destination address
// | | | | |             	else ([4] == 1 (after opcode))
// | | | | |             		unused
// | | | | |
// | | | | | -------[2-0]: if ([7] == 0 (register/flag))
// | | | | | | | |         	if ([5] == 0 (8-bit))
// | | | | | | | |         		if ([3] == 0 (8-bit register))
// | | | | | | | |         			0 = B, 1 = C, 2 = D, 3 = E, 4 = H, 5 = L, 7 = A
// | | | | | | | |         		else ([3] == 1 (flag))
// | | | | | | | |         			[2]: 0 = no invert, 1 = invert (NOT flag)
// | | | | | | | |         			[1-0]: 0 = Carry, 1 = Half-carry, 2 = Negative, 3 = Zero
// | | | | | | | |         	else ([5] == 1 (16-bit))
// | | | | | | | |         		[2-0]: 0 = BC, 1 = DE, 2 = HL, 3 = AF, 4 = SP, 5-7 = unused
// | | | | | | | |         else ([7] == 1 (immediate))
// | | | | | | | |         	if ([4] == 0 (within opcode))
// | | | | | | | |         		if ([3] == 0 (bit))
// | | | | | | | |         			bit = [2-0]
// | | | | | | | |         		else ([3] == 1 (RST desintation address offset))
// | | | | | | | |         			address = [2-0] * 8
// | | | | | | | |         	else ([4] == 1 (after opcode))
// | | | | | | | |         		[2]: 0 = unsigned, 1 = signed
// | | | | | | | |         		[1]: 0 = immediate only, 1 = add SP to immediate
// | | | | | | | |         		[0]: unused
// b b b b b b b b 

// def enum gb_opc_opnd_id
enum gb_opc_opnd_id {
	OPND_NONE   = 0x00,

	// Constants for operand descriptor bits.
	// [7]
	OPND_IMMED  = 0x80, // 0 = register/flag, 1 = immediate
	// [6]
	OPND_PTR    = 0x40, // 0 = literal, 1 = pointer
	// [5]
	OPND_16BIT  = 0x20, // 0 = 8-bit, 1 = 16-bit
	// [4] (NOTE: Modify only applies to 16-bit registers)
	OPND_8BIT   = 0x10, // Always set if [7] and [5] are not
	OPND_MODIFY = 0x10, // 0 = read-only, 1 = increment/decrement after read
	OPND_AFTER  = 0x10, // 0 = immediate within opcode, 1 = immediate after opcode
	// [3]
	OPND_FLAG   = 0x08, // 0 = 8-bit register, 1 = flag
	OPND_INCR   = 0x08, // 0 = decrement, 1 = increment
	OPND_IS_RST = 0x08, // 0 = encoded value is bit, 1 = encoded value is RST address
	// [2]
	OPND_INVERT = 0x04, // 0 = flag not inverted, 1 = flag inverted
	OPND_SIGNED = 0x04, // 0 = unsigned, 1 = signed
	// [1]
	OPND_ADD_SP = 0x02, 

	// 8-bit registers
	OPND_rB = OPND_8BIT,
	OPND_rC,
	OPND_rD,
	OPND_rE,
	OPND_rH,
	OPND_rL,
	OPND_rA,
	OPND_rF,
	// 16-bit register operands
	OPND_rBC = OPND_16BIT,
	OPND_rDE,
	OPND_rHL,
	OPND_rAF,
	OPND_rSP,
	// Bit offsets
	OPND_b0 = OPND_IMMED,
	OPND_b1,
	OPND_b2,
	OPND_b3,
	OPND_b4,
	OPND_b5,
	OPND_b6,
	OPND_b7,
	// RST addresses
	OPND_RST00 = OPND_IMMED | OPND_IS_RST,
	OPND_RST08,
	OPND_RST10,
	OPND_RST18,
	OPND_RST20,
	OPND_RST28,
	OPND_RST30,
	OPND_RST38,
	// Flags
	OPND_fC = OPND_8BIT | OPND_FLAG,  // if carry flag set
	OPND_fNC = OPND_fC | OPND_INVERT, // if carry flag clear
	OPND_fZ = OPND_fC | 3,            // if zero flag set
	OPND_fNZ = OPND_fZ | OPND_INVERT, // if zero flag clear
	// 8-bit register pointer operands
	OPND_mC = OPND_rC | OPND_PTR,
	// 16-bit register pointer operands
	OPND_mBC = OPND_rBC | OPND_PTR,
	OPND_mDE,
	OPND_mHL,
	OPND_mHLd = OPND_mHL | OPND_MODIFY, // Decrement HL after taking pointer value
	OPND_mHLi = OPND_mHLd | OPND_INCR,  // Increment HL after taking pointer value
	// External immediate operands
	OPND_ui8 = OPND_IMMED | OPND_AFTER,   // unsigned 8-bit literal
	OPND_si8 = OPND_ui8 | OPND_SIGNED,    // signed 8-bit literal
	OPND_si8rSP = OPND_si8 | OPND_ADD_SP, // signed 8-bit literal combined with SP
	OPND_mui8 = OPND_ui8 | OPND_PTR,      // unsigned 8-bit pointer
	OPND_ui16 = OPND_ui8 | OPND_16BIT,    // unsigned 16-bit literal
	OPND_mui16 = OPND_ui16 | OPND_PTR,    // unsigned 16-bit pointer
}; // end enum gb_opc_opnd_id

#endif // GB_CPU_OPC_DECODER_OPND_H

