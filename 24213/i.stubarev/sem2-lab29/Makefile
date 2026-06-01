CC = cc
CFLAGS = -v
LIBS = -lresolv -lsocket -lnsl -laio
TARGET = lab29

$(TARGET): sem2lab29.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
