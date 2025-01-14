#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFFER_SZ 50

/* Prototypes */
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);
int count_words(char *, int, int);
int reverse_string(char *, int, int);
int print_words(char *, int, int);
int replace_substring(char *, int, int, char *, char *);

/**
 * setup_buff
 *
 * Copies over every non-whitespace character from the user-supplied string
 * to the internal buffer. Consecutive whitespace is replaced by a single
 * space ' '. Tabs and other whitespace are also replaced by a single space.
 * The remainder of the buffer is filled with '.' characters. Returns:
 *   -1 : if user_str is too large to fit into buff (more than BUFFER_SZ chars)
 *   >=0: the length of the user-supplied string after whitespace collapsing.
 */
int setup_buff(char *buff, char *user_str, int len) {
    char *dst = buff;
    char *src = user_str;
    int count = 0;
    int in_space = 0;

    if (!buff || !user_str || len <= 0) {
        return -2;
    }

    while (*src != '\0') {
        if (isspace(*src)) {
            if (!in_space && count < len) {
                *dst++ = ' ';
                count++;
                in_space = 1;
            }
        } else {
            if (count < len) {
                *dst++ = *src;
                count++;
                in_space = 0;
            } else {
                return -1;
            }
        }
        src++;
    }

    while (count < len) {
        *dst++ = '.';
        count++;
    }

    return count;
}

/**
 * print_buff
 *
 * Prints the entire buffer for debugging purposes.
 */
void print_buff(char *buff, int len) {
    printf("Buffer:  ");
    for (int i = 0; i < len; i++) {
        putchar(buff[i]);
    }
    putchar('\n');
}

/**
 * usage
 *
 * Prints usage instructions for the program.
 */
void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

/**
 * count_words
 *
 * Counts the number of words in the user string (up to str_len).
 */
int count_words(char *buff, int buff_len, int str_len) {
    if (!buff || str_len > buff_len) {
        return -1;
    }

    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (!isspace(buff[i]) && buff[i] != '.') {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        } else {
            in_word = 0;
        }
    }

    return count;
}

/**
 * reverse_string
 *
 * Reverses the user-supplied string in place (up to str_len).
 */
int reverse_string(char *buff, int buff_len, int str_len) {
    if (!buff || str_len > buff_len) {
        return -1;
    }

    char *start = buff;
    char *end = buff + str_len - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    return 0;
}

/**
 * print_words
 *
 * Prints individual words and their lengths.
 */
int print_words(char *buff, int buff_len, int str_len) {
    if (!buff || str_len > buff_len) {
        return -1;
    }

    int word_idx = 1;
    int in_word = 0;
    char *start = NULL;

    printf("Word Print\n");
    printf("----------\n");

    for (int i = 0; i < str_len; i++) {
        if (buff[i] != ' ' && buff[i] != '.') {
            if (!in_word) {
                start = &buff[i];
                in_word = 1;
            }
        } else if (in_word) {
            printf("%d. ", word_idx++);
            fwrite(start, 1, &buff[i] - start, stdout);
            printf(" (%ld)\n", &buff[i] - start);
            in_word = 0;
        }
    }

    if (in_word) {
        printf("%d. ", word_idx++);
        printf("%s (%ld)\n", start, (buff + str_len) - start);
    }

    return word_idx - 1;
}

/**
 * replace_substring
 *
 * Replaces the first occurrence of 'search' with 'replace' in buff.
 */
int replace_substring(char *buff, int buff_len, int str_len, char *search, char *replace) {
    if (!buff || !search || !replace) {
        return -1;
    }

    int search_len = 0, replace_len = 0;

    while (search[search_len] != '\0') search_len++;
    while (replace[replace_len] != '\0') replace_len++;

    char *found = NULL;
    for (int i = 0; i <= str_len - search_len; i++) {
        if (strncmp(buff + i, search, search_len) == 0) {
            found = buff + i;
            break;
        }
    }

    if (!found) {
        return -2;
    }

    int new_len = str_len - search_len + replace_len;
    if (new_len > buff_len) {
        return -1;
    }

    memmove(found + replace_len, found + search_len, str_len - (found - buff) - search_len);
    memcpy(found, replace, replace_len);

    return 0;
}

/**
 * main
 */
int main(int argc, char *argv[]) {
    char *buff;
    char *input_string;
    char opt;
    int rc;
    int user_str_len;

    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1);

    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2];
    buff = (char *)malloc(BUFFER_SZ);

    if (!buff) {
        perror("Memory allocation failed");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            printf("Word Count: %d\n", rc);
            break;
        case 'r':
            rc = reverse_string(buff, BUFFER_SZ, user_str_len);
            if (rc == 0) {
                printf("Reversed String: ");
                fwrite(buff, 1, user_str_len, stdout);
                putchar('\n');
            }
            break;
        case 'w':
            print_words(buff, BUFFER_SZ, user_str_len);
            break;
        case 'x':
            if (argc < 5) {
                printf("Error: -x requires search and replace strings\n");
                free(buff);
                exit(1);
            }
            rc = replace_substring(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            if (rc == 0) {
                printf("Modified String: ");
                fwrite(buff, 1, user_str_len, stdout);
                putchar('\n');
            } else {
                printf("Error replacing substring\n");
            }
            break;
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}
