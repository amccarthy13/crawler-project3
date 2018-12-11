CC = g++
CFLAGS = -std=c++11 -g -Wall -pthread


all: mcrawler2 mcrawler1


mcrawler2: mcrawler2.o socket.o parser.o
	$(CC) $(CFLAGS) -o mcrawler2 mcrawler2.o socket.o parser.o

mcrawler1: mcrawler1.o socket.o parser.o
	$(CC) $(CFLAGS) -o mcrawler1 mcrawler1.o socket.o parser.o

mcrawler2.o: mcrawler2.cpp socket.h parser.h
	$(CC) $(CFLAGS) -c mcrawler2.cpp

mcrawler1.o: mcrawler1.cpp socket.h parser.h
	$(CC) $(CFLAGS) -c mcrawler1.cpp

socket.o: socket.cpp parser.h
	$(CC) $(CFLAGS) -c socket.cpp

parser.o: parser.cpp
	$(CC) $(CFLAGS) -c parser.cpp

clean:
	rm -f mcrawler1 mcrawler2 *.o *.jpg *.pdf *.css *.png *.pdf *.js *.ico *.html