SRC = ./src/*.c ./src/emulator/*.c

build-linux:
	gcc $(SRC) \
	-Wall -Wextra -Wformat=2 -O3 -march=native -std=c99 \
	-lSDL2 -lSDL2_mixer -lm \
	-o renderer

SDL2_WIN = ./lib/SDL2-2.32.10/x86_64-w64-mingw32
MIXER_WIN = ./lib/SDL2_mixer-2.8.1/x86_64-w64-mingw32

build-windows:
	x86_64-w64-mingw32-gcc \
	-Wall -Wextra -O3 -std=c99 \
	-I$(SDL2_WIN)/include/SDL2 \
	-I$(SDL2_WIN)/include \
	-I$(MIXER_WIN)/include/SDL2 \
	-I$(MIXER_WIN)/include \
	$(SRC) \
	-L$(SDL2_WIN)/lib \
	-L$(MIXER_WIN)/lib \
	-lmingw32 -lSDL2main -lSDL2_mixer -lSDL2 -lm \
	-o renderer.exe
	cp $(SDL2_WIN)/bin/SDL2.dll .
	cp $(MIXER_WIN)/bin/SDL2_mixer.dll .

run-linux:
	./renderer

run-windows:
	wine ./renderer.exe

clean:	
	rm renderer

.PHONY: build run clean