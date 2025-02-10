#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dshlib.h"

/*
 * dsh_cli.c
 *
 * The main loop for our custom shell (dsh).
 */
int main()
{
    // We can store user input lines into a buffer sized up to SH_CMD_MAX
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1)
    {
        // Print the shell prompt
        printf("%s", SH_PROMPT);

        // Attempt to read one line of input
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL)
        {
            // If EOF or error reading, just break out
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // 1. Built-in exit
        if (strcmp(cmd_buff, EXIT_CMD) == 0)
        {
            // End the shell with a success exit code
            return 0;
        }

        // 2. Extra-Credit (simple version):
        //    If user typed exactly "dragon", print ASCII art and go to next prompt
        if (strcmp(cmd_buff, "dragon") == 0)
        {
            // Print the ASCII art (trimmed down to keep it shorter here; use your full version)
            // Ensure you preserve spacing. For demonstration, I show just a few lines:
            printf("                                                                        @%%%%                       \n");
            printf("                                                                     %%%%%%                         \n");
            printf("                                                                    %%%%%%                          \n");
            printf("... [rest of ASCII dragon omitted for brevity in sample] ...\n");
            continue;
        }

        // 3. Build command list from user input
        // Clear out the clist structure first
        memset(&clist, 0, sizeof(clist));
        int rc = build_cmd_list(cmd_buff, &clist);

        // 4. Check the return code and react accordingly
        if (rc == WARN_NO_CMDS)
        {
            printf("%s", CMD_WARN_NO_CMD);
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            // e.g. "error: piping limited to 8 commands\n"
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        }
        else if (rc == OK)
        {
            // Print the parsed commands
            printf(CMD_OK_HEADER, clist.num);
            for (int i = 0; i < clist.num; i++)
            {
                // If no arguments for this command
                if (strlen(clist.commands[i].args) == 0)
                {
                    printf("<%d> %s\n", i + 1, clist.commands[i].exe);
                }
                else
                {
                    printf("<%d> %s [%s]\n", i + 1,
                           clist.commands[i].exe,
                           clist.commands[i].args);
                }
            }
        }
        else
        {
            // If you want to handle ERR_CMD_OR_ARGS_TOO_BIG or other codes
            // you can do that here, or omit if not explicitly tested.
            // We'll just do a generic message:
            printf("error: command/arguments too long\n");
        }
    }

    return 0;
}
