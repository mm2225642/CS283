#ifndef __DSHLIB_H__
#define __DSHLIB_H__

#include <stdbool.h> // needed for bool

// Command structure sizes
#define EXE_MAX 64
#define ARG_MAX 256
#define CMD_MAX 8
#define CMD_ARGV_MAX (CMD_MAX + 1)
#define SH_CMD_MAX (EXE_MAX + ARG_MAX)

// Container for a single command and its arguments
typedef struct cmd_buff {
    int  argc;
    char *argv[CMD_ARGV_MAX];
    char *_cmd_buffer;

    // (New assignment) Redirection via file paths (if you want):
    char *input_file;  
    char *output_file; 
    bool append_mode;   // if >> is used

    // (Old assignment) If you prefer storing actual FDs:
    int  redirect_in_fd;  
    int  redirect_out_fd;
} cmd_buff_t;

// Container for a pipeline (a series of commands)
typedef struct command_list {
    int num;
    cmd_buff_t commands[CMD_MAX];
} command_list_t;

// Some #defines
#define SPACE_CHAR  ' '
#define PIPE_CHAR   '|'
#define PIPE_STRING "|"

// Shell constants
#define SH_PROMPT "dsh4> "
#define EXIT_CMD  "exit"
#define RC_SC     99
#define EXIT_SC   100

// Return codes
#define OK                       0
#define WARN_NO_CMDS            -1
#define ERR_TOO_MANY_COMMANDS   -2
#define ERR_CMD_OR_ARGS_TOO_BIG -3
#define ERR_CMD_ARGS_BAD        -4
#define ERR_MEMORY              -5
#define ERR_EXEC_CMD            -6
#define OK_EXIT                 -7

// Prototypes
int alloc_cmd_buff(cmd_buff_t *cmd_buff);
int free_cmd_buff(cmd_buff_t *cmd_buff);
int clear_cmd_buff(cmd_buff_t *cmd_buff);
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff);
int close_cmd_buff(cmd_buff_t *cmd_buff);
int build_cmd_list(char *cmd_line, command_list_t *clist);
int free_cmd_list(command_list_t *cmd_lst);

// Built-In command stuff
typedef enum {
    BI_CMD_EXIT,
    BI_CMD_DRAGON,
    BI_CMD_CD,
    BI_CMD_RC,         // old extra credit
    BI_CMD_STOP_SVR,   // new assignment
    BI_NOT_BI,
    BI_EXECUTED
} Built_In_Cmds;

Built_In_Cmds match_command(const char *input);
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd);

// Main execution context
int exec_local_cmd_loop();
int exec_cmd(cmd_buff_t *cmd);
int execute_pipeline(command_list_t *clist);

// Output constants
#define CMD_OK_HEADER       "PARSED COMMAND LINE - TOTAL COMMANDS %d\n"
#define CMD_WARN_NO_CMD     "warning: no commands provided\n"
#define CMD_ERR_PIPE_LIMIT  "error: piping limited to %d commands\n"
#define BI_NOT_IMPLEMENTED  "not implemented"

#endif
