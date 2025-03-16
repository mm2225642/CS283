#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * exec_remote_cmd_loop(server_ip, port)
 *   The core remote client logic:
 *   1) connect to server
 *   2) read lines from user, send to server
 *   3) read all output from server until EOF_CHAR
 *   4) loop
 *   5) if user typed 'exit' or 'stop-server', also exit local client
 */
int exec_remote_cmd_loop(char *address, int port)
{
    // 1) Allocate buffers
    char *cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    char *rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!cmd_buff || !rsp_buff)
    {
        fprintf(stderr, "Error: cannot allocate memory\n");
        return client_cleanup(-1, cmd_buff, rsp_buff, ERR_MEMORY);
    }

    // 2) Start client
    int cli_socket = start_client(address, port);
    if (cli_socket < 0)
    {
        fprintf(stderr, "Error: cannot connect to server\n");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    // 3) Command loop
    while (1)
    {
        // a) Print local prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // b) Read user input
        if (!fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin))
        {
            // EOF or error
            printf("\n");
            break;
        }
        // remove newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // If empty, skip
        if (strlen(cmd_buff) == 0) {
            continue;
        }

        // c) Send the command + null terminator
        ssize_t to_send = strlen(cmd_buff) + 1;
        if (send(cli_socket, cmd_buff, to_send, 0) < 0)
        {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        // d) Now read server response until we see RDSH_EOF_CHAR
        while (1)
        {
            ssize_t recvd = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ, 0);
            if (recvd < 0)
            {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            if (recvd == 0)
            {
                // server closed
                fprintf(stderr, RCMD_SERVER_EXITED "\n");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
            }

            // Print the chunk we got
            printf("%.*s", (int)recvd, rsp_buff);
            fflush(stdout);

            // check if the last byte is EOF
            if (rsp_buff[recvd - 1] == RDSH_EOF_CHAR)
            {
                // done with this command's output
                break;
            }
        }

        // e) If user typed "exit" or "stop-server", done
        if (strcmp(cmd_buff, "exit") == 0 || strcmp(cmd_buff, "stop-server") == 0)
        {
            break;
        }
    }

    // cleanup
    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}

/*
 * start_client()
 */
int start_client(char *server_ip, int port)
{
    int cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0)
    {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // build server address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // convert ip
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    // connect
    if (connect(cli_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

/*
 * client_cleanup()
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
{
    if (cli_socket > 0) {
        close(cli_socket);
    }
    free(cmd_buff);
    free(rsp_buff);
    return rc;
}
