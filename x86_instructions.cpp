#include "x86_instructions.h"
#include "snes_instructions.h"
#include "p4lib/print.h"

bool match(const x86_operand &op, asm_operand_type type)
{
	return op.type == type;
}

bool match(const x86_operand &op1, asm_operand_type type1, const x86_operand &op2, asm_operand_type type2)
{
	return match(op1, type1) && match(op2, type2);
}

int get_mask(const x86_line &line)
{
	int mask = 0;
	mask += line.mnemonic().ends_with("b") * 0xFF;
	mask += line.mnemonic().ends_with("w") * 0xFFFF;
	mask += line.mnemonic().ends_with("l") * 0xFFFFFFFF;
	return mask;
}

snes_address_mode get_addressing_mode(const x86_operand &operand)
{
	if(operand.type == REGISTER || (operand.base.value == NONE && operand.index.value == NONE)){
		return DIRECT;
	}else if(operand.base.value != NONE && operand.index.value == NONE){
		return INDIRECT;
	}
	
	assertf(0, "Unsupported addressing mode");
}

snes_line get_register_width(const string &mnemonic)
{
	char end = *(mnemonic.end() - 1);
	return get_width_directive(end == 'b' ? BITS_8 :
					end == 'w' ? BITS_16 :
					BITS_32);
}

unsigned int register_address(const x86_register reg)
{
	return (reg.value - 1) * 4 + (reg.width == BITS_8_HIGH);
}

array<snes_line> index_memory(const x86_operand &operand)
{
	array<snes_line> lines;
	if(operand.index.value != NONE){
		lines += get_memory_instruction("lda", DIRECT, register_address(operand.index));
		for(int i = operand.scale; i > 1; i >>= 1){
			lines += get_instruction("asl");
		}
	}
	
	if(operand.index.value != NONE && operand.base.value != NONE){
		lines += get_instruction("clc");
		lines += get_memory_instruction("adc", DIRECT, register_address(operand.base));
	}else if(operand.base.value != NONE){
		lines += get_memory_instruction("lda", DIRECT, register_address(operand.base));
	}
	
	if(operand.offset != 0){
		lines += get_instruction("clc");
		lines += get_instruction("adc", CONSTANT, (operand.offset & 0xFFFFu));
	}

	return lines;
}

array<snes_line> build_esp_instruction(string mnemonic, const x86_operand &op)
{
	array<snes_line> lines;
	switch(get_addressing_mode(op)){
		case DIRECT:
			if(mnemonic == "sta"){
				return {get_instruction("tcs")};
			}else if(mnemonic == "lda"){
				return {get_instruction("tsc")};
			}
			//operation on stack opcode
			return {
				get_instruction("tsc"),
				get_memory_instruction(mnemonic, DIRECT, op.offset)
			};
		case INDIRECT:
			assertf(!op.label.length(), "labeled indirect indexed esp not supported");
			return {get_memory_instruction(mnemonic, STACK, op.offset + 1)};
	}
	assertf(0, "unsupported addressing mode");
	return lines;
}

array<snes_line> build_memory_instruction(string mnemonic, const x86_operand &op)
{
	//hacky esp optimization
	if(op.base.value == ESP || op.offset == 0x18){
		return build_esp_instruction(mnemonic, op);
	}
	array<snes_line> lines;
	switch(get_addressing_mode(op)){
		case DIRECT:
			return {get_memory_instruction(mnemonic, DIRECT, op.offset)};
		case INDIRECT:
			if(op.offset || op.label.length()){
				string target;
				if(op.label.length()){
					target = string("#rodata_") + op.label;
				}
				if(op.offset){
					if(!target.length()){	//todo this is a really really nasty hack
						target = "#";
					}
					target += to_string(op.offset);
				}
				lines += get_memory_instruction("ldy.w", DIRECT, target);
				lines += get_memory_instruction(mnemonic, INDIRECT_Y, register_address(op.base));
			}else{
				lines += get_memory_instruction(mnemonic, INDIRECT, register_address(op.base));
			}
			return lines;
	}
	assertf(0, "unsupported addressing mode");
	return lines;
}

