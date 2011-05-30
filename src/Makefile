CC=g++
CFLAGS=-c -Wall -g
LDFLAGS=
SOURCES=main.cpp rob.h rob.cpp rob_circ.h rob_circ.cpp rob_dyn.h rob_dyn.cpp rob_latch.h rob_latch.cpp common.h common.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=../rob_pwr

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
