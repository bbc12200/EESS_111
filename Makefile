EXEC=main

CC=g++
CFLAGS=-std=c++11 `pkg-config gtkmm-3.0 --cflags`
LDFLAGS=`pkg-config gtkmm-3.0 --libs` -lpthread

all: $(EXEC)

$(EXEC): main.o helper.o
	$(CC) -o $(EXEC) main.o helper.o $(LDFLAGS)

main.o: main.cpp
	$(CC) -c main.cpp $(CFLAGS)

helper.o: helper.cpp
	$(CC) -c helper.cpp $(CFLAGS)

clean:
	rm -f *.o $(EXEC)
