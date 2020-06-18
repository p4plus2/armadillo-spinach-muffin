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
		return line.operands.length() && state.value < 0x100;
	};
	
	const auto get_value = [](const snes_line &line){
		state.label = line.operands.length() ? line.operand(0).label : "";
		state.value = line.operands.length() ? line.operand(0).value : 0;
		return line.operands.length();
	};
	
	const auto value = [](const snes_line &line){
		return line.operands.length() && 
		line.operand(0).value == state.value && line.operand(0).label == state.label;
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
	
	const auto index_constant = [](const snes_line &line){
		return line.operands.length() && line.operand(0).type == INDEX_CONSTANT;
	};
	
	const auto direct = [](const snes_line &line){
		return line.operands.length() && line.operand(0).mode == DIRECT;
	};
	
	const auto indirect = [](const snes_line &line){
		return line.operands.length() && line.operand(0).mode == INDIRECT;
	};
	
	const auto get_register = [](const snes_line &line){
		state.value = line.operands.length() ? line.operand(0).value : 0;
		return line.operands.length() && state.value < 0x20;	//max register address
	};
	
	const auto label = [](const snes_line &line){
		return line.type == LABEL;
	};
	
	const auto sta = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "sta");
	};
	
	const auto width = [](const snes_line &line){
		return line.matches(DIRECTIVE, 1, ".width");
	};
	
	const auto ldy = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "ldy");
	};
	
	const auto lda = [](const snes_line &line){
		return line.matches(INSTRUCTION, 1, "lda");
	};
	
	const auto tdc = [](const snes_line &line){
		return line.matches(INSTRUCTION, 0, "tdc");
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

maybe<int> find_register_use(int start, unsigned int dp_value, const array<snes_line> &lines)
{
	return lines.fuzzy_find(start, use_value(dp_value, all_of(memory, direct, value)));
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

template <typename T, typename F>
array<snes_line> eviction_optimization_iterator(const array<snes_line> &lines, const T &comparators, const F &codegen)
{
	//this optimization looks for dp and optimizes redundant loads and stores to it, does not handle indirects
	array<snes_line> optimized;
	array<int> skips;
	int previous = 0;

	for(auto i = lines.fuzzy_find(0, comparators); i; i = lines.fuzzy_find(i + 1, comparators)){
		optimized += blacklist_copy(lines, skips, previous, i);
		previous = i + comparators.size;
		auto [code, potential_skips] = codegen(i);
		optimized += code;
		skips += potential_skips;
	}
	optimized += blacklist_copy(lines, skips, previous, lines.length());
	return optimized;
}

array<snes_line> redundant_load_store_optimize(const array<snes_line> &lines)
{
	//this optimization looks for dp and optimizes redundant loads and stores to it, does not handle indirects
	//possibly add branch skips if this turns out to be a common flaw
	const auto accumulator_uses = [](const snes_line &line){
		return label(line) || (passive_instruction(line) && !all_of(lda, memory, direct, value)(line));
	};
	return eviction_optimization_iterator(lines,
		tuple(
			all_of(sta, memory, direct, get_dp_value), 
			width, 
			all_of(lda, memory, direct, value)
		),
		[&lines, &accumulator_uses](int i){
			int register_store = find_register_store(i + 3, state.value, lines);
			auto potential_accumulator_use = lines.fuzzy_find(i + 3, register_store, accumulator_uses);
			auto skips = lines.fuzzy_find_all(i + 3, register_store, all_of(lda, memory, direct, value));
			
			if(skips.length() && max(skips) > potential_accumulator_use.get_or(lines.length())){
				return pair(array<snes_line>({lines[i]}), array<int>());
			}else{
				return pair(array<snes_line>(), skips);
			}
		}
	);
}

array<snes_line> adc_to_inc_optimize(const array<snes_line> &lines)
{
	//this should cover the postfixed increment case specifically
	//todo look into prefix case
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

array<snes_line> redundant_unary_reload_optimize(const array<snes_line> &lines)
{
	return basic_optimization_iterator(lines,
		tuple(
			all_of(sta, get_value, memory, get_mode), 
			width, 
			all_of(bit_shift, memory),
			width, 
			all_of(lda, value, memory, mode)
		),
		[&lines](int i){
			return array<snes_line>{
				lines[i],
				lines[i + 1],
				lines[i + 2],
				lines[i + 3]
			};
		}
	);
}

array<snes_line> redundant_bit_optimize(const array<snes_line> &lines)
{
	//todo verify codegen never requires the initial store
	return basic_optimization_iterator(lines,
		tuple(
			all_of(sta, memory, direct, get_dp_value), 
			width, 
			use_alt_state(all_of(lda, memory, direct, get_value)),
			all_of(bit_logic, memory, direct, value)
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
			all_of(sta, memory, direct, get_dp_value), 
			width, 
			all_of(bit_shift, memory, direct, value)
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
			all_of(lda, memory, direct, get_dp_value), 
			clc, 
			all_of(adc, memory, direct, value)
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
			all_of(sta, get_dp_value, memory, direct), 
			clc, 
			width, 
			all_of(lda, memory, direct, value), 
			all_of(adc, memory, direct, value), 
			all_of(sta, memory, direct, value)
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

array<snes_line> delay_load_optimize(const array<snes_line> &lines)
{
	//this optimization looks for addresses which are loaded then stored then tries to delay
	//the load until the first time the result is used.  This optimization will only trigger
	//when the width of both the original load and the replacement match to avoid rep/sep
	//overhead
	array<snes_line> optimized;
	array<pair<int, int>> replacements;
	int previous = 0;

	tuple comparators(
			width,
			all_of(lda),
			all_of(sta, get_register)
		);
		
	const auto replacement_slice = [&](int offset, int bound){
		for(const auto &[i, replacement] : replacements){
			if(i < offset){
				continue;
			}else if(i < bound){
				optimized.append(lines.slice(offset, i));
				optimized += lines[replacement];
				optimized += lines[replacement + 1];
				optimized += lines[replacement + 2];
				optimized += lines[i - 1];
				offset = i + 1;				
			}
		}
		optimized.append(lines.slice(offset, bound));
	};
	
	for(auto i = lines.fuzzy_find(0, comparators); i; i = lines.fuzzy_find(i + 1, comparators)){
		replacement_slice(previous, i);
		previous = i + 3;
		
		bool delay_load = false;
		auto register_use = find_register_use(previous, state.value, lines);
		if(register_use && lda(lines[register_use]) && width(lines[register_use - 1]) &&
		lines[register_use - 1].operand(0).width == lines[i].operand(0).width){
			delay_load = true;
			for(int j = previous; j < register_use; j++){
				if(label(lines[j])){
					delay_load = false;
					break;
				}
			}
		}
		
		if(lines.fuzzy_find(previous, passive_instruction) < lines.fuzzy_find(previous, one_of(lda, tdc))){
			delay_load = false;
		}
		
		if(delay_load){
			replacements.append({register_use, i});
		}else{
			previous -= 3;
		}
	}
	replacement_slice(previous, lines.length());
	return optimized;
}

array<snes_line> redundant_index_constant_optimize(const array<snes_line> &lines)
{
	return eviction_optimization_iterator(lines,
		tuple(
			all_of(ldy, index_constant, get_value)
		),
		[&lines](int i){
			array<int> removable_ldy;
			for(int j = i + 1; j < lines.length(); j++){
				if(all_of(ldy, index_constant, value)(lines[j])){
					removable_ldy += j;
				}else if(ldy(lines[j]) || label(lines[j])){
					break;
				}
			}
			if(removable_ldy.length()){
				return pair(array<snes_line>({lines[i]}), removable_ldy);
			}else{
				return pair(array<snes_line>(), removable_ldy);
			}
		}
	);
}


array<snes_line> optimize(const array<snes_line> &lines)
{
	array<snes_line> optimized;
	
	optimized = optimizations.redundant_load_store ? redundant_load_store_optimize(lines) : lines;
	optimized = optimizations.adc_to_inc ? adc_to_inc_optimize(optimized) : optimized;
	optimized = optimizations.adc_to_shift ? adc_to_shift_optimize(optimized) : optimized;
	optimized = optimizations.bitshift ? bitshift_optimize(optimized) : optimized;
	optimized = optimizations.redundant_bit ? redundant_bit_optimize(optimized) : optimized;
	optimized = optimizations.redundant_unary_reload ? redundant_unary_reload_optimize(optimized) : optimized;
	optimized = optimizations.delay_load ? delay_load_optimize(optimized) : optimized;
	optimized = optimizations.redundant_index_constant ? redundant_index_constant_optimize(optimized) : optimized;
	
	//rerun load store optimize to cleanup some of the new opportunities left by other optimizations
	optimized = optimizations.redundant_load_store ? redundant_load_store_optimize(optimized) : optimized;
	return optimized;
}


