#pragma once
#define PACKED __attribute__((packed))

struct PPU_mmio{
	uint8_t screen;
	
	uint8_t sprite_select;
	union{
		uint16_t oam_address;
		struct{
			uint8_t oam_address_low;
			uint8_t oam_address_high;
		}PACKED;
	}PACKED;
	uint8_t oam_write;
	
	uint8_t layer_mode;
	uint8_t mosaic;
	
	union{
		uint16_t layer_0_1_tilemap_select;
		struct{
			uint8_t layer_0_tilemap_select;
			uint8_t layer_1_tilemap_select;
		}PACKED;
	}PACKED;
	union{
		uint16_t layer_2_3_tilemap_select;
		struct{
			uint8_t layer_2_tilemap_select;
			uint8_t layer_3_tilemap_select;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t layer_all_tiledata_select;
		struct{
			uint8_t layer_0_1_tiledata_select;
			uint8_t layer_2_3_tiledata_select;
		}PACKED;
	}PACKED;
	
	uint8_t layer_0_scroll_x;
	uint8_t layer_0_scroll_y;
	uint8_t layer_1_scroll_x;
	uint8_t layer_1_scroll_y;
	uint8_t layer_2_scroll_x;
	uint8_t layer_2_scroll_y;
	uint8_t layer_3_scroll_x;
	uint8_t layer_3_scroll_y;
	
	uint8_t vram_control;
	union{
		uint16_t vram_address;
		struct{
			uint8_t vram_address_low;
			uint8_t vram_address_high;
		}PACKED;
	}PACKED;
	union{
		uint16_t vram_write;
		struct{
			uint8_t vram_write_low;
			uint8_t vram_write_high;
		}PACKED;
	}PACKED;
		
	uint8_t mode_7_settings;
	union{
		uint8_t fixed_point_mul_A;
		uint8_t mode_7_A;	
	}PACKED;
	union{
		uint8_t fixed_point_mul_B;
		uint8_t mode_7_B;	
	}PACKED;
	uint8_t mode_7_C;	
	uint8_t mode_7_D;	
	uint8_t mode_7_center_x;
	uint8_t mode_7_center_y;
	
	uint8_t cgram_address;
	uint8_t cgram_write;
	
	union{
		uint16_t window_layer_all_settings;
		struct{
			uint8_t window_layer_0_1_settings;
			uint8_t window_layer_2_3_settings;
		}PACKED;
	}PACKED;
	uint8_t window_sprite_color_settings;
	union{
		uint16_t window_1;
		struct{
			uint8_t window_1_left;
			uint8_t window_1_right;
		}PACKED;
	}PACKED;
	union{
		uint16_t window_2;
		struct{
			uint8_t window_2_left;
			uint8_t window_2_right;
		}PACKED;
	}PACKED;			
	union{
		uint16_t window_logic;
		struct{
			uint8_t window_layer_logic;
			uint8_t window_sprite_color_logic;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t screens;
		struct{
			uint8_t main_screen;
			uint8_t sub_screen;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t window_masks;
		struct{
			uint8_t window_mask_main_screen;
			uint8_t window_mask_sub_screen;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t color_math;
		struct{
			uint8_t color_addition_logic;
			uint8_t color_math_settings;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t display_control;
		struct{
			uint8_t fixed_color;
			uint8_t video_mode;
		}PACKED;
	}PACKED;
	
	union{
		uint16_t multiply_result_word;
		struct{
			uint8_t multiply_result_low;
			uint8_t multiply_result_mid;
		}PACKED;
	}PACKED;
	uint8_t multiply_result_high;
		
	uint8_t latch;
	
	uint8_t oam_read;
	
	union{
		uint16_t vram_read;
		struct{
			uint8_t vram_read_low;
			uint8_t vram_read_high;
		}PACKED;
	}PACKED;
		
	uint8_t cgram_read;
	
	uint8_t horizontal_scanline;
	uint8_t vertical_scanline;
	
	union{
		uint16_t status;
		struct{
			uint8_t status_ppu1;
			uint8_t status_ppu2;
		}PACKED;
	}PACKED;
}PACKED;

struct APU_mmio{
	uint8_t IO1;
	uint8_t IO2;
	uint8_t IO3;
	uint8_t IO4;
}PACKED;

struct WRAM_mmio{
	uint8_t data;
	union{
		uint16_t word;
		struct{
			uint8_t low;
			uint8_t mid;
		}PACKED;
	}PACKED;
	uint8_t high;
}PACKED;

struct joypad_mmio{
	uint8_t port_0;
	uint8_t port_1;
}PACKED;


struct CPU_mmio{
	uint8_t enable_interrupts;
	uint8_t output_port;
	union{
		uint16_t multiply;
		struct{
			uint8_t multiply_A;
			uint8_t multiply_B;
		}PACKED;
	}PACKED;
	union{
		uint16_t dividen;
		struct{
			uint8_t dividen_low;
			uint8_t dividen_high;
		}PACKED;
	}PACKED;
	uint8_t divisor;
	
	union{
		uint16_t horizontal_timer;
		struct{
			uint8_t horizontal_timer_low;
			uint8_t horizontal_timer_high;
		}PACKED;
	}PACKED;
		
	union{
		uint16_t vertical_timer;
		struct{
			uint8_t vertical_timer_low;
			uint8_t vertical_timer_high;
		}PACKED;
	}PACKED;
		
