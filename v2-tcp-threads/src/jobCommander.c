#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "variables.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        if(argc == 1) {
            printf("\ncommander:missing arguments!!\n");
        }
        fprintf(stderr, "Use ---> %s [serverName] [portNum] [jobCommanderInputCommand]\n", argv[0]);
        return 1;
    }

    char *serverName = argv[1];
    int portNum = atoi(argv[2]);
    char write_arg[max_command_size];

    if(argc >= 4) { //formats the command-parameter string to be passed into 
        strcpy(write_arg,argv[3]);
        for(int i = 4;i < argc && argv[i]!=NULL;i++) {
            sprintf(write_arg, "%s %s", write_arg, argv[i]);
        }
    }
    printf("\n");

    struct hostent *server = gethostbyname(serverName);
    if (server == NULL) {
        fprintf(stderr, "Error: no such host\n");
        return 1;
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    write(client_socket, write_arg, strlen(write_arg));

    char buffer[max_command_size];
    while (1) { //reading responses from the server until it gets "close" string
        buffer[0] = '\0';
        int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            if (strcmp(buffer, "close") == 0) {
                break;
            }
            printf("%s\n", buffer);
        } else if (bytes_received == 0) {
            break;
        } else {
            perror("read");
            break;
        }
    }
    close(client_socket);
    return 0;
}
