build:
	gcc -Wall -Wextra -O3 -march=native -std=c99 ./src/*.c -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm -lGL -o renderer

run:
	./renderer

clean:	
	rm renderer

.PHONY: build run clean