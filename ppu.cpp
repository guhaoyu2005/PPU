#include <iostream>
#include "ppu.hpp"

using namespace std;

PPU::PPU() {
}

PPU::~PPU() {
}

void PPU::cycle() {

}

// reference
// https://wiki.nesdev.com/w/index.php/PPU_nametables
uint8_t PPU::mem_access(uint16_t addr, uint8_t val, bool rw) {
		switch (addr) {
				case 0x0000 ... 0x1fff: { /* read cartridge here */ }
				case 0x2000 ... 0x2fff: {
					if (rw)
							nametable_mem[addr - nametable_mem_base_addr] = val;
					return nametable_mem[addr - nametable_mem_base_addr]; 
				}
				case 0x3000 ... 0x3eff: { 
						uint16_t naddr = addr - nametable_mem_base_addr;
						if (mirroring_mode == PPU_MIRRORING_HORIZONTAL) {
								naddr = naddr - 0x800;
						} else if (mirroring_mode == PPU_MIRRORING_VERTICAL) {
								naddr = naddr - 0x400;
						}
						if (rw)
								nametable_mem[naddr] = val;
						return nametable_mem[naddr];
				}
				case 0x3f00 ... 0x3fff: {
						// Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
						uint16_t naddr = addr;
						if ((naddr & 0x13) == 0x10) naddr -= 0x10;
						naddr = (naddr - palette_mem_base_addr) & 0x1f;
						uint8_t grayscale = (ppu_ext_registers[PPUMASK] >> R1_G0_S) & R1_G0_M;
						if (rw)
								palette_mem[naddr]  = val;
						return palette_mem[naddr] & (grayscale ? 0x30: 0xff); 
				}
		}
		return 0;
}

// reference 
// https://wiki.nesdev.com/w/index.php/PPU_scrolling#.242005_second_write_.28w_is_1.29
uint8_t PPU::access_ppu_ext_register(int reg, uint8_t s, uint8_t m, uint8_t val, bool rw) {
		if (rw) {
				switch (reg) {
						case 0:
						case 1: { 
							ppu_ext_registers[reg] = (ppu_ext_registers[reg] & (~(1<<s))) | (val<<s); 
							t_addr.nt_addr =  (ppu_ext_registers[reg] >> R0_I_S) & R0_I_M;
							break;
						}
						case 3: { oam_addr = val; break; }
						case 4: { oam_mem[oam_addr++] = val; break; }
						case 5: {
								if (!w)	{ 
									t_addr.coarse_x = val >> 3; 
									fine_x = val & 0x7; 
								}
								else { 
									t_addr.coarse_y = val >> 3; 
									t_addr.fine_y = val & 0x7; 
								}
								w = !w;
								break;
						}
						case 6: {
								if (!w) { 
									t_addr.high_addr = val & 0x3f; 
								}
								else { 
									t_addr.low_addr = val; 
									v_addr.reg = t_addr.reg; 
								}
								w = !w;
								break;
						}
						case 7: {
								mem_write(v_addr.addr, val);
								v_addr.addr += (((ppu_ext_registers[PPUCTRL] >> R0_I_S) & R0_I_M) ? COARSE_Y_SCROLL: COARSE_X_SCROLL);
						}
				}
		}
		else {
				switch (reg) {
						case 2: {
								w = false;
								ppu_ext_registers[PPUSTATUS] = (ppu_ext_registers[PPUSTATUS] & (~(1<<R2_V_S))) | (0<<R2_V_M);
								return (ppu_ext_registers[reg] >> s) & m;
						}
						case 4: { return oam_mem[oam_addr]; }
						case 7: {
								uint8_t ret = internal_read_buffer;
								internal_read_buffer = mem_read(v_addr.addr);
								if (v_addr.addr > 0x3eff) { ret = internal_read_buffer; }
								v_addr.addr += (((ppu_ext_registers[PPUCTRL] >> R0_I_S) & R0_I_M) ? COARSE_Y_SCROLL: COARSE_X_SCROLL);
								return ret;
						}
				}
		}

		return 1;
}

void PPU::oam_clear() {
		for (uint8_t i=0;i<0x20;i++) {
			sec_oam[i] = 255;
			sprite_chosen[i/4] = false;
		}
}

inline int PPU::sprite_height() {
		return (((ppu_ext_registers[PPUCTRL] >> R0_H_S) & R0_H_M) ? 16: 8);
}

uint8_t PPU::oam_get(uint8_t *oam, uint8_t idx, OAM_SPRITE p) {
		switch (p) {
				case OAM_SPRITE_X: { return oam[idx*4+3]; }
				case OAM_SPRITE_Y: { return oam[idx*4]; }
				case OAM_SPRITE_TILES_BANK: { return oam[idx*4+1] & 0x1; }
				case OAM_SPRITE_TILES_NUM_TOP_SPRITE: { return oam[idx*4+1] & (~0x1);}
				case OAM_SPRITE_PALETTE: { return oam[idx*4+2] & 0x3; }
				case OAM_SPRITE_PRIORITY: { return (oam[idx*4+2]>>5) & 0x1; }
				case OAM_SPRITE_FLIP_H: { return (oam[idx*4+2]>>6) & 0x1; }
				case OAM_SPRITE_FLIP_V: { return (oam[idx*4+2]>>7) & 0x1; }
				case OAM_BYTE_1: { return oam[idx*4+1]; }
				/*
				case OAM_SPRITE_TILE_LOW:
				case OAM_SPRITE_TILE_HIGH: {
						uint16_t addr;
						if (sprite_height() == 16) 
								addr = (oam[idx*4+1] & 0x1) * 0x1000 + (oam[idx*4+1] & (~0x1)) * 0x10;
						else
								addr = ((ppu_ext_registers[PPUCTRL] >> R0_S_S) & R0_S_M) * 0x1000 + oam[idx*4+1] * 0x10;


				}
				*/
		}
		cout<<"OAM_GET PROBLEM!!!!!!"<<endl;
		return 0;
}

void PPU::scanline_cycle(SCANLINE_TYPE t) {
		if (t == SCANLINE_TYPE_POST_RENDER && dot == 0) {
				// trigger GUI to draw output
		}
		else if (t == SCANLINE_TYPE_VBLANK && dot == 1) {
				 ppu_ext_registers[PPUSTATUS] = (ppu_ext_registers[PPUSTATUS] & (~(1<<R2_V_S))) | (1<<R2_V_S); 
				 if ((ppu_ext_registers[PPUCTRL] >> R0_V_S) & R0_V_M) {
						 // call cpu interrupt
				 }
		}
		else if (t == SCANLINE_TYPE_PRE_RENDER || t == SCANLINE_TYPE_VISIBLE) {
				switch (dot) {
						case 1: {
								
								break;
						}
						case 257: {
								// evaluate sprite
								break;
						}
				}
		}
}
