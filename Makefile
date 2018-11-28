all: nes_emu
nes_emu: emu.o ppu.o
ifeq ($(shell uname), Darwin)
	g++ -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework Cocoa -framework SDL2 emu.o ppu.o -o nes_emu
else
	g++ -lSDL2 emu.o ppu.o -o nes_emu
endif

emu.o:	emu.cpp
	g++ -std=c++11 -c emu.cpp
ppu.o:	ppu.cpp
	g++ -std=c++11 -c ppu.cpp

clean:
	rm -f *.o
