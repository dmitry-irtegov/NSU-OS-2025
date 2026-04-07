TARGET = my_cpr
OBJS = main.o dirhandler.o regularhandler.o

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc -Wall -Wextra -pthread -O2 $(OBJS) -o $(TARGET)

main.o: main.c
	gcc -Wall -Wextra -pthread -O2 -c main.c

dirhandler.o: dirhandler.c
	gcc -Wall -Wextra -pthread -O2 -c dirhandler.c

regularhandler.o: regularhandler.c
	gcc -Wall -Wextra -pthread -O2 -c regularhandler.c

clean:
	rm -f $(OBJS) $(TARGET)