//currently this is meant for shift instructions as the snes supports indirects on most other arithmetics
//could be used for inc/dec too
array<snes_line> build_synthetic_memory_instruction(string mnemonic, const x86_operand &op)
{
	//won't support esp here until I see it necessary
	array<snes_line> lines;
	switch(get_addressing_mode(op)){
		case DIRECT:
			if(op.reg.value == NONE){
				return {get_memory_instruction(mnemonic, DIRECT, op.offset)};
			}else{
				return {get_memory_instruction(mnemonic, DIRECT, register_address(op.reg))};
			}
		case INDIRECT:
			if(op.offset || op.label.length()){
				string target;
				if(op.label.length()){
					target = string("#rodata_") + op.label;
				}
				if(op.offset){
					target += to_string(op.offset);
				}
				lines += get_memory_instruction("ldy", DIRECT, target);
				lines += get_memory_instruction("lda", INDIRECT_Y, register_address(op.base));
				lines += get_instruction(mnemonic);
				lines += get_memory_instruction("sta", INDIRECT_Y, register_address(op.base));
			}else{
				lines += get_memory_instruction("lda", INDIRECT, register_address(op.base));
				lines += get_instruction(mnemonic);
				lines += get_memory_instruction("sta", INDIRECT, register_address(op.base));
			}
			return lines;
	}
	assertf(0, "unsupported addressing mode");
	return lines;
}

snes_line build_register_instruction(string mnemonic, const x86_operand &op)
{
	if(op.reg.value == ESP){
		if(mnemonic == "sta"){
			return get_instruction("tcs");
		}else if(mnemonic == "lda"){
			return get_instruction("tsc");
		}
	}else{
		return get_memory_instruction(mnemonic, DIRECT, register_address(op.reg));
	}
	assertf(0, "build ESP register instruction failed");
	return snes_line();
}


array<snes_line> dummy(const x86_line &line)
{
	return {get_instruction(string(";").append(line.mnemonic()))};
}

array<snes_line> decode_binary_arithmetic(const string &mnemonic, const x86_line &line)
{
	assert(line.operands.length() == 2);
	array<snes_line> lines;
	
	int mask = get_mask(line);
	const auto op1 = line.operand(0);
	const auto op2 = line.operand(1);
	
	lines += get_register_width(line.mnemonic());
	
	//todo optimize build indexed indirect to a single ldx
	if(match(op1, CONSTANT, op2, MEMORY)){
		lines += build_memory_instruction("lda", op2);
		if(op1.label.length()){
			lines += get_instruction(mnemonic, CONSTANT, string("#rodata_") + (op1.label.begin() + 1));
		}else{
			lines += get_instruction(mnemonic, CONSTANT, op1.constant & mask);
		}
		lines += build_memory_instruction("sta", op2);
	}else if(match(op1, CONSTANT, op2, REGISTER)){
		lines += build_register_instruction("lda", op2);
		if(op1.label.length()){
			lines += get_instruction(mnemonic, CONSTANT, string("#rodata_") + (op1.label.begin() + 1));
		}else{
			lines += get_instruction(mnemonic, CONSTANT, op1.constant & mask);
		}
		lines += build_register_instruction("sta", op2);
	}else if(match(op1, MEMORY, op2, REGISTER)){
		lines += build_register_instruction("lda", op2);
		lines += build_memory_instruction(mnemonic, op2);
		lines += build_register_instruction("sta", op2);
	}else if(match(op1, REGISTER, op2, REGISTER)){
		lines += build_register_instruction("lda", op2);
		lines += build_register_instruction(mnemonic, op1);
		lines += build_register_instruction("sta", op2);
	}else if(match(op1, REGISTER, op2, MEMORY)){
		lines += build_memory_instruction("lda", op2);
		lines += build_register_instruction(mnemonic, op1);
		lines += build_memory_instruction("sta", op2);
	}else{
		assert(0 && "unhandled operand");
	}
	
	//bit is special and results don't need stored
	if(mnemonic == "bit"){
		lines.remove(lines.length());
	}

	return lines;
}

array<snes_line> decode_branch(const string &mnemonic, const x86_line &line)
{
	assert(line.operands.length() == 1);
	array<snes_line> lines;
	
	const auto op1 = line.operand(0);
	
	if(match(op1, CONSTANT)){
		if(op1.label.length()){
			lines += get_pc_instruction(mnemonic, RELATIVE, op1.label);
		}else{
			lines += get_pc_instruction(mnemonic, RELATIVE, op1.constant);
		}
	}else{
		assert(0 && "unhandled operand");
	}

	return lines;
}

