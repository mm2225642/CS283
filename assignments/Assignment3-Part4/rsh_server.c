#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * start_server(ifaces, port, is_threaded)
 */
int start_server(char *ifaces, int port, int is_threaded)
{
    int svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0)
    {
        return svr_socket;
    }

    int rc = process_cli_requests(svr_socket);
    stop_server(svr_socket);
    return rc;
}

/*
 * stop_server(svr_socket)
 */
int stop_server(int svr_socket)
{
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 */
int boot_server(char *ifaces, int port)
{
    int svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0)
    {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    int enable = 1;
    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (listen(svr_socket, 20) < 0)
    {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 */
int process_cli_requests(int svr_socket)
{
    while (1)
    {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int cli_socket = accept(svr_socket, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_socket < 0)
        {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        int rc = exec_client_requests(cli_socket);
        close(cli_socket);

        if (rc == OK_EXIT)
        {
            printf(RCMD_MSG_SVR_STOP_REQ "\n");
            return OK_EXIT;
        }
        else if (rc < 0)
        {
            return rc;
        }
    }
    return OK;
}

/*
 * exec_client_requests(cli_socket)
 */
int exec_client_requests(int cli_socket)
{
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff)
    {
        return ERR_RDSH_SERVER;
    }

    while (1)
    {
        ssize_t total_read = 0;
        int got_null_terminator = 0;
        while (!got_null_terminator)
        {
            ssize_t recvd = recv(cli_socket, io_buff + total_read, RDSH_COMM_BUFF_SZ - total_read, 0);
            if (recvd < 0)
            {
                perror("recv");
                free(io_buff);
                return ERR_RDSH_COMMUNICATION;
            }
            if (recvd == 0)
            {
                printf(RCMD_MSG_CLIENT_EXITED "\n");
                free(io_buff);
                return OK;
            }

            for (int i = 0; i < recvd; i++)
            {
                if (io_buff[total_read + i] == '\0')
                {
                    got_null_terminator = 1;
                    break;
                }
            }
            total_read += recvd;

            if (total_read >= RDSH_COMM_BUFF_SZ - 1)
            {
                io_buff[RDSH_COMM_BUFF_SZ - 1] = '\0';
                break;
            }
        }

        command_list_t clist;
        memset(&clist, 0, sizeof(clist));
        int rc = build_cmd_list(io_buff, &clist);
        if (rc == WARN_NO_CMDS)
        {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            continue;
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), CMD_ERR_PIPE_LIMIT, CMD_MAX);
            send_message_string(cli_socket, tmp);
            continue;
        }
        else if (rc < 0)
        {
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "error parsing command: %d\n", rc);
            send_message_string(cli_socket, tmp);
            continue;
        }

        if (clist.num > 0)
        {
            char *first_cmd = clist.commands[0].argv[0];
            if (first_cmd)
            {
                if (strcmp(first_cmd, "exit") == 0)
                {
                    send_message_string(cli_socket, "Bye!\n");
                    free_cmd_list(&clist);
                    free(io_buff);
                    return OK;
                }
                else if (strcmp(first_cmd, "stop-server") == 0)
                {
                    send_message_string(cli_socket, "Stopping the server...\n");
                    free_cmd_list(&clist);
                    free(io_buff);
                    return OK_EXIT;
                }
            }
        }

        int exit_code = rsh_execute_pipeline(cli_socket, &clist);

        // Send EOF character after pipeline execution (Professor's code)
        rc = send_message_eof(cli_socket);
        if (rc != OK)
        {
            printf(CMD_ERR_RDSH_COMM);
            free(io_buff);
            close(cli_socket);
            return ERR_RDSH_COMMUNICATION;
        }

        // Optional: Send exit code (not required by prof but keeps original behavior)
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "cmd finished (rc=%d)\n", exit_code);
        send_message_string(cli_socket, tmp);

        free_cmd_list(&clist);
    }

    free(io_buff);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *   Professor's exact implementation
 */
int send_message_eof(int cli_socket)
{
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len)
    {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * send_message_string(cli_socket, buff)
 */
int send_message_string(int cli_socket, char *buff)
{
    size_t msg_len = strlen(buff);
    ssize_t sent = send(cli_socket, buff, msg_len, 0);
    if (sent < 0 || (size_t)sent != msg_len)
    {
        return ERR_RDSH_COMMUNICATION;
    }
    return send_message_eof(cli_socket);
}

/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *   Updated with Professor's dup2 for last command
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist)
{
    if (clist->num < 1)
        return 0;

    int i;
    int pipes[CMD_MAX - 1][2] = {{0}};
    pid_t pids[CMD_MAX] = {0};
    int status[CMD_MAX] = {0};

    for (i = 0; i < clist->num - 1; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe");
            return ERR_RDSH_COMMUNICATION;
        }
    }

    for (i = 0; i < clist->num; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            if (i > 0)
            {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < clist->num - 1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            // Professor's code: For last command, redirect to cli_sock unless output_file exists
            if (i == clist->num - 1 && !clist->commands[i].output_file)
            {
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO); // Also redirect stderr to socket
            }

            for (int j = 0; j < clist->num - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            _exit(1);
        }
        else if (pid < 0)
        {
            perror("fork");
            return ERR_RDSH_COMMUNICATION;
        }
        else
        {
            pids[i] = pid;
        }
    }

    for (int j = 0; j < clist->num - 1; j++)
    {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }

    for (i = 0; i < clist->num; i++)
    {
        waitpid(pids[i], &status[i], 0);
    }

    int exit_code = WEXITSTATUS(status[clist->num - 1]);
    return exit_code;
}