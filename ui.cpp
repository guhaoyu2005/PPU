//
//
#include <iostream>
#include <stdlib.h>
#include "ui.hpp"

using namespace std;

SDL_Window *win;
SDL_Renderer *render;

void init_sdl() {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
				std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
				exit(0);
		}
		win = SDL_CreateWindow("PeNES", 200, 200, 256, 240, SDL_WINDOW_SHOWN);
		if (win == nullptr) {
			std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
			SDL_Quit();
			exit(0);
		}

		render = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (render == nullptr){
				SDL_DestroyWindow(win);
				std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
				SDL_Quit();
				exit(0);
		}
}

void render_screen(int *output) {
		if (win == nullptr)
				init_sdl();

		SDL_RenderClear(render);
		for (int i=0;i<240;i++) {
				for (int j=0;j<256;j++) {
						SDL_SetRenderDrawColor(render, (output[i*256+j] >> 16) & 0xff, (output[i*256+j] >> 8) & 0xff, output[i*256+j] & 0xff, 0xff);
						//SDL_SetRenderDrawColor(render, 0, 255, 0, 255);
						SDL_RenderDrawPoint(render, j, i);
				}
		}	
		SDL_RenderPresent(render);
		//SDL_Delay(50);
}
