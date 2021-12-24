all: build

build:
	g++ -lSDL2main $(pkg-config --cflags --libs sdl2) -fno-permissive -o spi spi.cpp Pixi.cpp -lbcm2835
