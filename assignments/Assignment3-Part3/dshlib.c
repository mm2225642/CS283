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
 * HELPER: clear_cmd_buff()
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
    return OK;
}

/* ---------------------------------------------------------------------------
 * HELPER: alloc_cmd_buff()
 *  (Not strictly needed if you prefer to do your own memory logic in build_cmd_buff)
 *  Shown here to match the function signature from dshlib.h
 * --------------------------------------------------------------------------- */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);
    return OK;
}

/* ---------------------------------------------------------------------------
 * HELPER: free_cmd_buff()
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
    return OK;
}

/* ---------------------------------------------------------------------------
 * HELPER: close_cmd_buff()
 *  (Rarely used—some skeleton code references it. We can treat it like free_cmd_buff).
 * --------------------------------------------------------------------------- */
int close_cmd_buff(cmd_buff_t *cmd_buff)
{
    return free_cmd_buff(cmd_buff);
}

/* ---------------------------------------------------------------------------
 * HELPER: free_cmd_list()
 *  Iterates through each cmd_buff in the list, freeing its memory.
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
 *  Tokenizes a single command string (no pipes) by whitespace.
 *  - Allocates a copy of cmd_line in cmd_buff->_cmd_buffer.
 *  - Splits on spaces and sets cmd_buff->argv[] accordingly.
 *  - Updates cmd_buff->argc to reflect the # of tokens.
 * --------------------------------------------------------------------------- */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    // Ensure a clean starting point
    clear_cmd_buff(cmd_buff);

    // Allocate a copy of the command text
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
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate argv

    return OK;
}

/* ---------------------------------------------------------------------------
 * build_cmd_list()
 *  Splits the entire user input line by '|' to get multiple commands.
 *  Populates a command_list_t with each piped command in sequence.
 *  
 *  Return codes:
 *    OK                   => normal
 *    WARN_NO_CMDS        => user typed nothing
 *    ERR_TOO_MANY_COMMANDS => more than CMD_MAX commands (exceeding pipe limit)
 *    ERR_MEMORY          => problem with memory
 * --------------------------------------------------------------------------- */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Start fresh
    clist->num = 0;

    // Trim leading/trailing spaces (optional but helps).
    // Not strictly required, but can avoid empty commands
    // if the user typed weird spaces around the pipe.
    while (isspace((unsigned char)*cmd_line)) cmd_line++;
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end > cmd_line && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    // If user typed an empty line
    if (strlen(cmd_line) == 0)
    {
        return WARN_NO_CMDS;
    }

    // Duplicate the entire line for tokenizing by '|'
    char *temp_line = strdup(cmd_line);
    if (!temp_line)
    {
        return ERR_MEMORY;
    }

    // Tokenize by '|'
    char *saveptr;
    char *segment = strtok_r(temp_line, PIPE_STRING, &saveptr);
    while (segment != NULL)
    {
        if (clist->num >= CMD_MAX)
        {
            free(temp_line);
            return ERR_TOO_MANY_COMMANDS; 
        }

        // Build this command into clist->commands[clist->num]
        build_cmd_buff(segment, &clist->commands[clist->num]);
        clist->num++;

        // Next segment
        segment = strtok_r(NULL, PIPE_STRING, &saveptr);
    }

    free(temp_line);

    // If we somehow got no commands, warn
    if (clist->num == 0)
    {
        return WARN_NO_CMDS;
    }
    return OK;
}

/* ---------------------------------------------------------------------------
 * match_command()
 *  Identifies if the user typed a known built-in command.
 *  We'll only do simple matching: 'exit', 'dragon', 'cd' as examples.
 * --------------------------------------------------------------------------- */
