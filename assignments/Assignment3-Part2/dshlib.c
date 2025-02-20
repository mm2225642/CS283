#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"

// Global variable to store the last return code
static int last_rc = 0;

// Helper function to trim leading and trailing spaces
static void trim_spaces(char *str) {
    char *start = str;
    char *end;

    while (isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

// Parse cmd_line into cmd_buff_t, handling quoted strings and spaces
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (!cmd_line || !cmd_buff) return ERR_MEMORY;

    clear_cmd_buff(cmd_buff);
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;

    trim_spaces(cmd_buff->_cmd_buffer);
    if (strlen(cmd_buff->_cmd_buffer) == 0) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
        return WARN_NO_CMDS;
    }

    char *token;
    bool in_quotes = false;
    char *ptr = cmd_buff->_cmd_buffer;
    cmd_buff->argc = 0;

    while (*ptr && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        while (isspace((unsigned char)*ptr) && !in_quotes) ptr++;
        if (!*ptr) break;

        if (*ptr == '"') {
            in_quotes = !in_quotes;
            token = ++ptr;
        } else {
            token = ptr;
        }

        while (*ptr && ((in_quotes && *ptr != '"') || (!in_quotes && !isspace((unsigned char)*ptr)))) {
            ptr++;
        }

        if (*ptr == '"') {
            in_quotes = false;
            *ptr = '\0';
            ptr++;
        } else if (*ptr) {
            *ptr = '\0';
            ptr++;
        }

        cmd_buff->argv[cmd_buff->argc++] = token;
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (!cmd_buff) return ERR_MEMORY;
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

Built_In_Cmds match_command(const char *input) {
    if (!input) return BI_NOT_BI;
    if (strcmp(input, EXIT_CMD) == 0) return BI_CMD_EXIT;
    if (strcmp(input, "cd") == 0) return BI_CMD_CD;
    if (strcmp(input, "rc") == 0) return BI_CMD_RC; // Added for extra credit
    return BI_NOT_BI;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0) return BI_NOT_BI;
    
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
    switch (cmd_type) {
        case BI_CMD_EXIT:
            last_rc = 0;
            return BI_EXECUTED;
        case BI_CMD_CD:
            if (cmd->argc == 1) {
                last_rc = 0; // Success (no-op)
                return BI_EXECUTED;
            } else if (cmd->argc == 2) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd failed");
                    last_rc = errno; // Store errno on failure
                    return BI_EXECUTED; // Still executed, even if failed
                }
                last_rc = 0; // Success
                return BI_EXECUTED;
            }
            return BI_NOT_BI;
        case BI_CMD_RC: // Extra credit: print last return code
            if (cmd->argc == 1) {
                printf("%d\n", last_rc);
                last_rc = 0; // Reset after displaying
                return BI_EXECUTED;
            }
            return BI_NOT_BI;
        default:
            return BI_NOT_BI;
    }
}

int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        last_rc = errno;
        return ERR_EXEC_CMD;
    } else if (pid == 0) { // Child
        if (execvp(cmd->argv[0], cmd->argv) == -1) {
            int err = errno;
            if (err == ENOENT) {
                fprintf(stderr, "%s", CMD_ERR_NOT_FOUND);
            } else if (err == EACCES) {
                fprintf(stderr, "%s", CMD_ERR_PERM_DENIED);
            } else {
                perror("execvp failed");
            }
            exit(err); // Exit with errno
        }
    } else { // Parent
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            last_rc = WEXITSTATUS(status);
            if (last_rc != 0) {
                if (last_rc != ENOENT && last_rc != EACCES) {
                    printf("%s", CMD_ERR_EXECUTE); // Generic error for non-specific cases
                }
                return ERR_EXEC_CMD;
            }
        } else {
            last_rc = -1; // Signal or other failure
            printf("%s", CMD_ERR_EXECUTE);
            return ERR_EXEC_CMD;
        }
    }
    return OK;
}

int exec_local_cmd_loop() {
    char input[SH_CMD_MAX];
    cmd_buff_t cmd;
    alloc_cmd_buff(&cmd);

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        int parse_rc = build_cmd_buff(input, &cmd);
        if (parse_rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            last_rc = 0;
            continue;
        } else if (parse_rc != OK) {
            last_rc = parse_rc;
            continue;
        }

        Built_In_Cmds bi_result = exec_built_in_cmd(&cmd);
        if (bi_result == BI_EXECUTED) {
            if (match_command(cmd.argv[0]) == BI_CMD_EXIT) {
                free_cmd_buff(&cmd);
                return OK_EXIT;
            }
            continue;
        }

        exec_cmd(&cmd);
    }

    free_cmd_buff(&cmd);
    return OK;
}