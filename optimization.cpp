#include "p4lib/print.h"
#include "optimization.h"

//this is more of a poormans class to allow use of const auto lambda types.
//maybe I will get more creative in the future
namespace {
	snes_operand state;
	snes_operand alt_state;
	
	//maybe this should take an arg....but for now good enough
	const auto use_alt_state = [](const auto &comparator){
		return [comparator](const snes_line &line) {
			return scoped_swap(state, alt_state), comparator(line);
		};
	};
	
	const auto get_dp_value = [](const snes_line &line){
		state.value = line.operands.length() ? line.operand(0).value : 0;
		return line.operands.length() && line.operand(0).type == MEMORY && state.value < 0x100;
	};
	
	const auto get_value = [](const snes_line &line){
		state.value = line.operands.length() ? line.operand(0).value : 0;
		return line.operands.length() && line.operand(0).type == MEMORY;
	};
	
	
	const auto value = [](const snes_line &line){
		return line.operands.length() && line.operand(0).value == state.value;
	};
	
	const auto use_value = [](unsigned int value, const auto &comparator){
		return [comparator, value](const snes_line &line) {
			return scoped_temp(state.value, value), comparator(line);
		};
	};
	
	const auto get_mode = [](const snes_line &line){
		state.mode = line.operands.length() ? line.operand(0).mode : UNSPECIFIED;
		return line.operands.length();
	};
	
	const auto mode = [](const snes_line &line){
		return line.operands.length() && line.operand(0).mode == state.mode;
	};
	
	const auto memory = [](const snes_line &line){
		return line.operands.length() && line.operand(0).type == MEMORY;
	};
	
	const auto constant = [](const snes_line &line){
		return line.operands.length() && line.operand(0).type == CONSTANT;
	};
	
	const auto direct = [](const snes_line &line){
		return line.operands.length() && line.operand(0).mode == DIRECT;
	};
	
	const auto indirect = [](const snes_line &line){
		return line.operands.length() && line.operand(0).mode == INDIRECT;
	};
	
	const auto sta = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "sta");
	};
	
	const auto width = [](const snes_line &line){
		return line.matches(DIRECTIVE, 1, ".width");
	};
	
	const auto lda = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "lda");
	};
	
	const auto adc = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "adc");
	};
	
	const auto clc = [](const snes_line &line){
		return line.matches(INSTRUCTION, 0, "clc");
	};
	
	const auto anything = [](const snes_line &line){
		return true;
	};
	
	const auto bit_shift = [](const snes_line &line){
		return (line.matches(INSTRUCTION, 1, "lsr") || line.matches(INSTRUCTION, 1, "asl"));
	};
	
	const auto bit_logic = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "ora") || line.matches(INSTRUCTION, 1, "and") ||
			line.matches(INSTRUCTION, 1, "eor");
	};
	
	//expand to more types later
	const auto passive_instruction = [](const snes_line &line){
		return (line.type == INSTRUCTION && !line.mnemonic().starts_with("st"));
	};
	
	namespace detail{
		enum operations{
			ALL,
			NONE,
			ONE,
			ANY
		};
		
		const auto apply = [](const operations operation, const auto &...comparators){
			return [operation, comparators...](const snes_line &line){
				tuple comparator_collection = {comparators...};
				bool result = operation == ALL || operation == NONE;
				for(int i = 0; i < comparator_collection.size; i++){
					visit(comparator_collection, i, [&](auto comparator){
						if(operation == ALL){
							result &= comparator(line);
						}else if(operation == NONE){
							result &= !comparator(line);
						}else if(operation == ONE){
							if(result && comparator(line)){
								result = false;
								i = comparator_collection.size;
							}else{
								result ^= comparator(line);
							}
						}else if(operation == ANY){
							result |= comparator(line);
						}else{
							assertf(0, "operation not supported %", operation);
						}
					});	
				}
				return result;
			};
		};
	};
	
	
	const auto all_of = [](const auto &...comparators){
		return detail::apply(detail::ALL, comparators...);
	};

	const auto none_of = [](const auto &...comparators){
		return detail::apply(detail::NONE, comparators...);
	};
	
	const auto one_of = [](const auto &...comparators){
		return detail::apply(detail::ONE, comparators...);
	};
	
	const auto any_of = [](const auto &...comparators){
		return detail::apply(detail::ANY, comparators...);
	};
	
	int debug_count = 0;
	const auto debug_start = [](const snes_line &line){
		debug_count = 0;
		print_f("DEBUG START\n");
		return true;
	};
	const auto debug = [](const snes_line &line){
		print_f("DEBUG: %\n", debug_count++);
		return true;
	};
	
};

int find_register_store(int start, unsigned int dp_value, const array<snes_line> &lines)
{
	return lines.fuzzy_find(start, use_value(dp_value, all_of(sta, memory, direct, value))).get_or(lines.length());
}

template <typename T, typename F>
array<snes_line> basic_optimization_iterator(const array<snes_line> &lines, const T &comparators, const F &codegen)
{
	array<snes_line> optimized;
	int previous = 0;

	for(auto i = lines.fuzzy_find(0, comparators); i; i = lines.fuzzy_find(i + 1, comparators)){
		optimized += lines.slice(previous, i);
		previous = i + comparators.size;
		optimized += codegen(i);
	}
	return optimized.append(lines.slice(previous, lines.length()));
}