Built_In_Cmds match_command(const char *input)
{
    if (!input)
    {
        return BI_NOT_BI;
    }
    if (strcmp(input, EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(input, "dragon") == 0)
    {
        return BI_CMD_DRAGON;
    }
    else if (strcmp(input, "cd") == 0)
    {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

/* ---------------------------------------------------------------------------
 * exec_built_in_cmd()
 *  Executes the "dragon", "cd", or "exit" built-in commands.
 *  For this assignment, we mostly care about 'exit' in the main loop.
 *  But we show how you'd handle 'cd' if you want to.
 * --------------------------------------------------------------------------- */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bic = match_command(cmd->argv[0]);
    switch (bic)
    {
        case BI_CMD_EXIT:
            // We usually handle "exit" in the main loop, 
            // so might not do anything here.
            return BI_CMD_EXIT;

        case BI_CMD_DRAGON:
            // Just a silly example built-in
            // You can do something fun here
            printf("You have summoned a dragon! (built-in cmd)\n");
            return BI_EXECUTED;

        case BI_CMD_CD:
            // cd <dir>
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

        default:
            return BI_NOT_BI;
    }
}

/* ---------------------------------------------------------------------------
 * execute_pipeline()
 *  Given a command_list_t with clist->num commands, sets up a pipeline:
 *    cmd1 | cmd2 | cmd3 | ...
 *  by creating pipes, forking, and using dup2() to connect them.
 *  
 *  Basic outline:
 *    1) Create (num-1) pipes if there are num commands.
 *    2) For each command i in [0..num-1], fork:
 *         - If child:
 *             - If not first command, dup2( the read end of previous pipe ) -> STDIN
 *             - If not last command, dup2( the write end of this pipe )     -> STDOUT
 *             - Close all pipe file descriptors
 *             - execvp(...)
 *         - If parent:
 *             - Keep track of child's pid
 *    3) Parent closes all pipe FDs and waitpid() for each child.
 * --------------------------------------------------------------------------- */
int execute_pipeline(command_list_t *clist)
{
    int num = clist->num;
    if (num < 1)
    {
        return WARN_NO_CMDS; 
    }

    // If there's just one command, check if it's a built-in first
    if (num == 1)
    {
        Built_In_Cmds bic = match_command(clist->commands[0].argv[0]);
        if (bic != BI_NOT_BI)
        {
            // run the built-in:
            if (bic == BI_CMD_EXIT)
            {
                // We might do nothing here and let the main loop handle exit
                return OK_EXIT; 
            }
            else
            {
                exec_built_in_cmd(&clist->commands[0]);
                return OK; 
            }
        }
    }

    // We have multiple commands or a single non-built-in command -> pipeline logic

    // We'll need to store child pids so we can wait on them
    pid_t pids[CMD_MAX] = {0};

    // Create (num-1) pipes
    int pipefd[2 * (num - 1)]; 
    for (int i = 0; i < num - 1; i++)
    {
        if (pipe(&pipefd[2*i]) < 0)
        {
            perror("pipe");
            return -1;
        }
    }

    // Fork each command
    for (int i = 0; i < num; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            return -1;
        }
        else if (pid == 0)
        {
            // Child
            // If NOT first command, we read from the pipe of the previous command
            if (i > 0)
            {
                dup2(pipefd[2*(i-1)], STDIN_FILENO);
            }

            // If NOT last command, we write to pipefd[2*i + 1]
            if (i < num - 1)
            {
                dup2(pipefd[2*i + 1], STDOUT_FILENO);
            }

            // Close ALL pipe fds in the child to avoid leaks
            for (int j = 0; j < 2*(num-1); j++)
            {
                close(pipefd[j]);
            }

            // Finally, exec the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            // If we get here, exec failed
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else
        {
            // Parent
            pids[i] = pid;
        }
    }

    // Parent closes all pipe fds
    for (int j = 0; j < 2*(num-1); j++)
    {
        close(pipefd[j]);
    }

    // Parent waits for each child
    int status = 0;
    for (int i = 0; i < num; i++)
    {
        waitpid(pids[i], &status, 0);
        // You can check WIFEXITED, WEXITSTATUS, etc. if you want
    }
    return OK;
}

/* ---------------------------------------------------------------------------
 * exec_local_cmd_loop()
 *  The main shell loop:
 *   1) Print prompt
 *   2) Read user input (fgets)
 *   3) If "exit", then quit
 *   4) Otherwise parse into command_list_t
 *   5) Call execute_pipeline()
 *   6) free_cmd_list()
 * 
 *  Returns 0 after user types exit or we get EOF on stdin.
 * --------------------------------------------------------------------------- */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];

    while (1)
    {
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // If EOF or error reading line, break
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL)
        {
            printf("\n");
            break; 
        }

        // Remove the trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Check for "exit" right away
        if (strcmp(cmd_line, EXIT_CMD) == 0)
        {
            printf("exiting...\n");
            break;
        }

        // Build our command list
        command_list_t clist;
        int rc = build_cmd_list(cmd_line, &clist);

        if (rc == WARN_NO_CMDS)
        {
            // user typed empty line or only spaces
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
            // Some other error: memory, parse issues, etc.
            // You can add debugging prints if you want
            continue;
        }

        // Execute the pipeline
        int exec_rc = execute_pipeline(&clist);
        // If the pipeline included a built-in 'exit', you might see OK_EXIT
        if (exec_rc == OK_EXIT)
        {
            printf("exiting...\n");
            free_cmd_list(&clist);
            break;
        }

        // Free after done
        free_cmd_list(&clist);
    }
    return OK; // shell is done
}
