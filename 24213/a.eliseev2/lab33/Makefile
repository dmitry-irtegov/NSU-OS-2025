CC = gcc
LDLIBS = -lsocket -lnsl
CFLAGS = -O2 -Wall

proxy.out: proxy.c
	$(LINK.c) -o $@ $(LDLIBS) $^
