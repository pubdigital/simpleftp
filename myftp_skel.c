#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include "debug.h"

#define BUFSIZE 512

/**
 * function: receive and analize the answer from the server
 * sd: socket descriptor
 * code: three leter numerical code to check if received
 * text: normally NULL but if a pointer if received as parameter
 *       then a copy of the optional message from the response
 *       is copied
 * return: result of code checking
 **/
bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // receive the answer
    recv_s = recv(sd, buffer, BUFSIZE, 0);
    DEBUG_PRINT(("recv_s: %d\n", recv_s));

    // error checking
    if (recv_s < 0) warn("error receiving data");
    if (recv_s == 0) errx(1, "connection closed by host");

    // parsing the code and message receive from the answer
    sscanf(buffer, "%d %[^\r\n]\r\n", &recv_code, message);
    printf("%d %s\n", recv_code, message);
    // optional copy of parameters
    if(text) strcpy(text, message);
    // boolean test for the code
    return (code == recv_code) ? true : false;
}

/**
 * function: send command formated to the server
 * sd: socket descriptor
 * operation: four letters command
 * param: command parameters
 **/
void send_msg(int sd, char *operation, char *param) {
    char buffer[BUFSIZE] = "";
    int bytes_sent;

    // command formating
    if (param != NULL)
        sprintf(buffer, "%s %s\r\n", operation, param);
    else
        sprintf(buffer, "%s\r\n", operation);

    // send command and check for errors
    bytes_sent = send(sd, buffer, strlen(buffer), 0);
    if(bytes_sent == -1)
    {
        perror("send error: ");
    }
}

/**
 * function: simple input from keyboard
 * return: input without ENTER key
 **/
char * read_input() {
    char *input = malloc(BUFSIZE);
    if (fgets(input, BUFSIZE, stdin)) {
        return strtok(input, "\n");
    }
    return NULL;
}

/**
 * function: login process from the client side
 * sd: socket descriptor
 **/
void authenticate(int sd) {
    char *input, desc[100];
    int code;

    // ask for user
    printf("username: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "USER", input);
    
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    if(!recv_msg(sd, 331, desc))
    {
        warn("Password request not received from server\n");
    }
    printf("Server says: %s\n", desc);

    // ask for password
    printf("passwd: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    if(!recv_msg(sd, 230, desc))
    {
        warn("Authentication status not received or incorrect from server\n");
    }
    printf("Server says: %s\n", desc);

}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server

    // check for the response

    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write
    file = fopen(file_name, "w");

    //receive the file



    // close the file
    fclose(file);

    // receive the OK from the server

}

/**
 * function: operation quit
 * sd: socket descriptor
 **/
void quit(int sd) {
    // send command QUIT to the client

    // receive the answer from the server

}

/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/
void operate(int sd) {
    char *input, *op, *param;

    while (true) {
        printf("Operation: ");
        input = read_input();
        if (input == NULL)
            continue; // avoid empty input
        op = strtok(input, " ");
        // free(input);
        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(sd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            quit(sd);
            break;
        }
        else {
            // new operations in the future
            printf("TODO: unexpected command\n");
        }
        free(input);
    }
    free(input);
}

bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

bool isValidPortNumber(char *portNumber)
{
    int port = atoi(portNumber);
    return (port < UINT16_MAX && port > 0);
}

unsigned int convert(char *st) {
  char *x;
  for (x = st ; *x ; x++) {
    // if its not a valid string (only digits), just return 0
    if (!isdigit(*x))
      return 0L;
  }
  return (strtoul(st, 0L, 10));
}


/**
 * Run with
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    int sd;
    struct sockaddr_in addr;
    char *server_ipaddr = argv[1];
    char *server_portn = argv[2];

    // arguments checking
    if(argc == 3 && isValidIpAddress(server_ipaddr) && isValidPortNumber(server_portn))
    {
        printf("arguments are valid :D\n");
    } else
    {
        DEBUG_PRINT(("args: %d\nserverIp: %s\nserverPort: %s\n", argc, server_ipaddr, server_portn));
        printf("Wrong numer of arguments or argument formatting\nExpecting: ./myftp x.x.x.x 2280\nWhere the first arg is the server ip addr and the second one is the server port\n");
        exit(EXIT_FAILURE);
    }

    // create socket and check for errors
    sd = socket(AF_INET, SOCK_STREAM, 0);
    DEBUG_PRINT(("socket descriptor [sd]: %d\n", sd));
    if(sd == -1)
    {
        printf("Error opening socket\n");
        exit(EXIT_FAILURE);
    }

    // set socket data 
    addr.sin_port = htons(convert(server_portn));
    DEBUG_PRINT(("addr.sin_port (without using htons): %u\n", convert(server_portn)));
    DEBUG_PRINT(("addr.sin_port: %u\n", addr.sin_port));
    addr.sin_addr.s_addr = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    // connect and check for errors
    // remember to use ports > 1024 if not, a binding error is likely to happen
    // binding error == port in use
    if(connect(sd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        printf("Connection with the server failed...\n");
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected to server %s\n", server_portn);   

    // if receive hello proceed with authenticate and operate if not warning
    if(!recv_msg(sd, 220, NULL))
    {
        warn("Hello message not received from server");
    }
    authenticate(sd);

    // close socket
    close(sd);

    return 0;
}