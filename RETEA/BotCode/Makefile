CC=g++
CFLAGS=-Wall -Wextra -std=c++17 -O3 -march=native
TARGETS=taleNt

build: $(TARGETS)

taleNt: taleNt.cpp hlt.hpp networking.hpp *.h ../Network/*.h
	$(CC) $(CFLAGS) taleNt.cpp -lm -o taleNt

pack:
	zip -FSr taleNt_I.zip Makefile *.cpp *.h *.hpp

clean:
	rm -f taleNt

.PHONY: pack clean


