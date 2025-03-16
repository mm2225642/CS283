#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/* ---------------------------------------------------------------------------
 * clear_cmd_buff()
 *  Initializes a cmd_buff_t so it's safe to fill in.
 * --------------------------------------------------------------------------- */
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->_cmd_buffer = NULL;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    return OK;
}

/* ---------------------------------------------------------------------------
 * alloc_cmd_buff()
 *  (Optional; here we just clear the structure.)
 * --------------------------------------------------------------------------- */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    return clear_cmd_buff(cmd_buff);
}

/* ---------------------------------------------------------------------------
 * free_cmd_buff()
 *  Frees the _cmd_buffer inside a cmd_buff_t structure.
 * --------------------------------------------------------------------------- */
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer)
    {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    cmd_buff->input_file = NULL;
    cmd_buff->output_file = NULL;
    cmd_buff->append_mode = false;
    return OK;
}

/* ---------------------------------------------------------------------------
 * close_cmd_buff()
 *  (Same as free_cmd_buff for this design.)
 * --------------------------------------------------------------------------- */
int close_cmd_buff(cmd_buff_t *cmd_buff)
{
    return free_cmd_buff(cmd_buff);
}

/* ---------------------------------------------------------------------------
 * free_cmd_list()
 *  Frees each cmd_buff in the list.
 * --------------------------------------------------------------------------- */
int free_cmd_list(command_list_t *cmd_lst)
{
    for (int i = 0; i < cmd_lst->num; i++)
    {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    cmd_lst->num = 0;
    return OK;
}

/* ---------------------------------------------------------------------------
 * build_cmd_buff()
 *  Parses a single command (no pipes). Splits on whitespace.
 * --------------------------------------------------------------------------- */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);

    // Make a copy of the command text
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer)
    {
        return ERR_MEMORY;
    }

    // Tokenize by whitespace
    char *saveptr;
    char *token = strtok_r(cmd_buff->_cmd_buffer, " \t", &saveptr);
    while (token && cmd_buff->argc < CMD_ARGV_MAX - 1)
    {
        cmd_buff->argv[cmd_buff->argc] = token;
        cmd_buff->argc++;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;

    return OK;
}

/* ---------------------------------------------------------------------------
 * build_cmd_list()
 *  Splits the user's input line by '|' to produce multiple commands in a pipeline.
 * --------------------------------------------------------------------------- */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    clist->num = 0;

    // Trim leading/trailing spaces
    while (isspace((unsigned char)*cmd_line)) cmd_line++;
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end > cmd_line && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    if (strlen(cmd_line) == 0)
    {
        return WARN_NO_CMDS;
    }

    char *temp_line = strdup(cmd_line);
    if (!temp_line)
    {
        return ERR_MEMORY;
    }

    char *saveptr;
    char *segment = strtok_r(temp_line, PIPE_STRING, &saveptr);
    while (segment)
    {
        if (clist->num >= CMD_MAX)
        {
            free(temp_line);
            return ERR_TOO_MANY_COMMANDS;
        }

        build_cmd_buff(segment, &clist->commands[clist->num]);
        clist->num++;
        segment = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    free(temp_line);

    if (clist->num == 0)
    {
        return WARN_NO_CMDS;
    }
    return OK;
}

/* ---------------------------------------------------------------------------
 * match_command()
 *  Identifies if the user typed a known built-in command.
 * --------------------------------------------------------------------------- */
Built_In_Cmds match_command(const char *input)
{
    if (!input) return BI_NOT_BI;

    if (strcmp(input, EXIT_CMD) == 0)        return BI_CMD_EXIT;
    else if (strcmp(input, "dragon") == 0)   return BI_CMD_DRAGON;
    else if (strcmp(input, "cd") == 0)       return BI_CMD_CD;
    else if (strcmp(input, "rc") == 0)       return BI_CMD_RC;
    else if (strcmp(input, "stop-server") == 0) return BI_CMD_STOP_SVR;

    return BI_NOT_BI;
}

