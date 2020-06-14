#include "p4lib/traits.h"
#include "snes_instructions.h"
#include "p4lib/print.h"

snes_line get_directive(const string &directive)
{
	snes_line line;
	line.type = DIRECTIVE;
	line.directive() = directive;
	return line;
}

snes_line get_width_directive(asm_register_width width)
{
	snes_line line;
	line.type = DIRECTIVE;
	line.directive() = ".width";
	line.create_operand().width = width;
	return line;
}

snes_line get_instruction(const string &instruction)
{
	snes_line line;
	line.type = INSTRUCTION;
	line.mnemonic() = instruction;
	return line;
}

template <typename T>
snes_line get_instruction(const string &instruction, asm_operand_type type, T value)
{
	snes_line line = get_instruction(instruction);
	add_operand(line, type, value);
	return line;
}

template snes_line get_instruction<unsigned int>(const string &instruction, asm_operand_type type, unsigned int value);
template snes_line get_instruction<string>(const string &instruction, asm_operand_type type, string value);

template <typename T>
snes_operand &add_operand(snes_line &line, asm_operand_type type, T value)
{
	int operand = line.operands.length();
	line.create_operand().type = type;
	
	if constexpr(is_integral_v<T>){
		line.operand(operand).value = value;
	}else if constexpr(is_same_v<string, T>){
		line.operand(operand).label = value;
	}
	
	return line.operand(operand);
}

template snes_operand &add_operand<unsigned int>(snes_line &line, asm_operand_type type, unsigned int value);
template snes_operand &add_operand<string>(snes_line &line, asm_operand_type type, string value);

template <typename T>
snes_line get_memory_instruction(const string &instruction, snes_address_mode mode, T address)
{
	snes_line line = get_instruction(instruction);
	snes_operand &operand = add_operand(line, MEMORY, address);
	operand.mode = mode;
	
	//if we know the address is dp we should optimize it
	//todo handle long addresses
	if constexpr(is_integral_v<T>){
		if(address < 0x100){
			operand.width = BITS_8;
		}else if(address < 0x10000){
			operand.width = BITS_16;
		}else{
			operand.width = BITS_24;
		}
	}else{
		//todo don't assume word length
		operand.width = BITS_16;
	}
	return line;
}
template snes_line get_memory_instruction(const string &instruction, snes_address_mode mode, int address);
template snes_line get_memory_instruction(const string &instruction, snes_address_mode mode, unsigned int address);
template snes_line get_memory_instruction(const string &instruction, snes_address_mode mode, string address);

template <typename T>
snes_line get_pc_instruction(const string &instruction, snes_address_mode mode, T address)
{
	snes_line line = get_instruction(instruction);
	snes_operand &operand = add_operand(line, CONSTANT, address);
	operand.mode = mode;
	
	if constexpr(is_integral_v<T>){
		if(mode != RELATIVE && address < 0x10000){
			operand.width = BITS_16;
		}else if(mode != RELATIVE){
			operand.width = BITS_24;
		}else{
			operand.width = BITS_8;
		}
	}else{
		//assumes no brl for now
		if(instruction[0] == 'b'){
			operand.width = BITS_8;
		}else if(instruction[2] == 'r' || instruction[2] == 'p'){ //jsr/jmp
			operand.width = BITS_16;
		}else{	//jsl
			operand.width = BITS_24;
		}
	}
	return line;
}
template snes_line get_pc_instruction(const string &instruction, snes_address_mode mode, unsigned int address);
template snes_line get_pc_instruction(const string &instruction, snes_address_mode mode, string address);
