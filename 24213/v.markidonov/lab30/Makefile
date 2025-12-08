CC = cc
CFLAGS = -v -lsocket
TARGET_CLIENT = client
TARGET_SERVER = server

all: $(TARGET_CLIENT) $(TARGET_SERVER)

$(TARGET_CLIENT): client.c
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_SERVER): server.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET_CLIENT) $(TARGET_SERVER)

.PHONY: all clean
