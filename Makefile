SRC = ./src/*.c
FLAGS = -Wall -Wextra -Wformat=2 -O3 -march=native -std=c99
SANITIZER_FLAGS = -Wall -Wextra -Wformat=2 -g -O0 -std=c99 -fsanitize=address -fno-omit-frame-pointer

build-linux:
	gcc $(SRC) \
	$(FLAGS) \
	-lSDL2 -lSDL2_mixer -lm \
	-lGL \
	-o blocks

build-linux-asan:
	gcc $(SRC) \
	$(SANITIZER_FLAGS) \
	-lSDL2 -lSDL2_mixer -lm \
	-lGL \
	-o blocks

SDL2_WIN = ./lib/SDL2-2.32.10/x86_64-w64-mingw32
MIXER_WIN = ./lib/SDL2_mixer-2.8.1/x86_64-w64-mingw32

build-windows:
	x86_64-w64-mingw32-gcc \
	$(FLAGS) \
	-I$(SDL2_WIN)/include/SDL2 \
	-I$(SDL2_WIN)/include \
	-I$(MIXER_WIN)/include/SDL2 \
	-I$(MIXER_WIN)/include \
	$(SRC) \
	-L$(SDL2_WIN)/lib \
	-L$(MIXER_WIN)/lib \
	-lmingw32 -lSDL2main -lSDL2_mixer -lSDL2 -lm \
	-o blocks.exe
	cp $(SDL2_WIN)/bin/SDL2.dll .
	cp $(MIXER_WIN)/bin/SDL2_mixer.dll .

run-linux:
	./blocks

run-windows:
	wine ./blocks.exe

clean:
	rm -f blocks blocks.exe

.PHONY: build-linux build-linux-asan build-windows run-linux run-windows clean