	union{
		uint16_t enable_dma_hdma;
		struct{
			uint8_t enable_dma;
			uint8_t enable_hdma;
		}PACKED;
	}PACKED;
	
	uint8_t rom_speed;
	
	uint16_t openbus;
	
	uint8_t nmi_flag;
	uint8_t irq_flag;
	uint8_t ppu_status;
	
	uint8_t input_port;
	
	union{
		uint16_t divide_result;
		struct{
			uint8_t divide_result_low;
			uint8_t divide_result_high;
		}PACKED;
	}PACKED;
	
	struct{
		union{
			uint16_t multiply_result;
			uint16_t divide_remainder;
		}PACKED;
		struct{
			union{
				uint8_t multiply_result_low;
				uint8_t divide_remainder_low;
			}PACKED;
			union{
				uint8_t multiply_result_high;
				uint8_t divide_remainder_high;
			}PACKED;
		}PACKED;
	}PACKED;
		
	union{
		uint16_t port_0_data_1;
		struct{
			uint8_t port_0_data_1_low;
			uint8_t port_0_data_1_high;
		}PACKED;
	}PACKED;
	union{
		uint16_t port_1_data_1;
		struct{
			uint8_t port_1_data_1_low;
			uint8_t port_1_data_1_high;
		}PACKED;
	}PACKED;
	union{
		uint16_t port_0_data_2;
		struct{
			uint8_t port_0_data_2_low;
			uint8_t port_0_data_2_high;
		}PACKED;
	}PACKED;
	union{
		uint16_t port_1_data_2;
		struct{
			uint8_t port_1_data_2_low;
			uint8_t port_1_data_2_high;
		}PACKED;
	}PACKED;
}PACKED;

struct DMA_mmio{
	union{
		uint16_t settings;
		struct{
			uint8_t control;
			uint8_t destination;
		}PACKED;
	}PACKED;
	union{
		uint16_t source_word;
		struct{
			uint8_t source_word_low;
			uint8_t source_word_high;
		}PACKED;
	}PACKED;
	uint8_t source_bank;
	union{
		uint16_t size;
		struct{
			uint8_t size_low;
			uint8_t size_high;
		}PACKED;
	}PACKED;
}PACKED;

struct HDMA_mmio{
	union{
		uint16_t settings;
		struct{
			uint8_t control;
			uint8_t destination;
		}PACKED;
	}PACKED;
	union{
		uint16_t source_word;
		struct{
			uint8_t source_word_low;
			uint8_t source_word_high;
		}PACKED;
	}PACKED;
	uint8_t source_bank;
	union{
		uint16_t indirect_source_word;
		struct{
			uint8_t indirect_source_word_low;
			uint8_t indirect_source_word_high;
		}PACKED;
	}PACKED;
	uint8_t indirect_source_bank;
	union{
		uint16_t midframe_table_word;
		struct{
			uint8_t midframe_table_low;
			uint8_t midframe_table_high;
		}PACKED;
	}PACKED;
	uint8_t counter;
}PACKED;

inline volatile joypad_mmio * const joypad = reinterpret_cast<joypad_mmio *>(0x4016);
inline volatile WRAM_mmio * const WRAM = reinterpret_cast<WRAM_mmio *>(0x2180);

inline volatile PPU_mmio * const PPU = reinterpret_cast<PPU_mmio *>(0x2100);
inline volatile APU_mmio * const APU = reinterpret_cast<APU_mmio *>(0x2140);
inline volatile CPU_mmio * const CPU = reinterpret_cast<CPU_mmio *>(0x4200);

inline volatile DMA_mmio * const DMA0 = reinterpret_cast<DMA_mmio *>(0x4300);
inline volatile DMA_mmio * const DMA1 = reinterpret_cast<DMA_mmio *>(0x4310);
inline volatile DMA_mmio * const DMA2 = reinterpret_cast<DMA_mmio *>(0x4320);
inline volatile DMA_mmio * const DMA3 = reinterpret_cast<DMA_mmio *>(0x4330);
inline volatile DMA_mmio * const DMA4 = reinterpret_cast<DMA_mmio *>(0x4340);
inline volatile DMA_mmio * const DMA5 = reinterpret_cast<DMA_mmio *>(0x4350);
inline volatile DMA_mmio * const DMA6 = reinterpret_cast<DMA_mmio *>(0x4360);
inline volatile DMA_mmio * const DMA7 = reinterpret_cast<DMA_mmio *>(0x4370);


inline volatile HDMA_mmio * const HDMA0 = reinterpret_cast<HDMA_mmio *>(0x4300);
inline volatile HDMA_mmio * const HDMA1 = reinterpret_cast<HDMA_mmio *>(0x4310);
inline volatile HDMA_mmio * const HDMA2 = reinterpret_cast<HDMA_mmio *>(0x4320);
inline volatile HDMA_mmio * const HDMA3 = reinterpret_cast<HDMA_mmio *>(0x4330);
inline volatile HDMA_mmio * const HDMA4 = reinterpret_cast<HDMA_mmio *>(0x4340);
inline volatile HDMA_mmio * const HDMA5 = reinterpret_cast<HDMA_mmio *>(0x4350);
inline volatile HDMA_mmio * const HDMA6 = reinterpret_cast<HDMA_mmio *>(0x4360);
inline volatile HDMA_mmio * const HDMA7 = reinterpret_cast<HDMA_mmio *>(0x4370);
