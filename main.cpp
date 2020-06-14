#include <cstdlib>
#include <cctype>
#include "p4lib/traits.h"
#include "p4lib/print.h"
#include "p4lib/string.h"
#include "p4lib/string_view.h"
#include "p4lib/array.h"
#include "p4lib/maybe.h"
#include "p4lib/utility.h"
#include "instructions.h"
#include "x86_instructions.h"
#include "snes_instructions.h"
#include "optimization.h"


asm_register_width output_snes_line(const snes_line &line, asm_register_width previous_width = BITS_NONE);


array<char> load_file(const char *file_name)
{
	FILE *file = fopen(file_name, "rb");
	fseek(file, 0, SEEK_END);
	unsigned int size = ftell(file);
	rewind(file);
	
	array<char> contents;
	contents.resize(size);
	if(fread(contents.data(), 1, size, file) != size){
		assertf(0, "Bad file read: %\n", file_name);
	}
	
	fclose(file);
	return contents;
}

array<string> split_lines(array<char> data)
{
	array<string> tokens;
	string current;
	bool comment_found = false;
	
	for(const auto &c : data){
		if(c == '#'){
			comment_found = true;
		}else if(c == '\n'){
			tokens.append(current.trim());
			current.clear();
			comment_found = false;
		}else if(!comment_found){
			current.append(c);
		}
	}
	return tokens;
}

array<x86_line> parse_lines(const array<string> &lines)
{
	array<x86_line> parsed_lines;
	string current;
	enum parse_phase : int{
		PARSE_DIRECTIVE_LABEL,
		PARSE_DIRECTIVE,
		PARSE_INSTRUCTION,
		PARSE_OPERAND,
		PARSE_OPERAND_HAS_NEXT,
		PARSE_END
	};
	
	for(const auto &line : lines){
		parse_phase phase = PARSE_INSTRUCTION;
		x86_line parsed_line;
		bool first_character = true;
		int current_operand = 0;
		bool eat_whitespace = true;
		bool in_delim = false;
		bool delim_open = false;
		char delim_type[2][3] = {"\"\"", "()"};
		for(const auto &c : line){
			if(eat_whitespace && isspace(c)){
				//discard
			}else if(first_character && c == '.'){
				phase = PARSE_DIRECTIVE_LABEL;
				parsed_line.type = DIRECTIVE;
				parsed_line.label().append(c);
				eat_whitespace = false;
			}else if(phase != PARSE_END){
				eat_whitespace = false;
				if(phase == PARSE_INSTRUCTION){
					if(c == ':'){
						parsed_line.type = LABEL;
						eat_whitespace = true;
						phase = PARSE_END;
					}else if(isident(c)){
						parsed_line.type = INSTRUCTION;
						parsed_line.mnemonic().append(c);
					}else if(isspace(c)){
						phase = PARSE_OPERAND;
						eat_whitespace = true;
					}else{
						assertf(0, "Parse error: Unexpected instruction character %", c);
					}
				}else if(phase == PARSE_OPERAND || phase == PARSE_OPERAND_HAS_NEXT){
					if(!current_operand){
						current_operand++;
						parsed_line.create_operand();
					}
					if(c == delim_type[0][0] || c == delim_type[1][0]){
						in_delim = true;
						delim_open = c == delim_type[1][0];
						parsed_line.operand(current_operand - 1).text.append(c);
					}else if(c != ',' || in_delim){
						parsed_line.operand(current_operand - 1).text.append(c);
						phase = PARSE_OPERAND;
						if(c == delim_type[delim_open][1]){
							in_delim = false;
						}
					}else if(c == ',' && phase != PARSE_OPERAND_HAS_NEXT){
						current_operand++;
						parsed_line.create_operand();
						eat_whitespace = true;
						phase = PARSE_OPERAND_HAS_NEXT;						
					}else if(c == ','){
						//Ignore for now this is common in directives.
						//Handle better if needed.
						//assert(0 && "Parse error: Double comma seperating operand");
					}
				}else if(phase == PARSE_DIRECTIVE_LABEL || phase == PARSE_DIRECTIVE){
					if(c == ':' && phase != PARSE_DIRECTIVE){
						parsed_line.type = LABEL;
						eat_whitespace = true;
						phase = PARSE_END;
					}else if(isgraph(c)){
						if(!isalnum(c) && c != '_'){
							phase = PARSE_DIRECTIVE;
						}
						parsed_line.label().append(c);
					}else if(isspace(c)){
						eat_whitespace = true;
						phase = PARSE_OPERAND;
					}else{
						assertf(0, "Parse error: Is this file binary or something....?");
					}
				}
			}else{
				assertf(0, "Parse error: Expected EOL");
			}
			first_character = false;
		}
		if(phase == PARSE_OPERAND_HAS_NEXT){
			assertf(0, "Parse error: Unexpected EOL");
		}
		parsed_lines.append(parsed_line);
	}
	return parsed_lines;
}

