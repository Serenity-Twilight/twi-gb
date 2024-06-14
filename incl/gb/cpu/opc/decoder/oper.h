#ifndef GB_CPU_OPC_DECODER_OPER_H
#define GB_CPU_OPC_DECODER_OPER_H
//=======================================================================
// doc enum gb_opc_oper_id
// TODO
//
// --- Instructions which implicitly use register A
// RLCA
// RRCA
// RLA
// RRA
// --- Instructions which implicitly use the carry flag
// ADC
// SBC
// RL(A)
// RR(A)
// SCF
// CCF
// --- Instructions which implicitly use [SP]
// PUSH
// POP
// CALL
// RET
// RETI

// def enum gb_opc_oper_id
enum gb_opc_oper_id {
	OPER_INVALID = -1,

	// CPU control instructions (and NOP)
	OPER_CTL_START,
	OPER_NOP = OPER_CTL_START,
	OPER_STOP,
	OPER_HALT,
	OPER_DI,
	OPER_EI,
	OPER_CTL_END,
	// Branch instructions
	OPER_BRANCH_START = OPER_CTL_END,
	OPER_JP = OPER_BRANCH_START,
	OPER_JR,
	OPER_CALL,
	OPER_RST,
	OPER_RET,
	OPER_RETI,
	OPER_BRANCH_END,
	// Unprefixed 8-bit instructions
	OPER_NOPFX_START = OPER_BRANCH_END,
	OPER_LD8 = OPER_NOPFX_START,
	OPER_ADD8,
	OPER_ADC,
	OPER_SUB,
	OPER_SBC,
	OPER_AND,
	OPER_XOR,
	OPER_OR,
	OPER_CP,
	OPER_INC8,
	OPER_DEC8,
	OPER_RLCA,
	OPER_RRCA,
	OPER_RLA,
	OPER_RRA,
	OPER_DAA,
	OPER_CPL,
	OPER_SCF,
	OPER_CCF,
	OPER_NOPFX_END,
	// 16-bit instructions
	OPER_16BIT_START = OPER_NOPFX_END,
	OPER_LD16 = OPER_16BIT_START,
	OPER_PUSH,
	OPER_POP,
	OPER_ADD16,
	OPER_INC16,
	OPER_DEC16,
	OPER_16BIT_END,
	// CB-prefixed instructions
	OPER_CBPFX_START = OPER_16BIT_END,
	OPER_RLC = OPER_CBPFX_START,
	OPER_RRC,
	OPER_RL,
	OPER_RR,
	OPER_SLA,
	OPER_SRA,
	OPER_SWAP,
	OPER_SRL,
	OPER_BIT,
	OPER_RES,
	OPER_SET,
	OPER_CBPFX_END,

	NUM_OPERS = OPER_CBPFX_END
}; // end enum gb_opc_oper_id

#endif // GB_CPU_OPC_DECODER_OPER_H

