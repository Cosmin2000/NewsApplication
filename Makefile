
# Makefile

CFLAGS = -Wall -g -lm


build: server subscriber
all: server subscriber

# Compileaza server.cpp
server: server.cpp helpers.cpp
	g++ helpers.cpp server.cpp -o server ${CFLAGS}

# Compileaza subscriber.cpp
subscriber: subscriber.cpp
	g++ subscriber.cpp -o subscriber ${CFLAGS}

.PHONY: clean server subscriber

clean:
	rm -f server subscriber
