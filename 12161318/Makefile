CC = g++
CFLAGS = -std=c++11 -g -Wall -pthread


all: mcrawl2 mcrawl1


mcrawl2: mcrawl2.o socket.o parser.o
	$(CC) $(CFLAGS) -o mcrawl2 mcrawl2.o socket.o parser.o

mcrawl1: mcrawl1.o socket.o parser.o
	$(CC) $(CFLAGS) -o mcrawl1 mcrawl1.o socket.o parser.o

mcrawl2.o: mcrawl2.cpp socket.h parser.h
	$(CC) $(CFLAGS) -c mcrawl2.cpp

mcrawl1.o: mcrawl1.cpp socket.h parser.h
	$(CC) $(CFLAGS) -c mcrawl1.cpp

socket.o: socket.cpp parser.h
	$(CC) $(CFLAGS) -c socket.cpp

parser.o: parser.cpp
	$(CC) $(CFLAGS) -c parser.cpp

clean:
	rm -f mcrawl1 mcrawl2 *.o *.jpg *.pdf *.css *.png *.pdf *.js *.ico *.html *.zip