array<x86_line> discard_debug_sections(const array<x86_line> &lines)
{
	array<x86_line> cleaned_lines;
	bool keep_section = true;
	for(const auto &line : lines){
		if(keep_section && (line.type != DIRECTIVE || line.directive() == ".string")){
			cleaned_lines.append(line);
		}else{
			if(line.directive() == ".text" || line.directive() == ".data"){
				keep_section = true;
				cleaned_lines.append(line);
			}else if(line.directive() == ".section" && line.operand(0).text.starts_with(".debug")){
				keep_section = false;
			}else if(line.directive() == ".section" && line.operand(0).text.starts_with(".note")){
				keep_section = false;
			}else if(line.directive() == ".section" && line.operand(0).text.starts_with(".rodata")){
				x86_line label;
				label.type = LABEL;
				label.label() = "rodata";
				cleaned_lines.append(label);
				keep_section = true;
			}else if(line.directive() == ".section" || (keep_section && line.directive() != ".loc")){
				keep_section = true;
				cleaned_lines.append(line);
			}
		}
	}
	return cleaned_lines;
}

array<x86_line> clean_labels(const array<x86_line> &lines)
{
	array<x86_line> cleaned_lines;
	for(const auto &line : lines){
		if(line.type != LABEL){
			cleaned_lines.append(line);
		}else{
			if(!line.label().starts_with(".LBB") && !line.label().starts_with(".LBE") &&
				!line.label().starts_with(".LCFI")){
				cleaned_lines.append(line);
			}
		}
	}
	return cleaned_lines;
}

x86_register read_register(string_view &register_text)
{
	static const array<x86_register_map>registers = {
		{"%eax", {EAX, BITS_32}},
		{"%ebx", {EBX, BITS_32}},
		{"%ecx", {ECX, BITS_32}},
		{"%edx", {EDX, BITS_32}},
		{"%esi", {ESI, BITS_32}},
		{"%edi", {EDI, BITS_32}},
		{"%esp", {ESP, BITS_32}},
		{"%ebp", {EBP, BITS_32}},
		
		{"%ax", {EAX, BITS_16}},
		{"%bx", {EBX, BITS_16}},
		{"%cx", {ECX, BITS_16}},
		{"%dx", {EDX, BITS_16}},
		{"%si", {ESI, BITS_16}},
		{"%di", {EDI, BITS_16}},
		{"%sp", {ESP, BITS_16}},
		{"%bp", {EBP, BITS_16}},
		
		{"%ah", {EAX, BITS_8_HIGH}},
		{"%bh", {EBX, BITS_8_HIGH}},
		{"%ch", {ECX, BITS_8_HIGH}},
		{"%dh", {EDX, BITS_8_HIGH}},
		
		{"%al", {EAX, BITS_8_LOW}},
		{"%bl", {EBX, BITS_8_LOW}},
		{"%cl", {ECX, BITS_8_LOW}},
		{"%dl", {EDX, BITS_8_LOW}}
	};
	
	for(const auto &reg : registers){
		if(register_text.starts_with(reg.name)){
			register_text.shrink_prefix(reg.name.length());
			return reg.reg;
		}
	}
	
	return {NONE, BITS_NONE};
}

