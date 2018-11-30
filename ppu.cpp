#include <iostream>
#include "ppu.hpp"

using namespace std;

PPU::PPU() {
		reset();
}

PPU::~PPU() {
}

void PPU::reset() {
		scanline = 0;
		dot = 0;
		odd_frame = false;
		ppu_ext_registers[0] = 0;
		ppu_ext_registers[1] = 0;
		ppu_ext_registers[2] = 0;
		ppu_ext_registers[5] = 0;
		ppu_ext_registers[7] = 0;

		memset(output, 0, sizeof(output));
		//memset(nametable_mem, 255 ,sizeof(nametable_mem));
		memset(oam_mem, 0, sizeof(oam_mem));
		memset(sprite_chosen, 0, sizeof(sprite_chosen));
		memset(sprite_id, -1, sizeof(sprite_id));
		memset(sec_sprite_chosen, 0, sizeof(sec_sprite_chosen));
		memset(sec_sprite_id, -1, sizeof(sec_sprite_id));
}

void PPU::cycle() {
		switch (scanline) {
				case 0 ... 239:{ scanline_cycle(SCANLINE_TYPE_VISIBLE); break; }
				case 240: { scanline_cycle(SCANLINE_TYPE_POST_RENDER); break; }
				case 241: { scanline_cycle(SCANLINE_TYPE_VBLANK); break; }
				case 260: { scanline_cycle(SCANLINE_TYPE_PRE_RENDER); break; }
		}
		if (++dot > 340) {
				dot %= 340;
				if (++scanline > 261) {
						scanline = 0;
						odd_frame ^= 1;
				}
		}
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
uint8_t PPU::access_ppu_ext_register(int reg, uint8_t val, bool rw) {
		if (rw) {
				switch (reg) {
						case 0:
						case 1: { 
							ppu_ext_registers[reg] = val; 
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
								return ppu_ext_registers[reg];
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
			sprite_id[i/4] = -1;
			sec_sprite_chosen[i/4] = false;
			sec_sprite_id[i/4] = -1;
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
				case OAM_SPRITE_TILE_LOW:
				case OAM_SPRITE_TILE_HIGH: {
						uint16_t addr;
						if (sprite_height() == 16) 
								addr = (oam[idx*4+1] & 0x1) * 0x1000 + (oam[idx*4+1] & (~0x1)) * 0x10;
						else
								addr = ((ppu_ext_registers[PPUCTRL] >> R0_S_S) & R0_S_M) * 0x1000 + oam[idx*4+1] * 0x10;
						uint16_t sprite_y = (scanline - oam_get(oam, idx, OAM_SPRITE_Y)) % sprite_height();
						if (oam_get(oam, idx, OAM_SPRITE_FLIP_V))
								sprite_y ^= (sprite_height()-1);
						addr += sprite_y + (sprite_y & 0x8);

						if (p == OAM_SPRITE_TILE_HIGH)
								addr += 8;
						return mem_read(addr);
				}
		}
		cout<<"OAM_GET PROBLEM!!!!!!"<<endl;
		return 0;
}

inline bool PPU::show_sprite_or_bkg() {
		return (ppu_ext_registers[PPUMASK] >> R1_b_S & 0x3) > 0; 
}

void PPU::draw() {
		uint8_t palette = 0;
		uint8_t spalette = 0;
		bool obj_priority = 0;
		if (scanline < 240 && dot >=2 && dot < 258) {
				if (((ppu_ext_registers[PPUMASK] >> R1_b_S) & 0x1) && !(!((ppu_ext_registers[PPUMASK] >> R1_m_S) & 0x1)) && dot < 10) {
						palette = (((bm_shift_high >> (15-fine_x)) & 0x1) << 1) | (((bm_shift_low >> (15-fine_x)) & 0x1) << 2);
						if (palette)
								palette |= (((attr_shift_high >> (7-fine_x)) & 0x1) << 1) | (((attr_shift_low >> (7-fine_x)) & 0x1) << 2);

				}
				if (((ppu_ext_registers[PPUMASK] >> R1_s_S) & 0x1) && !(!((ppu_ext_registers[PPUMASK] >> R1_M_S) & 0x1)) && dot < 10) {
						for (int i=7; i>=0; i--) {
								if (!sprite_chosen[i]) continue;
								uint8_t sprite_x = dot - 2 - oam_get(pri_oam, i, OAM_SPRITE_X);
								if (sprite_x >= 8) continue;
								if (oam_get(pri_oam, i, OAM_SPRITE_FLIP_H))
										sprite_x ^= 7;
								spalette = (((oam_get(pri_oam, i, OAM_SPRITE_TILE_HIGH) >> (7-sprite_x)) & 0x1) << 1) | ((oam_get(pri_oam, i, OAM_SPRITE_TILE_LOW) >> (7-sprite_x)) & 0x1);
								if (spalette == 0) continue;
								if (sprite_chosen[i] && sprite_id[i] == 0 && palette && dot != 257)
										ppu_ext_registers[PPUSTATUS] =  ppu_ext_registers[PPUSTATUS] | ( 1 << R2_S_S);
								spalette |= ((oam_get(pri_oam, i, OAM_SPRITE_PALETTE) & 0x3) << 2);
								spalette += 16;
								obj_priority = oam_get(pri_oam, i, OAM_SPRITE_PALETTE) & 0x20;
						}
				}
				if (spalette && (palette == 0 || obj_priority == 0)) 
						palette = spalette;
        		output[scanline*256 + dot - 2] = NES_COLOR_PALETTES_STD[mem_read(0x3f00 + (show_sprite_or_bkg() ? palette : 0))];

		}	
		bm_shift_low <<= 1;
		bm_shift_high <<= 1;
		attr_latch_low = (attr_latch_low << 1) | attrtable_latch;
		attr_latch_high = (attr_latch_high << 1) | attrtable_latch;
}

// reference 
// http://wiki.nesdev.com/w/index.php/PPU_scrolling
void PPU::scroll_vert() {
		if (!show_sprite_or_bkg()) return;
		if (v_addr.fine_y < 7)	v_addr.fine_y++;
		else {
				v_addr.fine_y = 0;
				if (v_addr.coarse_y == 29) {
						v_addr.coarse_y = 0;
						v_addr.nt_addr ^= 0x2;
				}
				else if (v_addr.coarse_y == 31)
						v_addr.coarse_y = 0;
				else
						v_addr.coarse_y++;
		}
}

void PPU::scroll_horiz() {
		if (!show_sprite_or_bkg()) return;
		if (v_addr.coarse_x == 31) {
				v_addr.coarse_x = 0;
				v_addr.nt_addr ^= 0x1;
		}
		else
				v_addr.coarse_x++;
}

void PPU::oam_copy() {
		for (int i=0;i<8;i++) {
				pri_oam[i] = sec_oam[i];
				sprite_id[i] = sec_sprite_id[i];
				sprite_chosen[i] = sec_sprite_chosen[i];
		}
}

void PPU::evaluate_sprites() {
		int count = 0;
		for (int i=0;i<64;i++) {
				int sprite_y = (scanline == 261 ? -1: scanline) - oam_mem[i*4];
				if (sprite_y >= 0 && sprite_y < sprite_height()) {
						sec_sprite_chosen[count] = true;
						sec_sprite_id[count] = i;
						sec_oam[count*4+0] = oam_mem[i*4+0];
						sec_oam[count*4+1] = oam_mem[i*4+1];
						sec_oam[count*4+2] = oam_mem[i*4+2];
						sec_oam[count*4+3] = oam_mem[i*4+3];
						count++;
						if (count>=8) {
								ppu_ext_registers[PPUSTATUS] = (ppu_ext_registers[PPUSTATUS] | (0x1<<R2_O_S));
								break;
						}
				}
		}
}

void PPU::scanline_cycle(SCANLINE_TYPE t) {
		static uint16_t addr;
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
								if (t == SCANLINE_TYPE_PRE_RENDER)
										ppu_ext_registers[PPUSTATUS] = (ppu_ext_registers[PPUSTATUS] & (~(0x3<<R2_O_S)));
								break;
						}
						case 257: { evaluate_sprites(); break; }
						case 321: { oam_copy(); break; }
				}

				switch (dot) {
						case 1 ... 256:
						case 322 ... 337: {
								draw();
								switch (dot%8) {
										case 1: {
												addr = 0x2000 | (v_addr.reg & 0xfff);

												if (dot == 1) {
														if (t == SCANLINE_TYPE_PRE_RENDER)
																ppu_ext_registers[PPUSTATUS] = ppu_ext_registers[PPUSTATUS] & (~(0x1<<R2_V_S));
												}
												else {
														bm_shift_low = (bm_shift_low & 0xff00) | bm_low;
    													bm_shift_high = (bm_shift_high & 0xff00) | bm_high;
											
    													attr_latch_low = (attrtable_latch & 0x1);
    													attr_latch_high = (attrtable_latch & 0x2);
    											}
    											break;
										}
										case 2: { nametable_latch = mem_read(addr); break; }
										case 3: {
												addr = 0x23c0 | (v_addr.nt_addr << 10) | ((v_addr.coarse_y / 4) << 3) | (v_addr.coarse_x / 4);
												break;
										}
										case 4: {
												attrtable_latch = mem_read(addr);
												if (v_addr.coarse_y & 0x2)
														attrtable_latch >>= 4;
												if (v_addr.coarse_x & 0x2)
														attrtable_latch >>= 2;
												break;
										}
										case 5: {
												addr = (((ppu_ext_registers[PPUCTRL]>>R0_B_S) & R0_B_M) * 0x1000) + (nametable_latch * 16) + v_addr.fine_y;
												break;
										}
										case 6: { bm_low = mem_read(addr); break; }
										case 7: { addr += 8; break; }
										case 0: {
												bm_high = mem_read(addr);
												if (dot == 256) scroll_vert(); 
												else scroll_horiz();
												break;
										}
								}
								break;
						}
						case 257: {
								draw();

								bm_shift_low = (bm_shift_low & 0xff00) | bm_low;
    							bm_shift_high = (bm_shift_high & 0xff00) | bm_high;
								
    							attr_latch_low = (attrtable_latch & 0x1);
    							attr_latch_high = (attrtable_latch & 0x2);
    							if (show_sprite_or_bkg())
    									v_addr.reg = (v_addr.reg & (~0x041f)) | (t_addr.reg & 0x041f);
    							break;
						}
						case 280 ... 304: {
								if (t == SCANLINE_TYPE_PRE_RENDER)
										v_addr.reg = (v_addr.reg & (~0x7be0)) | (t_addr.reg & 0x7be0);
								break;
						}
						case 321:
						case 339: {
								addr = 0x2000 | (v_addr.reg & 0xfff);
								break;
						}
						case 338: { nametable_latch = mem_read(addr); break; }
						case 340: {
								nametable_latch = mem_read(addr);
								if (t == SCANLINE_TYPE_PRE_RENDER && show_sprite_or_bkg() && odd_frame)
										dot++;
						}
						/* CALL CARTRIDGE INTERRUPT */
				}
		}
}
