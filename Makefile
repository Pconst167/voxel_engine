SRC = ./src/*.c ./src/emulator/*.c
FLAGS = -Wall -Wextra -Wformat=2 -O3 -march=native -std=c99 -lSDL2 -lSDL2_mixer -lm -lGL

build:
	gcc $(SRC) $(FLAGS)  -o renderer

run:
	./renderer

clean:	
	rm renderer

.PHONY: build run clean