x86_operand decode_instruction_operand(x86_operand operand)
{
	string_view text = operand.text;
	if(!text.length()){
		return operand;
	}
	
	if(text.peek() == '%'){
		operand.type = REGISTER;
		operand.reg = read_register(text);
		assertf(operand.reg.value != NONE, "Invalid operand register");
	}else if(text.peek() == '-' || isdigit(text.peek()) || text.contains("(")){
		operand.type = MEMORY;
		operand.base = {NONE, BITS_NONE};
		operand.index = {NONE, BITS_NONE};
		operand.scale = 1;
		
		//this should handle the cases I expect gcc to generate
		//however if later it is found labels are not always first this can be changed here
		//also this assumes gcc won't typical generate extra math operations
		//alternative will be to parse the operand until ( and pass to snes asm
		if(text.read_if('.')){
			if(isidentstart(text.peek())){
				while(isident(text.peek())){
					operand.label.append(text.read());
				}
			}else{
				assertf(0, "Invalid label start");
			}
		}
		operand.offset = text.read_int();
		
		if(!text.length()){
			return operand;
		}
		assertf(text.read() == '(', "Unexpected operand character %", text.peek());
		if(text.peek() == '%'){
			operand.base = read_register(text);
			assertf(operand.base.value != NONE, "Invalid operand base register");
		}

		if(text.read_if(',')){
			operand.index = read_register(text);
			assertf(operand.index.value != NONE, "Invalid operand index register");
			
			if(text.read_if(',')){
				operand.scale = text.read_int();
				if(operand.scale != 2 && operand.scale != 4 && operand.scale != 8){
					assertf(0, "Invalid operand scale %", operand.scale);
				}
			}
		}
		
		assertf(text.peek() == ')', "Unexpected operand character %", text.peek());
	}else{
		operand.type = CONSTANT;
		if(text.read_if('$')){
			if(isdigit(text.peek()) || text.peek() == '-'){
				operand.constant = text.read_int();
			}else{
				operand.label = text.to_string();
			}
		}else{
			operand.label = text.to_string();
		}
	}
	
	return operand;
}

array<x86_line> decode_operands(const array<x86_line> &lines)
{
	array<x86_line> decoded_lines;
	for(x86_line line : lines){
		switch(line.type){
			case LABEL:
				decoded_lines.append(line);
			break;
			case DIRECTIVE:
				decoded_lines.append(line);
			break;
			case INSTRUCTION:
				for(int i = 0; i < line.operands.length(); i++){
					line.operands[i] = decode_instruction_operand(line.operands[i]);
				}
				decoded_lines.append(line);
			break;
			default:
				assertf(0, "Unknown operand state %.", line.type);
		}
	}
	return decoded_lines;
}

//subject to much change
snes_line translate_label(const string &line)
{
	snes_line translated;
	translated.type = LABEL;
	translated.label() = line;
	return translated;
}

snes_line translate_directive(const x86_line &line)
{
	//semi temporary only for .string
	snes_line translated;
	translated.type = DIRECTIVE;
	translated.directive() = "db";
	
	auto &operand = translated.create_operand();
	operand.text = line.operand(0).text;
	operand.type = CONSTANT;
	return translated;
}

