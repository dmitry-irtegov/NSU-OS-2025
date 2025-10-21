#include <stdio.h>
#include <stdlib.h>

typedef struct String_T {
    int capacity;
    int size;
    char* arr;
} String;

void init_string(String* str) {
    str->capacity = 2;
    str->size = 0;
    str->arr = (char*)malloc(str->capacity * sizeof(char));
    if(!str->arr)
    {
        
    }
    if (str->arr) str->arr[0] = '\0';
}

void append_char(String* str, char c) {
    if (str->size + 1 >= str->capacity) {
        str->capacity *= 2;
        str->arr = (char*)realloc(str->arr, str->capacity);
    }
    str->arr[str->size++] = c;
    str->arr[str->size] = '\0';
}

void free_string(String* str) {
    if (str->arr) free(str->arr);
    str->arr = NULL;
    str->size = 0;
    str->capacity = 0;
}

String copy_string(const String* src) {
    String dest;
    dest.capacity = src->size + 1;
    dest.size = src->size;
    dest.arr = (char*)malloc(dest.capacity);
    if (dest.arr && src->arr) {
        for(int i = 0; i < dest.size; i++)
        {
            dest.arr[i] = src->arr[i];
        }
        dest.arr[src->size] = '\0';
    }
    return dest;
}

typedef struct List_t {
    int capacity;
    int size;
    String* arr;
} List;

void init_list(List* lst) {
    lst->capacity = 2;
    lst->size = 0;
    lst->arr = (String*)malloc(lst->capacity * sizeof(String));
}

void list_append(List* lst, String* str) {
    if (lst->size >= lst->capacity) {
        lst->capacity *= 2;
        String* new_arr = (String*)realloc(lst->arr, lst->capacity * sizeof(String));
        lst->arr = new_arr;
    }
    lst->arr[lst->size++] = copy_string(str);
}

void free_list(List* lst) {
    if (lst->arr) {
        for (int i = 0; i < lst->size; i++) {
            free_string(&lst->arr[i]);
        }
        free(lst->arr);
    }
}

int main() {
    int c;
    String buffer;
    List result;
    
    init_string(&buffer);
    init_list(&result);

    while ((c = getchar()) != EOF) {
        if (c == '.' && buffer.size == 0) {
            break;
        }

        if (c == '\n') {
            if (buffer.size > 0) {
                list_append(&result, &buffer);
                free_string(&buffer);
                init_string(&buffer);
            }
        } else {
            append_char(&buffer, c);
        }
    }

    for (int i = 0; i < result.size; i++) {
        fputs(result.arr[i].arr, stdout);
        fputs("\n", stdout);
    }
    
    free_list(&result);
    return 0;
}