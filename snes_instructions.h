#pragma once
#include "instructions.h"

enum snes_address_mode{
	UNSPECIFIED,
	DIRECT,
	INDIRECT,
	INDIRECT_Y,
	INDEXED_X,
	INDEXED_Y,
	RELATIVE,
	STACK
	//add more as I find opcode uses
};

struct snes_operand{
	string text;
	asm_operand_type type = UNUSED;
	snes_address_mode mode = UNSPECIFIED;
	string label;
	unsigned int value;
	asm_register_width width = BITS_16; //default 16 bits
	
	bool matches(asm_operand_type op_type, snes_address_mode op_mode, unsigned int op_value) const
	{
		return op_type == type && op_mode == mode && op_value == value;
	}
	
	bool matches(asm_operand_type op_type, unsigned int op_value) const
	{
		return op_type == type && op_value == value;
	}
};

typedef asm_line<snes_operand> snes_line;

snes_line get_directive(const string &directive);
snes_line get_width_directive(asm_register_width width);

snes_line get_instruction(const string &instruction);
template <typename T>
snes_line get_instruction(const string &instruction, asm_operand_type type, T value);

template <typename T>
snes_operand &add_operand(snes_line &line, T value);

template <typename T>
snes_line get_memory_instruction(const string &instruction, snes_address_mode mode, T address);

template <typename T>
snes_line get_pc_instruction(const string &instruction, snes_address_mode mode, T address);
