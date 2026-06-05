typedef struct {
    char host[256];
    char port[6];
    char path[1824];
} url_t;

int url_parser(char* url, url_t* out);
