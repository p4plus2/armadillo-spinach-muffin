#pragma once
#include "p4lib/array.h"
#include "snes_instructions.h"


inline struct{
	bool redundant_load_store = true;
	bool adc_to_inc = true;
	bool adc_to_shift = true;
	bool bitshift = true;
	bool redundant_bit = true;
}optimizations;

array<snes_line> optimize(const array<snes_line> &lines);
