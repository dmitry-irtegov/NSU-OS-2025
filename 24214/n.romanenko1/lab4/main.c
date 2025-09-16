#include <stdio.h>
#include <stdlib.h>

typedef struct String_T {
    int capacity;
    int size;
    char* arr;
} String;

int init_string(String* str) {
    str->capacity = 2;
    str->size = 0;
    str->arr = (char*)malloc(str->capacity * sizeof(char));
    if (!str->arr) return -1;
    str->arr[0] = '\0';
    return 0;
}
int append_char(String* str, char c) {
    if (str->size + 1 >= str->capacity) {
        str->capacity *= 2;
        char* new_arr = (char*)realloc(str->arr, str->capacity);
        if (!new_arr) return -1;
        str->arr = new_arr;
    }
    str->arr[str->size++] = c;
    str->arr[str->size] = '\0';
    return 0;
}

void free_string(String* str) {
    if (str->arr) free(str->arr);
    str->arr = NULL;
    str->size = 0;
    str->capacity = 0;
}

String copy_string(const String* src) {
    String dest = {0, 0, NULL};
    if (!src || !src->arr) return dest;
    
    dest.capacity = src->size + 1;
    dest.size = src->size;
    dest.arr = (char*)malloc(dest.capacity);
    if (!dest.arr)
    {
        dest.size = 0;
        dest.capacity = 0;
        return dest;
    }
    
    for(int i = 0; i < dest.size; i++)
    {
        dest.arr[i] = src->arr[i];
    }
    dest.arr[src->size] = '\0';
    return dest;
}

typedef struct List_t {
    int capacity;
    int size;
    String* arr;
} List;

int init_list(List* lst) {
    lst->capacity = 2;
    lst->size = 0;
    lst->arr = (String*)malloc(lst->capacity * sizeof(String));
    if (!lst->arr) return -1;
    return 0;
}

int list_append(List* lst, const String* str) {
    if (lst->size >= lst->capacity) {
        lst->capacity *= 2;
        String* new_arr = (String*)realloc(lst->arr, lst->capacity * sizeof(String));
        if (!new_arr) return -1;
        lst->arr = new_arr;
    }
    
    String copy = copy_string(str);
    if (!copy.arr) return -1;
    
    lst->arr[lst->size++] = copy;
    return 0;
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
    
    if (init_string(&buffer) != 0) {
        fprintf(stderr, "Ошибка инициализации строки\n");
        return 1;
    }
    
    if (init_list(&result) != 0) {
        fprintf(stderr, "Ошибка инициализации списка\n");
        free_string(&buffer);
        return 1;
    }

    while ((c = getchar()) != EOF) {
        if (c == '.' && buffer.size == 0) {
            break;
        }

        if (c == '\n') {
            if (buffer.size > 0) {
                if (append_char(&buffer, '\0') != 0) {
                    fprintf(stderr, "Ошибка добавления символа\n");
                    break;
                }
                if (list_append(&result, &buffer) != 0) {
                    fprintf(stderr, "Ошибка добавления в список\n");
                    break;
                }
                free_string(&buffer);
                if (init_string(&buffer) != 0) {
                    fprintf(stderr, "Ошибка переинициализации строки\n");
                    break;
                }
            }
        } else {
            if (append_char(&buffer, c) != 0) {
                fprintf(stderr, "Ошибка добавления символа\n");
                break;
            }
        }
    }

    free_string(&buffer);

    for (int i = 0; i < result.size; i++) {
        fputs(result.arr[i].arr, stdout);
        fputs("\n", stdout);
    }
    
    free_list(&result);
    return 0;
}