#include "ppu.hpp"


PPU::PPU() {
}

PPU::~PPU() {
}

void PPU::cycle() {

}

uint8_t PPU::access_ppu_register(int reg, uint8_t s, uint8_t m, uint8_t val, bool rw) {
		static bool latch;

		if (rw) {
				switch (reg) {
						case 0 ... 2: {
								ppu_ext_registers[reg] = (ppu_ext_registers[reg] & (!(1<<s))) | (val<<s);
								break;
						}
						case 3: {
								oam_addr = val;
								break;
						}
						case 4: {
								oam_mem[oam_addr++] = val;
								break;
						}
						case 5: {
								if (!latch)	{
										
								}
								else {

								}
								latch = !latch;
								break;
						}
						case 6: {
								break;
						}
				}
		}
		else {
				switch (reg) {
						case 0 ... 2: {
								return (ppu_ext_registers[reg] >> s) & m;
								break;
						}
				}
		}

		return 1;
}
