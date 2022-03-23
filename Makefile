SRC := server.cpp
HDR := ircserver.h
OBJ := $(patsubst %.cpp, %.o, $(SRC))
INC_DIR := ./include

CXX=g++
CXXFLAGS=-Wall -g -O0 -std=c++17 -I$(INC_DIR)/

all: server client

server: $(OBJ)
	$(CXX) -o $@ $(OBJ)

client: client.o
	$(CXX) -o $@ client.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf server client *.o

