CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=main.cpp rob.cpp rob_circ.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=../rob_pwr

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@