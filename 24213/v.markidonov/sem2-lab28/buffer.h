typedef struct buffer {
    char *buf;
    int capacity;
    int size;
    int offset;
} buffer;

void buffer_init(buffer *b, int capacity);

int buffer_write(int fd, buffer *b, int size);

int buffer_recv(int sock_fd, buffer *b, int size);

char buffer_getchar(buffer *b);
