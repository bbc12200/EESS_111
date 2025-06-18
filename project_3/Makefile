
EXEC = a.out
CC = g++
CFLAGS = -std=c++11
LDFLAGS = -lpthread

all: $(EXEC)

$(EXEC): main.o
	$(CC) $(CFLAGS) -o $(EXEC) main.o $(LDFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

clean:
	rm -f *.o $(EXEC) a