array<snes_line> translate_instruction(const x86_line &line)
{
	//this may be worth splitting apart as it grows but good enough for now
	static const array<x86_instruction_map> instructions = {
		{"mov", &decode_mov},
		{"lea", &decode_lea},
		{"ret", &decode_ret},
		{"push", &decode_push},
		{"pop", &decode_pop},
		{"xor", &decode_xor},
		{"or", &decode_or},
		{"and", &decode_and},
		{"sal", &decode_sal},
		{"shr", &decode_shr},
		{"sar", &decode_shr},	//this may need to be properly implemented
		{"jmp", &decode_jmp},
		{"je", &decode_je},
		{"jne", &decode_jne},
		{"jl", &decode_jl},
		{"jge", &decode_jge},
		{"test", &decode_test},
		{"cmp", &decode_cmp},
		{"inc", &decode_inc},
		{"dec", &decode_dec},
		{"add", &decode_add},
		{"sub", &decode_sub}
	};
	
	for(const auto &instruction : instructions){
		if(line.mnemonic().starts_with(instruction.name)){
			return instruction.parser(line);
		}
	}
	print_f("Unhandled instruction: %", line.mnemonic());
	return array<snes_line>();
}

array<snes_line> translate(const array<x86_line> &lines)
{
	array<snes_line> translated_lines;
	
	array<snes_line> instruction_lines;
	
	for(const auto &line : lines){
		switch(line.type){
			case LABEL:
				translated_lines.append(translate_label(line.label()));
			break;
			case INSTRUCTION:
				instruction_lines = translate_instruction(line);
				for(const auto &instruction_line : instruction_lines){
					translated_lines.append(instruction_line);
				}
			break;
			case DIRECTIVE:
				if(line.directive() == ".string"){
					translated_lines.append(translate_directive(line));
				}
			default:
			break;
		}
	}
	return translated_lines;
}

int calculate_distance(int branch, int label, const array<snes_line> &lines)
{
	int distance = label < branch ? -2 : 0;
	int direction = label < branch ? -1 : 1;	//aka distance +1
	for(int i = branch + direction; i != label; i += direction){
		distance += direction;
		for(const auto &operand : lines[i].operands){
			distance += operand.width * direction;
		}
	}
	return distance;
}

pair<array<int>, array<int>> get_branch_pairs(const array<snes_line> &lines)
{
	array<int> branches;
	array<int> labels;
	for(auto i = 0; const auto &line : lines){
		switch(line.type){
			case LABEL:
				labels.append(i);
			break;
			case INSTRUCTION:
				if(line.operands.length() == 1 && line.operand(0).mode == RELATIVE){
					branches.append(i);
				}
			break;
			default:;
		}
		i++;
	}
	return {branches, labels};
}

auto get_distances(const array<int> &branches, const array<int> &labels, const array<snes_line> &lines)
{
	array<pair<pair<int, int>, int>>  distances;
	for(const auto &branch : branches){
		auto destination = lines[branch].operand(0).label;
		for(const auto &label : labels){
			if(lines[label].label() == destination){
				distances.append({{branch, label}, calculate_distance(branch, label, lines)});
				break;
			}
		}
	}
	return distances;
}

array<int> get_invalid_branches(const array<snes_line> &lines)
{
	array<int> invalid_branches;
	auto [branches, labels] = get_branch_pairs(lines);
	array<pair<pair<int, int>, int>> distances = get_distances(branches, labels, lines);
	
	bool invalid_detected = false;
	do{
		invalid_detected = false;
		for(auto &[branch_pair, distance] : distances){
			if(distance < -127 || distance > 127){
				auto [branch, label] = branch_pair;
				invalid_detected = true;
				invalid_branches.append(branch);
				distance = 0;	//after a branch is invalidated, distance is safe
				
				for(auto &[adjacent_branch_pair, adjacent_distance] : distances){
					auto &[adjacent_branch, adjacent_label] = adjacent_branch_pair;
					if(adjacent_branch == branch){
						continue;
					}
					//bra is supported but never really used at this time
					if(adjacent_branch > branch && adjacent_label < branch){
						adjacent_distance += lines[branch].mnemonic() == "bra" ? 1 : 3;
					}else if(adjacent_branch < branch && adjacent_label > branch){
						adjacent_distance += lines[branch].mnemonic() == "bra" ? 1 : 3;;
					}
				}

			}
		}
	}while(invalid_detected);
	
	return invalid_branches;
}

