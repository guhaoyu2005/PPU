#include "ppu.hpp"
#include <iostream>
#include <stdlib.h>

int main(int argc, char *argv[]) {
		PPU ppu;
		for (int i=0;i<256*240;i++) {
				ppu.output[i] = 0xff0000;
				ppu.render_current();
		}
		SDL_Quit();
		return 0;
}

