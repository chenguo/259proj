CC=g++
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=rob_pwr.cpp rob_base.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=../rob_pwr

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@