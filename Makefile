all: nes_emu
nes_emu: main.o ui.o ppu.o
ifeq ($(shell uname), Darwin)
	g++ -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework Cocoa -framework SDL2 ui.o ppu.o main.o -o nes_emu
else
	g++ -lSDL2 ui.o ppu.o main.o -o nes_emu
endif

ui.o:	ui.cpp
	g++ -std=c++11 -c ui.cpp
ppu.o:	ppu.cpp
	g++ -std=c++11 -c ppu.cpp
main.o: main.cpp
	g++ -std=c++11 -c main.cpp

clean:
	rm -f *.o
