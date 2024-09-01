SOURCES=server.cpp subscriber.cpp
LIBRARY=nope
INCPATHS=include
LIBPATHS=.
LDFLAGS=
CFLAGS=-c -Wall -Werror -Wno-error=unused-variable
CC=g++

# Automatic generation of some important lists
OBJECTS=$(SOURCES:.cpp=.o)
INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))
LIBFLAGS=$(foreach TMP,$(LIBPATHS),-L$(TMP))

# Set up the output file names for the different output types
BINARY=$(PROJECT)

all: server subscriber

server: server.cpp
	g++ server.cpp utils.cpp -o server

subscriber: subscriber.cpp
	g++ subscriber.cpp -o subscriber

clean:
	rm -rf server subscriber