string negate_branch(string branch)
{
	pair<string, string> branch_map[] = {
		{"beq", "bne"},
		{"bne", "beq"},
		{"bcs", "bcc"},
		{"bcc", "bcs"},
		{"bvs", "bvc"},
		{"bvc", "bvs"},
		{"bpl", "bmi"},
		{"bmi", "bpl"}
	};
	
	if(branch == "bra"){
		return "jmp";
	}
	
	for(const auto &[original, negation] : branch_map){
		if(branch == original){
			return negation;
		}
	}
	assertf(0, "Branch repair error: something went nuclear with branch %", branch);
}

void correct_branches(array<snes_line> &lines, array<int> &invalid_branches)
{
	sort(invalid_branches);
	for(int i = 0; auto invalid_branch : invalid_branches){
		string negation = negate_branch(lines[invalid_branch + i].mnemonic());
		auto target = lines[invalid_branch + i].operand(0);
		if(negation == "bra"){
			lines[invalid_branch + i] = get_pc_instruction("jmp", DIRECT, string("+"));
		}else{
			lines[invalid_branch + i] = get_pc_instruction(negation, RELATIVE, string("+"));
			i++;
			if(target.label.length()){
				lines.insert(get_pc_instruction("jmp", DIRECT, target.label), invalid_branch + i);
			}else{
				lines.insert(get_pc_instruction("jmp", DIRECT, target.value), invalid_branch + i);
			}
			i++;
			lines.insert(translate_label("+"), invalid_branch + i);
		}
	}
}

void dump_parsed_instructions(const array<x86_line> &lines)
{
	for(const auto &line : lines){
		switch(line.type){
			case INSTRUCTION:
				print_f("INSTRUCTION: %\n", line.mnemonic());
				for(const auto &operand : line.operands){
					print_f("\t%\n", operand.text);
					
					switch(operand.type){
						case CONSTANT:
							if(operand.label.length()){
								print_f("\t\t%\n", operand.label);
							}else{
								print_f("\t\t%\n", operand.constant);
							}
						break;
						case MEMORY:
							print_f("\t\t%\n", operand.offset);
							print_f("\t\t% | %\n", operand.base.value, operand.base.width);
							print_f("\t\t% | %\n", operand.index.value, operand.index.width);
							print_f("\t\t%\n", operand.scale);
						break;
						case REGISTER:
							print_f("\t\t% | %\n", operand.reg.value, operand.reg.width);
						break;
						default:
							assertf(0, "bad operand type: %", operand.type);
					}
				}
				break;
			case LABEL:
				print_f("LABEL: %s\n", line.label());
				break;
			case DIRECTIVE:
				print_f("DIRECTIVE: %s\n", line.directive());
				for(const auto &operand : line.operands){
					print_f("\t%s\n", operand.text);
				}
				break;
			default:
				assertf(0, "Bad things");
		}
	}	
}

void dump_translated_instructions(const array<snes_line> &lines)
{
	for(const auto &line : lines){
		switch(line.type){
			case INSTRUCTION:
				print_f("INSTRUCTION: %\n", line.mnemonic());
				for(const auto &operand : line.operands){
					echo('\t');
					switch(operand.type){
						case CONSTANT:
							echo('#');
						case MEMORY:
							if(operand.label.length()){
								print_f("$%s\n", operand.label);
							}else{
								print_f("${,X}\n", operand.value);
							}
						break;
						default:
							assertf(0, "bad operand type: %", operand.type);
					}
				}
				break;
			case LABEL:
				print_f("LABEL: %s\n", line.label());
				break;
			case DIRECTIVE:
				print_f("DIRECTIVE: %s\n", line.directive());
				for(const auto &operand : line.operands){
					print_f("\t%s\n", operand.text);
				}
				break;
			default:
				assertf(0, "Bad things");
		}
	}	
}

