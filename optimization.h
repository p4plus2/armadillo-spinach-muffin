#pragma once
#include "p4lib/array.h"
#include "snes_instructions.h"


inline struct{
	bool redundant_load_store = true;
	bool adc_to_inc = true;
	bool adc_to_shift = true;
	bool bitshift = true;
	bool redundant_bit = true;
	bool redundant_unary_reload = true;
	bool delay_load = true;
	bool redundant_index_constant = true;
}optimizations;

array<snes_line> optimize(const array<snes_line> &lines);
