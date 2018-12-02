#include "ppu.hpp"
#include <iostream>
#include <stdlib.h>

int main(int argc, char *argv[]) {
		PPU ppu;
		for (int i=0;i<256*240;i++) {
				ppu.output[i] = 0xff0000;
				bool quit = false;
				ppu.render_current(quit);
				if (quit)
					break;
		}
		SDL_Quit();
		return 0;
}