array<snes_line> redundant_load_store_optimize(const array<snes_line> &lines)
{
	//this optimization looks for dp and optimizes redundant loads and stores to it, does not handle indirects
	array<snes_line> optimized;
	array<int> skips;
	int previous = 0;

	//possibly add branch skips if this turns out to be a common flaw
	const auto accumulator_uses = [](const snes_line &line){
		return passive_instruction(line) && !all_of(lda, memory, direct, value)(line);
	};
	
	tuple comparators(
			all_of(get_dp_value, sta, direct), 
			width, 
			all_of(lda, memory, direct, value)
		);
	
	for(auto i = lines.fuzzy_find(0, comparators); i; i = lines.fuzzy_find(i + 1, comparators)){
		optimized += blacklist_copy(lines, skips, previous, i);
		previous = i + 3;
		int register_store = find_register_store(previous, state.value, lines);
		
		auto potential_accumulator_use = lines.fuzzy_find(previous, register_store, accumulator_uses);
		auto pending_skips = lines.fuzzy_find_all(previous, register_store, all_of(lda, memory, direct, value));
		
		if(pending_skips.length() && max(pending_skips) > potential_accumulator_use.get_or(lines.length())){
			optimized += lines[i];	//store will be needed still
		}else{
			skips.append(pending_skips);
		}
	}
	optimized += blacklist_copy(lines, skips, previous, lines.length());
	return optimized;
}

array<snes_line> adc_to_inc_optimize(const array<snes_line> &lines)
{
	//this should cover the prefixed increment case specifically
	//todo look into postfix case
	assertf(lines.length() > 4, "length too short"); //I don't think that is even possible with gcc output

	return basic_optimization_iterator(lines,
		tuple(
			all_of(lda, memory, get_mode, get_value),
			use_alt_state(all_of(sta, memory, get_mode, get_value)),
			clc,
			all_of(adc, constant, one_of(use_value(1, value), use_value(2, value))),
			all_of(sta, memory, mode, value),
			width,
			use_alt_state(all_of(lda, memory, mode, value))
		),
		[&lines](int i){
			auto inc = get_instruction("inc");
			inc.create_operand() = lines[i].operand(0);
			
			array<snes_line> optimized = {lines[i], inc};
			
			if(lines[i + 3].operand(0).value == 2){
				optimized += inc;
			}
			return optimized;
		}
	);
}

array<snes_line> redundant_bit_optimize(const array<snes_line> &lines)
{
	assertf(lines.length() > 4, "length too short"); //I don't think that is even possible with gcc output
	
	//todo verify codegen never requires the initial store
	return basic_optimization_iterator(lines,
		tuple(
			all_of(get_dp_value, sta, memory, direct), 
			width, 
			use_alt_state(all_of(lda, get_value, get_mode, memory)),
			all_of(bit_logic, value, memory, direct)
		),
		[&lines](int i){
			return array<snes_line>{
				lines[i + 1],
				get_memory_instruction(lines[i + 3].mnemonic(), alt_state.mode, alt_state.value)
			};
		}
	);
}

array<snes_line> bitshift_optimize(const array<snes_line> &lines)
{	
	return basic_optimization_iterator(lines,
		tuple(
			all_of(get_dp_value, sta, memory, direct), 
			width, 
			all_of(bit_shift, value, memory, direct)
		),
		[&lines](int i){
			return array<snes_line>{
				get_instruction(lines[i + 2].mnemonic()), 
				lines[i + 1], 
				lines[i]
			};
		}
	);
}



array<snes_line> nonlocal_adc_to_shift_optimize(const array<snes_line> &lines)
{
	return basic_optimization_iterator(lines,
		tuple(
			all_of(get_dp_value, lda, memory, direct), 
			clc, 
			all_of(adc, value, memory, direct)
		),
		[&lines](int i){
			return array<snes_line>{
				lines[i],
				get_instruction("asl")
			};
		}
	);
}

array<snes_line> adc_to_shift_optimize(const array<snes_line> &lines)
{
	return nonlocal_adc_to_shift_optimize(basic_optimization_iterator(lines,
		tuple(
			all_of(get_dp_value, sta, memory, direct), 
			clc, 
			width, 
			all_of(lda, value, memory, direct), 
			all_of(adc, value, memory, direct), 
			all_of(sta, value, memory, direct)
		),
		[&lines](int i){
			return array<snes_line>{
				lines[i + 2],
				get_instruction("asl"),
				lines[i + 5]
			};
		}
	));
}

array<snes_line> optimize(const array<snes_line> &lines)
{
	array<snes_line> optimized;
	
	optimized = optimizations.redundant_load_store ? redundant_load_store_optimize(lines) : lines;
	optimized = optimizations.adc_to_inc ? adc_to_inc_optimize(optimized) : optimized;
	optimized = optimizations.adc_to_shift ? adc_to_shift_optimize(optimized) : optimized;
	optimized = optimizations.bitshift ? bitshift_optimize(optimized) : optimized;
	optimized = optimizations.redundant_bit ? redundant_bit_optimize(optimized) : optimized;
	
	//rerun load store optimize to cleanup some of the new opportunities left by other optimizations
	optimized = optimizations.redundant_load_store ? redundant_load_store_optimize(optimized) : optimized;
	return optimized;
}
