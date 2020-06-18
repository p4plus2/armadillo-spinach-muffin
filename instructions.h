#pragma once
#include "p4lib/string.h"
#include "p4lib/array.h"

enum asm_line_type{
	LABEL,
	DIRECTIVE,
	INSTRUCTION,
	PLACEHOLDER
};

//todo index constant is new and could probably be retroactively applied in a few areas
enum asm_operand_type{
	CONSTANT,
	INDEX_CONSTANT,
	REGISTER,
	MEMORY,
	UNUSED
};

enum asm_register_width{
	BITS_NONE = 0,
	BITS_32 = 4,
	BITS_24 = 3,
	BITS_16 = 2,
	BITS_8 = 1,
	BITS_8_HIGH = 5,
	BITS_8_LOW = 6
};

template <typename T>
struct asm_line{
	string line_data;
	asm_line_type type;
	array<T> operands;
	
	string &mnemonic()
	{
		return line_data;
	}
	
	string &label()
	{
		return line_data;
	}
	
	string &directive()
	{
		return line_data;
	}
	
	const string &mnemonic() const
	{
		return line_data;
	}
	
	const string &label() const
	{
		return line_data;
	}
	
	const string &directive() const
	{
		return line_data;
	}
	
	T &operand(int n)
	{
		return operands[n];
	}
	
	const T &operand(int n) const
	{
		return operands[n];
	}
	
	T &create_operand()
	{
		operands.append(T());
		return operands[operands.length() - 1];
	}
	
	bool matches(asm_line_type op_type, int op_count, string op_data) const
	{
		return op_type == type && op_count == operands.length() && line_data == op_data;
	}
	
	bool matches(asm_line_type op_type, int op_count) const
	{
		return op_type == type && op_count == operands.length();
	}
};
