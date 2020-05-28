#pragma once
#include "instructions.h"
#include "snes_instructions.h"

enum x86_register_name{
	NONE,
	EAX,
	EBX,
	ECX,
	EDX,
	ESI,
	EDI,
	ESP,
	EBP
};

struct x86_register{
	x86_register_name value;
	asm_register_width width;
};

struct x86_register_map{
	string name;
	x86_register reg;
};

struct x86_operand{
	string text;
	asm_operand_type type = UNUSED;
	string label;
	union{
		x86_register reg;
		unsigned int constant;
		struct{
			//I'm hoping to get away without supporting FS/GS segments...
			int offset;
			x86_register base;
			x86_register index;
			int scale;
		};
	};
};

typedef asm_line<x86_operand> x86_line;

struct x86_instruction_map{
	string name;
	array<snes_line> (*parser)(const x86_line &);
};

array<snes_line> dummy(const x86_line &line);

//memory operations
array<snes_line> decode_mov(const x86_line &line);
array<snes_line> decode_lea(const x86_line &line);

//stack operations
array<snes_line> decode_push(const x86_line &line);
array<snes_line> decode_pop(const x86_line &line);

//bitwise operations
array<snes_line> decode_xor(const x86_line &line);
array<snes_line> decode_or(const x86_line &line);
array<snes_line> decode_and(const x86_line &line);
array<snes_line> decode_test(const x86_line &line);

//shift operations
array<snes_line> decode_sal(const x86_line &line);
array<snes_line> decode_shr(const x86_line &line);

//arithmetic operations
array<snes_line> decode_cmp(const x86_line &line);
array<snes_line> decode_inc(const x86_line &line);
array<snes_line> decode_dec(const x86_line &line);
array<snes_line> decode_add(const x86_line &line);
array<snes_line> decode_sub(const x86_line &line);

//common branches
array<snes_line> decode_jmp(const x86_line &line);
array<snes_line> decode_je(const x86_line &line);
array<snes_line> decode_jne(const x86_line &line);
array<snes_line> decode_jl(const x86_line &line);
array<snes_line> decode_jge(const x86_line &line);

//control flow
array<snes_line> decode_ret(const x86_line &line);
