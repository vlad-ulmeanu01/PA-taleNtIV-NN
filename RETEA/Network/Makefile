CC=g++
CFLAGS=-Wall -Wextra -std=c++17 -O2 -march=native -I ~/Eigen/
TARGETS=main

build: $(TARGETS)

main: main.cpp *.h
	$(CC) $(CFLAGS) main.cpp -lm -o main

pack:
	zip -FSr NetworkAntrenare.zip Makefile *.cpp *.h

clean:
	rm -f main

.PHONY: pack clean