//todo constant hack fix
array<snes_line> decode_mov(const x86_line &line)
{
	assert(line.operands.length() == 2);
	array<snes_line> lines;
	
	int mask = get_mask(line);
	const auto op1 = line.operand(0);
	const auto op2 = line.operand(1);
	
	assertf(op1.scale <= 1 && op2.scale <= 1, "Scalars not supported\n");
	
	if(line.mnemonic().contains("z")){
		assertf(line.mnemonic()[4] == 'b', "only movzbl currently supported");
		lines += get_register_width("b"); //this is probably pretty hacky, but it works
		lines += get_instruction("tdc"); //this needs changed if dp changes ever
	}else{
		lines += get_register_width(line.mnemonic());
	}
	
	if(match(op1, CONSTANT, op2, MEMORY)){
		if(op1.label.length()){
			lines += get_instruction("lda", CONSTANT, string("#rodata_") + (op1.label.begin() + 1));
			lines += build_memory_instruction("sta", op2);
		}else{
			if(op1.constant){
				lines += get_instruction("lda", CONSTANT, op1.constant & mask);
				lines += build_memory_instruction("sta", op2);
			}else{
				lines += build_memory_instruction("stz", op2);
			}
		}
		//lines += get_memory_instruction("sta", get_addressing_mode(op2), op2.offset);
	}else if(match(op1, CONSTANT, op2, REGISTER)){
		if(op1.label.length()){
			lines += get_instruction("lda", CONSTANT, string("#rodata_") + (op1.label.begin() + 1));
		}else{
			lines += get_instruction("lda", CONSTANT, op1.constant & mask);
		}
		lines += build_register_instruction("sta", op2);
	}else if(match(op1, MEMORY, op2, REGISTER)){
		//lines += get_memory_instruction("lda", get_addressing_mode(op1), op1.offset);
		lines += build_memory_instruction("lda", op1);
		lines += build_register_instruction("sta", op2);
	}else if(match(op1, REGISTER, op2, MEMORY)){
		lines += build_register_instruction("lda", op1);
		lines += build_memory_instruction("sta", op2);
		//lines += get_memory_instruction("sta", get_addressing_mode(op2), op2.offset);
	}else if(match(op1, REGISTER, op2, REGISTER)){
		lines += build_register_instruction("lda", op1);
		lines += build_register_instruction("sta", op2);
	}else{
		assert(0 && "unhandled operand");
	}
	
	if(line.mnemonic().contains("z")){
		lines.insert(get_register_width(line.mnemonic()), lines.length() - 1);
	}

	return lines;
}

array<snes_line> decode_lea(const x86_line &line)
{
	assert(line.operands.length() == 2);
	array<snes_line> lines;
	
	const auto op1 = line.operand(0);
	const auto op2 = line.operand(1);
	
	lines += get_register_width(line.mnemonic());
	lines += index_memory(op1);
	//lines += get_instruction("tax");
	
	if(match(op2, REGISTER)){
	//	lines += get_memory_instruction("lda", INDEXED_X, 0);
		lines += get_memory_instruction("sta", DIRECT, register_address(op2.reg));
	}else{
		assert(0 && "unhandled operand");
	}

	return lines;
}

array<snes_line> decode_ret(const x86_line &line)
{
	//assume short return for now.  Do something more complex later.
	assert(line.operands.length() == 0);
	return {get_instruction("rts")};
}

array<snes_line> decode_push(const x86_line &line)
{
	assert(line.operands.length() == 1);
	assert(line.operand(0).type == REGISTER);
	
	return {
		get_register_width(line.mnemonic()),
		get_memory_instruction("pei", INDIRECT, register_address(line.operand(0).reg))
	};
}

array<snes_line> decode_pop(const x86_line &line)
{
	assert(line.operands.length() == 1);
	assert(line.operand(0).type == REGISTER);
	
	return {
		get_register_width(line.mnemonic()),
		get_instruction("pla"),
		get_memory_instruction("sta", DIRECT, register_address(line.operand(0).reg))
	};
}