asm_register_width output_snes_line(const snes_line &line, asm_register_width previous_width)
{
	bool skip_newline = false;
	switch(line.type){
		case INSTRUCTION:
			print_f("% ", line.mnemonic());
			for(const auto &operand : line.operands){
				echo('\t');
				switch(operand.type){
					case CONSTANT:
						if(!operand.label.length()){
							echo('#');
						}
					case MEMORY:
						if(operand.mode == INDIRECT || operand.mode == INDIRECT_Y){
							echo("(");
						}
						
						if(operand.label.length()){
							print_f("%", operand.label);
						}else{
							if(operand.width == BITS_24){
								print_f("${0,6x}", operand.value);
							}else if((operand.type == CONSTANT && previous_width != BITS_8) 
							&& operand.width == BITS_16){
								print_f("${0,4x}", operand.value);
							}else{
								print_f("${0,2x}", operand.value);
							}
						}
						if(operand.mode == INDIRECT || operand.mode == INDIRECT_Y){
							echo(")");
						}
						
						if(operand.mode == INDEXED_X){
							echo(",x");
						}else if(operand.mode == INDIRECT_Y){
							echo(",y");
						}else if(operand.mode == STACK){
							echo(",s");
						}
					break;
					default:
						assertf(0, "bad operand type: %", operand.type);
				}
			}
			break;
		case LABEL:
			previous_width = BITS_NONE;
			print_f("%%", line.label(), isidentstart(line.label()[0]) ? ':' : ' ');
			break;
		case DIRECTIVE:
			if(line.directive() != ".width"){
				print_f("%", line.directive());
			}
			for(const auto &operand : line.operands){
				if(line.directive() == ".width"){
					skip_newline = true;
					if(operand.width == BITS_8 && previous_width != operand.width){
						print_ln("sep\t#$20");
					}else if(operand.width == BITS_16 || operand.width == BITS_32){
						if(previous_width == BITS_8 || previous_width == BITS_NONE){
							print_ln("rep\t#$20");
						}else{
							//print_ln(";width");
						}
					}else{
						//print_ln(";width");
					}
					previous_width = operand.width;
				}else{
					print_f(" %", operand.text);
				}
			}
			break;
		default:
			assertf(0, "Bad things");
	}
	if(!skip_newline){
		echo('\n');
	}
	
	return previous_width;
}

void output_snes(const array<snes_line> &lines)
{
	
	asm_register_width previous_width = BITS_NONE;
	for(const auto &line : lines){
		previous_width = output_snes_line(line, previous_width);
	}	
}

int main(int argc, char *argv[])
{
	test_string();
	test_array();
	if(argc > 1){
		//load_file(argv[1]);
	}else{
		//load_stdin();
	}
	
	//auto lines = split_lines(load_file("test.s"));
	auto lines = split_lines(load_file("demos/boot_snes.s"));
	auto parsed_lines = parse_lines(lines);
	parsed_lines = discard_debug_sections(parsed_lines);
	parsed_lines = clean_labels(parsed_lines);
	parsed_lines = decode_operands(parsed_lines);
	print_f(";input line count: %\n", lines.length());
	print_f(";trimmed input count: %\n", parsed_lines.length());
	
	auto translated_lines = translate(parsed_lines);
	auto invalid_branches = get_invalid_branches(translated_lines);
	print_f(";invalid branch count: %\n", invalid_branches.length());
	correct_branches(translated_lines, invalid_branches);
	//dump_parsed_instructions(parsed_lines);
	//dump_translated_instructions(translated_lines);
	print_f(";first pass translation count: %\n", translated_lines.length());
	//output_snes(translated_lines);
	translated_lines = optimize(translated_lines);
	print_f(";optimization pass translation count: %\n", translated_lines.length());
	output_snes(translated_lines);

	maybe<int> test(1);
	maybe<int> test1(nothing);
	//print_ln(test.get(), test.get_or(2), test1.valid(), test1.get_or(3));
	return 0;
}