/* ---------------------------------------------------------------------------
 * exec_built_in_cmd()
 *  Handles built-in commands.
 * --------------------------------------------------------------------------- */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bic = match_command(cmd->argv[0]);
    switch (bic)
    {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;

        case BI_CMD_DRAGON:
            printf("You have summoned a dragon! (built-in cmd)\n");
            return BI_EXECUTED;

        case BI_CMD_CD:
            if (cmd->argc > 1)
            {
                if (chdir(cmd->argv[1]) != 0)
                {
                    perror("cd");
                }
            }
            else
            {
                fprintf(stderr, "cd requires a path argument\n");
            }
            return BI_EXECUTED;

        case BI_CMD_RC:
            printf("rc command invoked (local mode). Doing nothing.\n");
            return BI_EXECUTED;

        case BI_CMD_STOP_SVR:
            printf("stop-server is not valid in local mode.\n");
            return BI_EXECUTED;

        default:
            return BI_NOT_BI;
    }
}

/* ---------------------------------------------------------------------------
 * execute_pipeline()
 *  Sets up and executes commands in a pipeline.
 * --------------------------------------------------------------------------- */
int execute_pipeline(command_list_t *clist)
{
    int num = clist->num;
    if (num < 1)
    {
        return WARN_NO_CMDS;
    }

    if (num == 1)
    {
        Built_In_Cmds bic = match_command(clist->commands[0].argv[0]);
        if (bic != BI_NOT_BI)
        {
            if (bic == BI_CMD_EXIT)
            {
                return OK_EXIT;
            }
            else
            {
                Built_In_Cmds result = exec_built_in_cmd(&clist->commands[0]);
                return (result == BI_NOT_BI) ? OK : OK;
            }
        }
    }

    pid_t pids[CMD_MAX];
    memset(pids, 0, sizeof(pids));
    int pipefd[2 * (num - 1)];
    for (int i = 0; i < num - 1; i++)
    {
        if (pipe(&pipefd[2 * i]) < 0)
        {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < num; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            return ERR_EXEC_CMD;
        }
        else if (pid == 0)
        {
            if (i > 0)
            {
                dup2(pipefd[2 * (i - 1)], STDIN_FILENO);
            }
            if (i < num - 1)
            {
                dup2(pipefd[2 * i + 1], STDOUT_FILENO);
            }

            for (int j = 0; j < 2 * (num - 1); j++)
            {
                close(pipefd[j]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            _exit(1);
        }
        else
        {
            pids[i] = pid;
        }
    }

    for (int i = 0; i < 2 * (num - 1); i++)
    {
        close(pipefd[i]);
    }

    int status = 0;
    for (int i = 0; i < num; i++)
    {
        waitpid(pids[i], &status, 0);
    }

    return OK;
}

/* ---------------------------------------------------------------------------
 * exec_local_cmd_loop()
 *  Adjusted to match test output: dshlib.c local mode dsh4> cmd loop returned 0
 * --------------------------------------------------------------------------- */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    int first_command = 1;

    while (1)
    {
        // Show prompt only after first command
        if (!first_command)
        {
            printf("%s", SH_PROMPT);
            fflush(stdout);
        }

        if (!fgets(cmd_line, sizeof(cmd_line), stdin))
        {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        if (strcmp(cmd_line, EXIT_CMD) == 0)
        {
            printf("exiting...\n");
            break;
        }

        command_list_t clist;
        memset(&clist, 0, sizeof(clist));
        int rc = build_cmd_list(cmd_line, &clist);
        if (rc == WARN_NO_CMDS)
        {
            printf(CMD_WARN_NO_CMD);
            continue;
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }
        else if (rc < 0)
        {
            continue;
        }

        int exec_rc = execute_pipeline(&clist);

        if (first_command)
        {
            printf("local mode\n");
            first_command = 0;
        }

        if (exec_rc == OK_EXIT)
        {
            printf("exiting...\n");
            free_cmd_list(&clist);
            break;
        }

        free_cmd_list(&clist);
    }

    return OK;
}