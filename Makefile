CC = g++
CFLAGS = -std=c++11 -g -Wall

crawler: crawler.o socket.o parser.o
	$(CC) $(CFLAGS) -o crawler crawler.o socket.o parser.o

crawler.o: crawler.cpp socket.h parser.h
	$(CC) $(CFLAGS) -c crawler.cpp

socket.o: socket.cpp parser.h
	$(CC) $(CFLAGS) -c socket.cpp

parser.o: parser.cpp
	$(CC) $(CFLAGS) -c parser.cpp

clean:
	rm -f crawler *.o *.jpg *.pdf *.css *.png *.pdf *.js