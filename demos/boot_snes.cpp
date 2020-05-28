#include <cstdint>
#include "mmio.h"
namespace {
volatile uint8_t &mmio(const uint16_t loc)
{
    return *reinterpret_cast<uint8_t *>(loc);
}

volatile uint16_t &mmio_16(const uint16_t loc)
{
    return *reinterpret_cast<uint16_t *>(loc);
}
//dummy is a temporary construct until I have a better way to read
//the nmi flag without a write
uint8_t * const dummy = reinterpret_cast<uint8_t *>(0x0040);
uint16_t * const frame_counter = reinterpret_cast<uint16_t *>(0x0050);

uint8_t * const controllers = reinterpret_cast<uint8_t *>(0x1000);

uint8_t * const str_dest = reinterpret_cast<uint8_t *>(0x1010);


void read_controller()
{
	mmio_16(0x4016) = 1;
	mmio_16(0x4016) = 0;
	for(uint8_t i = 0; i < 2; i++){
		for(uint8_t j = 0; j < 8; j++){
			uint8_t port0 = (joypad->port_0 & (uint8_t)0x03);
			controllers[i+0] = (controllers[i+0] << 1) | port0 & 1;
			controllers[i+4] = (controllers[i+4] << 1) | (port0 >> 1);
			uint8_t port1 = (joypad->port_1 & (uint8_t)0x03);
			controllers[i+2] = (controllers[i+2] << 1) | port1 & 1;
			controllers[i+6] = (controllers[i+6] << 1) | (port1 >> 1);
		}
	}
}

void DMA_VRAM(const uint8_t *src, const uint16_t dest, uint16_t length, uint8_t mode)
{
	PPU->vram_control = 0x80;
	PPU->vram_address = dest;
	DMA0->control = mode;
	DMA0->destination = 0x18;
	DMA0->source_word = (uint16_t)(uint32_t)src;
	DMA0->source_bank = 0;
	DMA0->size = length;
	
	CPU->enable_dma = 1;
} 

}

int main()
{
	*frame_counter = 0;
	PPU->screen = 0x80;
	CPU->enable_interrupts = 0x80;
	const char *test = "qwertyuiopasdfghjklzxcvbnm";
	for(uint8_t i = 0; i < 26; i++){
		mmio(0x5010) = test[i];
		str_dest[i] = test[i];
	}
	
	while(controllers[0] != 0x80){
		//5010 = debug mmio
		for(uint8_t i = 0; i < 8; i++){
			controllers[i] = 0;
		}
		read_controller();
		switch((*frame_counter)&1){
			case 0:
				mmio(0x5010) = controllers[0];
			break;
			case 1:
				mmio(0x5010) = controllers[1];
			break;
		}
	}
	
	DMA_VRAM((const uint8_t*)"a", 0x0, 0x0, 0x9);
}

void nmi()
{
	*dummy = CPU->nmi_flag;
	mmio_16(0x5010) = (*frame_counter)++;
}
