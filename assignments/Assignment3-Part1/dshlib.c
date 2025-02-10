#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  Splits the cmd_line based on '|' to populate clist with commands
 *  and arguments.
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // 1. Trim leading spaces
    char *start = cmd_line;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    // If everything was spaces or empty, return WARN_NO_CMDS.
    if (*start == '\0') {
        return WARN_NO_CMDS;
    }

    // 2. Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    // Null-terminate after the last non-space
    end[1] = '\0';

    // Double-check if empty after trim
    if (*start == '\0') {
        return WARN_NO_CMDS;
    }

    // We will tokenize the string by '|'
    //   Because we are about to modify `cmd_line`, we can just work in place.
    int cmd_count = 0;

    // Use safer strtok_r to handle nested parsing
    char *saveptr;
    char *pipe_substring = strtok_r(start, PIPE_STRING, &saveptr);

    while (pipe_substring != NULL) {
        // Trim leading/trailing spaces in pipe_substring
        while (isspace((unsigned char)*pipe_substring)) {
            pipe_substring++;
        }
        char *p_end = pipe_substring + strlen(pipe_substring) - 1;
        while (p_end > pipe_substring && isspace((unsigned char)*p_end)) {
            p_end--;
        }
        p_end[1] = '\0';

        // If this segment is not empty
        if (strlen(pipe_substring) > 0) {
            if (cmd_count >= CMD_MAX) {
                // We already have 8 commands; user is trying for more
                clist->num = 0;  // Just a sanity clear
                return ERR_TOO_MANY_COMMANDS;
            }

            // Parse out the first token as exe, the remainder as args
            // We'll do a separate strtok_r with spaces
            char *saveptr2;
            char *token = strtok_r(pipe_substring, " \t", &saveptr2);
            if (token == NULL) {
                // Nothing but spaces or empty substring
                // We can skip or treat as empty, but let's skip
            } else {
                // Check length of exe
                if (strlen(token) >= EXE_MAX) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                strcpy(clist->commands[cmd_count].exe, token);

                // Now gather the rest as arguments
                // We'll build them in a local buffer, then copy over
                char argbuf[ARG_MAX];
                argbuf[0] = '\0';

                token = strtok_r(NULL, " \t", &saveptr2);
                while (token != NULL) {
                    // If adding this token + possible space overflows, error out
                    if ((strlen(argbuf) + strlen(token) + 2) >= ARG_MAX) {
                        return ERR_CMD_OR_ARGS_TOO_BIG;
                    }
                    // Add a space if not the first argument
                    if (argbuf[0] != '\0') {
                        strcat(argbuf, " ");
                    }
                    strcat(argbuf, token);

                    token = strtok_r(NULL, " \t", &saveptr2);
                }

                // Copy the final argument string
                strcpy(clist->commands[cmd_count].args, argbuf);

                // We successfully parsed one command
                cmd_count++;
            }
        }

        // Advance to next pipe_substring
        pipe_substring = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    // Store how many commands we ended up with
    clist->num = cmd_count;

    // If we still have 0 commands, e.g. user typed all spaces or something
    if (cmd_count == 0) {
        return WARN_NO_CMDS;
    }

    return OK;
}