array<snes_line> decode_xor(const x86_line &line)
{
	assert(line.operands.length() == 2);

	const auto op1 = line.operand(0);
	const auto op2 = line.operand(1);
	if(match(op1, REGISTER, op2, REGISTER)){
		if(op1.reg.value == op2.reg.value){
			return {get_memory_instruction("stz", DIRECT, register_address(op1.reg))};
		}
	}
	
	return decode_binary_arithmetic("eor", line);
}

array<snes_line> decode_or(const x86_line &line)
{
	return decode_binary_arithmetic("ora", line);
}

array<snes_line> decode_and(const x86_line &line)
{
	return decode_binary_arithmetic("and", line);
}

array<snes_line> decode_test(const x86_line &line)
{
	return decode_binary_arithmetic("bit", line);
}

array<snes_line> decode_add(const x86_line &line)
{
	return array({get_instruction("clc")}).append(decode_binary_arithmetic("adc", line));
}

array<snes_line> decode_sub(const x86_line &line)
{
	return array({get_instruction("sec")}).append(decode_binary_arithmetic("sbc", line));
}

array<snes_line> decode_shift(string mnemonic, const x86_line &line)
{
	assert(line.operands.length() == 1 || line.operands.length() == 2);
	array<snes_line> lines;
	lines += get_register_width(line.mnemonic());
	if(line.operands.length() == 1){
		lines += build_synthetic_memory_instruction(mnemonic, line.operand(0));
	}else{
		const auto op1 = line.operand(0);
		const auto op2 = line.operand(1);
			
		for(auto i = 0u; i < op1.constant; i++){
			lines += build_synthetic_memory_instruction(mnemonic, op2);
		}
	}
	return lines;
}

array<snes_line> decode_shr(const x86_line &line)
{
	return decode_shift("lsr", line);
}

array<snes_line> decode_sal(const x86_line &line)
{
	return decode_shift("asl", line);
}

array<snes_line> decode_cmp(const x86_line &line)
{
	auto cmp_lines = decode_binary_arithmetic("cmp", line);	//last line is an sta we don't need
	return cmp_lines.remove(cmp_lines.length());
}

array<snes_line> decode_inc(const x86_line &line)
{
	assert(line.operands.length() == 1);
	assert(line.operand(0).type == REGISTER || line.operand(0).type == MEMORY);
	auto op = line.operand(0);
	
	if(op.type == REGISTER){
		return {
			get_register_width(line.mnemonic()),
			get_memory_instruction("inc", DIRECT, register_address(op.reg))
		};
	}else{
		return {
			get_register_width(line.mnemonic()),
			get_memory_instruction("inc", get_addressing_mode(op), op.offset)
		};
	}
}

array<snes_line> decode_dec(const x86_line &line)
{
	assert(line.operands.length() == 1);
	assert(line.operand(0).type == REGISTER || line.operand(0).type == MEMORY);
	auto op = line.operand(0);
	
	if(op.type == REGISTER){
		return {
			get_register_width(line.mnemonic()),
			get_memory_instruction("dec", DIRECT, register_address(op.reg))
		};
	}else{
		return {
			get_register_width(line.mnemonic()),
			get_memory_instruction("dec", get_addressing_mode(op), op.offset)
		};
	}
}


array<snes_line> decode_jmp(const x86_line &line)
{
	assert(line.operands.length() == 1);
	const auto op1 = line.operand(0);
	
	if(match(op1, CONSTANT)){
		if(op1.label.length()){
			return {get_pc_instruction("jmp", DIRECT, op1.label)};
		}else{
			return {get_pc_instruction("jmp", DIRECT, op1.constant)};
		}
	}
	//temporary
	return {get_pc_instruction("jmp", DIRECT, op1.label)};
}

array<snes_line> decode_je(const x86_line &line)
{
	return decode_branch("beq", line);
}

array<snes_line> decode_jne(const x86_line &line)
{
	return decode_branch("bne", line);
}

array<snes_line> decode_jl(const x86_line &line)
{
	return decode_branch("bcc", line);
}

array<snes_line> decode_jge(const x86_line &line)
{
	return decode_branch("bcs", line);
}
