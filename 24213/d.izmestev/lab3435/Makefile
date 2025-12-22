CC = cc
CFLAGS = -v
LIBS = -lsocket -lnsl
TARGETS = tunnel

all: $(TARGETS)

tunnel: tunnel.c
		$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
		rm -f $(TARGETS)

.PHONY: all clean
