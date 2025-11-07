#include "shell.h"
#include <ctype.h>

void clear_globals()
{
    infile = NULL;
    outfile = NULL;
    appfile = NULL;
    bkgrnd = 0;
    num_cmds = 0;
    for (int i = 0; i < MAX_CMDS; i++)
    {
        for (int j = 0; j < MAX_ARGS; j++)
        {
            cmds[i].cmdargs[j] = NULL;
        }
    }
}

int get_next_token(char **line, char *token)
{
    char *p = *line;
    int token_len = 0;

    while (isspace(*p))
        p++;

    if (*p == '\0')
    {
        *line = p;
        return 0;
    }

    if (*p == '|' || *p == '<' || *p == '&')
    {
        token[0] = *p;
        token[1] = '\0';
        *line = p + 1;
        return 1;
    }

    if (*p == '>' && *(p + 1) == '>')
    {
        token[0] = '>';
        token[1] = '>';
        token[2] = '\0';
        *line = p + 2;
        return 2;
    }

    if (*p == '>')
    {
        token[0] = *p;
        token[1] = '\0';
        *line = p + 1;
        return 1;
    }

    char quote_char = '\0';

    while (*p != '\0' && token_len < MAX_LINE - 1)
    {
        if (quote_char == '\0')
        {
            // Не внутри кавычек
            if (*p == '"' || *p == '\'')
            {
                quote_char = *p;
                p++;
                continue;
            }
            else if (isspace(*p) || *p == '|' || *p == '<' || *p == '>' || *p == '&')
            {
                break;
            }
        }
        else
        {
            // Внутри кавычек
            if (*p == quote_char)
            {
                quote_char = '\0';
                p++;
                continue;
            }
        }

        token[token_len++] = *p++;
    }

    if (quote_char != '\0')
    {
        return -1; // Ошибка: незакрытые кавычки
    }

    token[token_len] = '\0';
    *line = p;
    return token_len;
}

/*
 * @return count of commands. If error returns -1.
 */
int parseline(char *line)
{
    clear_globals();

    line[strcspn(line, "\n")] = '\0';

    char token[MAX_LINE];
    char *line_ptr = line;

    int cmd_index = 0;
    int arg_index = 0;

    static char token_storage[MAX_CMDS * MAX_ARGS][MAX_LINE];
    static int storage_index = 0;

    storage_index = 0;

    while (1)
    {
        int token_len = get_next_token(&line_ptr, token);

        if (token_len == -1)
        {
            fprintf(stderr, "Parse error: unclosed quotes\n");
            return -1;
        }

        if (token_len == 0)
            break; // Конец строки

        if (strcmp(token, "&") == 0)
        {
            bkgrnd = 1;
            break;
        }
        else if (strcmp(token, "<") == 0)
        {
            token_len = get_next_token(&line_ptr, token);
            if (token_len <= 0)
            {
                fprintf(stderr, "Parse error: missing filename after '<'\n");
                return -1;
            }

            if (storage_index >= MAX_CMDS * MAX_ARGS)
            {
                fprintf(stderr, "Parse error: too many tokens\n");
                return -1;
            }

            strcpy(token_storage[storage_index], token);
            infile = token_storage[storage_index++];
        }
        else if (strcmp(token, ">") == 0)
        {
            token_len = get_next_token(&line_ptr, token);
            if (token_len <= 0)
            {
                fprintf(stderr, "Parse error: missing filename after '>'\n");
                return -1;
            }

            if (storage_index >= MAX_CMDS * MAX_ARGS)
            {
                fprintf(stderr, "Parse error: too many tokens\n");
                return -1;
            }

            strcpy(token_storage[storage_index], token);
            outfile = token_storage[storage_index++];
        }
        else if (strcmp(token, ">>") == 0)
        {
            token_len = get_next_token(&line_ptr, token);
            if (token_len <= 0)
            {
                fprintf(stderr, "Parse error: missing filename after '>>'\n");
                return -1;
            }

            if (storage_index >= MAX_CMDS * MAX_ARGS)
            {
                fprintf(stderr, "Parse error: too many tokens\n");
                return -1;
            }

            strcpy(token_storage[storage_index], token);
            appfile = token_storage[storage_index++];
        }
        else if (strcmp(token, "|") == 0)
        {
            if (arg_index == 0)
            {
                fprintf(stderr, "Parse error: empty command before '|'\n");
                return -1;
            }

            cmds[cmd_index].cmdargs[arg_index] = NULL;
            cmd_index++;
            arg_index = 0;

            if (cmd_index >= MAX_CMDS)
            {
                fprintf(stderr, "Parse error: too many commands in pipeline\n");
                return -1;
            }
        }
        else
        {
            // Обычный аргумент
            if (arg_index >= MAX_ARGS - 1)
            {
                fprintf(stderr, "Parse error: too many arguments\n");
                return -1;
            }

            if (storage_index >= MAX_CMDS * MAX_ARGS)
            {
                fprintf(stderr, "Parse error: too many tokens\n");
                return -1;
            }

            strcpy(token_storage[storage_index], token);
            cmds[cmd_index].cmdargs[arg_index] = token_storage[storage_index++];
            arg_index++;
        }
    }

    cmds[cmd_index].cmdargs[arg_index] = NULL;

    if (cmds[0].cmdargs[0] == NULL)
        return 0; // Пустая строка

    return cmd_index + 1;
}
