#include <stdint.h>

class PPU {
public:
		uint8_t ppu_ext_registers[8];
		int output[256 * 240];

public:
		PPU();
		~PPU();
		void cycle();
		
private:
		const uint16_t ppu_mem_base_addr = 0x2000;
		uint8_t ppu_mem[0x800];
		const uint16_t palette_mem_base_addr = 0x3f00;
		uint8_t palette_mem[0x20];
		uint8_t oam_mem[0x100];
		uint8_t oam_addr;

public:

		const uint8_t PPUCTRL = 0;
		const uint8_t PPUMASK = 1;
		const uint8_t PPUSTATUS = 2;
		const uint8_t OAMADDR = 3;
		const uint8_t OAMDATA = 4;
		const uint8_t PPUSCROLL = 5;
		const uint8_t PPUADDR = 6;
		const uint8_t PPUDATA = 7;

		const uint8_t R0_V_S  = 7; const uint8_t R0_V_M  = 0x1;
		const uint8_t R0_P_S  = 6; const uint8_t R0_P_M  = 0x1;
		const uint8_t R0_H_S  = 5; const uint8_t R0_H_M  = 0x1;
		const uint8_t R0_B_S  = 4; const uint8_t R0_B_M  = 0x1;
		const uint8_t R0_S_S  = 3; const uint8_t R0_S_M  = 0x1;
		const uint8_t R0_I_S  = 2; const uint8_t R0_I_M  = 0x1;
		const uint8_t R0_NN_S = 0; const uint8_t R0_NN_M = 0x3;

		const uint8_t R1_B_S  = 7; const uint8_t R1_B_M  = 0x1;
		const uint8_t R1_G6_S = 6; const uint8_t R1_G6_M = 0x1;
		const uint8_t R1_R_S  = 5; const uint8_t R1_R_M  = 0x1;
		const uint8_t R1_s_S  = 4; const uint8_t R1_s_M  = 0x1;
		const uint8_t R1_b_S  = 3; const uint8_t R1_b_M  = 0x1;
		const uint8_t R1_M_S  = 2; const uint8_t R1_M_M  = 0x1;
		const uint8_t R1_m_S  = 1; const uint8_t R1_m_M  = 0x1;
		const uint8_t R1_G0_S = 0; const uint8_t R1_G0_M = 0x1;

		const uint8_t R2_V_S = 7; const uint8_t R2_V_M = 0x1;
		const uint8_t R2_S_S = 6; const uint8_t R2_S_M = 0x1;
		const uint8_t R2_O_S = 5; const uint8_t R2_O_M = 0x1;

		const uint8_t R3_S = 0; const uint8_t R3_M = 0xff;
		const uint8_t R4_S = 0; const uint8_t R4_M = 0xff;
		const uint8_t R5_S = 0; const uint8_t R5_M = 0xff;
		const uint8_t R6_S = 0; const uint8_t R6_M = 0xff;
		const uint8_t R7_S = 0; const uint8_t R7_M = 0xff;

public:
		uint8_t read_ppu_register(int reg, uint8_t s, uint8_t m) {
				return access_ppu_register(reg, s, m, 0, false);
		}
		void write_ppu_register(int reg, uint8_t s, uint8_t m, uint8_t val) {
				access_ppu_register(reg, s, m, val, true);
		}
		uint8_t access_ppu_register(int reg, uint8_t s, uint8_t m, uint8_t val, bool rw);

private:
		uint32_t NES_COLOR_PALETTES_STD[64] = 
				{ 0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
  				  0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
  				  0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
  				  0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
  				  0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
  				  0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
  				  0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
  				  0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000 };
};
