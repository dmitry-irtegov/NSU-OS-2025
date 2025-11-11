CC = cc
CFLAGS_32 = -m32 -v
CFLAGS_64 = -m64 -v

TARGET_32 = test_32
TARGET_64 = test_64
TIME_32 = time_32
TIME_64 = time_64

all: $(TARGET_32) $(TARGET_64) $(TIME_32) $(TIME_64)

execvpe.32.o: execvpe.c
	$(CC) $(CFLAGS_32) -c execvpe.c -o $@

test.32.o: test.c
	$(CC) $(CFLAGS_32) -c test.c -o $@

time.32.o: time.c
	$(CC) $(CFLAGS_32) -c time.c -o $@

$(TIME_32): time.32.o
	$(CC) $(CFLAGS_32) $^ -o $@

$(TARGET_32): execvpe.32.o test.32.o
	$(CC) $(CFLAGS_32) $^ -o $@


execvpe.64.o: execvpe.c
	$(CC) $(CFLAGS_64) -c execvpe.c -o $@

test.64.o: test.c
	$(CC) $(CFLAGS_64) -c test.c -o $@

time.64.o: time.c
	$(CC) $(CFLAGS_64) -c time.c -o $@

$(TIME_64): time.64.o
	$(CC) $(CFLAGS_64) $^ -o $@

$(TARGET_64): execvpe.64.o test.64.o
	$(CC) $(CFLAGS_64) $^ -o $@


run_32: $(TARGET_32) $(TIME_32)
	@echo "=== Running 32-bit test ==="
	./$(TARGET_32) TZ=America/Los_Angeles -- ./$(TIME_32)

run_64: $(TARGET_64) $(TIME_64)
	@echo "=== Running 64-bit test ==="
	./$(TARGET_64) TZ=America/Los_Angeles -- ./$(TIME_64)

run: run_32 run_64

clean:
	rm -f *.32.o *.64.o $(TARGET_32) $(TARGET_64) $(TIME_32) $(TIME_64)

.PHONY: all clean run run_32 run_64
