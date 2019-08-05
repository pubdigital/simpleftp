#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFSIZE 512
#define BACKLOG 5

#define MSG_550 "550 %s: no such file or directory\r\n"

/**
 * function: receive and analize the answer from the server
 * sd: socket descriptor
 * code: three leter numerical code to check if received
 * text: normally NULL but if a pointer is received as parameter
 *       then a copy of the optional message from the response
 *       is copied
 * return: result of code checking
 **/
bool recv_msg(int sd, int code, char *text) {
    char buffer[BUFSIZE], message[BUFSIZE];
    int recv_s, recv_code;

    // receive the answer
    recv_s = recv(sd, buffer, BUFSIZE, 0);

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

    // command formating
    if (param != NULL)
        sprintf(buffer, "%s %s\r\n", operation, param);
    else
        sprintf(buffer, "%s\r\n", operation);

    // send command and check for errors
    if(send(sd, buffer, sizeof(buffer), 0) < 0) {
        printf("ERROR: failed to send command.\n");
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
    printf("Username: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "USER", input);
    
    // release memory
    free(input);

    // wait to receive password requirement and check for errors
    code = 331;
    if(recv_msg(sd, code, desc) != true) {
        printf("Message code error.\n");
    }

    // ask for password
    printf("Password: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer, process it and check for errors
    code = 230;
    if(!recv_msg(sd, code, desc)) {
         exit(1);
    }
}

/**
 * function: operation get
 * controlfd: socket descriptor for control connection
 * datafd: socket descriptor for data connection 
 * file_name: file name to get from the server
 **/
void get(int controlfd, int datafd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(controlfd, "RETR", file_name);

    // check for the response
    if(recv_msg(controlfd, 550, NULL)) {
        return;
    }

    // receive the message "200 PORT command succesful"
    recv_msg(controlfd, 200, NULL);

    // parsing the file size from the answer received
    sscanf(buffer, "150 Opening BINARY mode data connection for %*s (%d bytes)", &f_size);

    // open the file to write
    file = fopen(file_name, "w");

    //receive the file
    while(1) {
        recv_s = read(datafd, desc, r_size);

        if(recv_s > 0) {
            fwrite(desc, 1, recv_s, file);
        }

        if(recv_s < r_size) {
            break;
        }
    }
    
    // close the file
    fclose(file);

    // receive the OK from the server
    recv_msg(controlfd, 226, NULL);
}

/**
 * function: operation put
 * controlfd: socket descriptor for control connection
 * datafd: socket descriptor for data connection 
 * file_name: file name to write in the server
 **/
void put(int controlfd, int datafd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    int bRead;
    FILE *file;

    // open the file to read and check for errors
    if((file = fopen(file_name, "r")) == NULL) {
        printf(MSG_550, file_name);
        return;
    }

    // send STOR command to the server
    send_msg(controlfd, "STOR", file_name);
    
    // receive the message "200 PORT command succesful"
    recv_msg(controlfd, 200, NULL);

    // receive message with file information
    recv_msg(controlfd, 150, NULL);

    // send the file
    while(1) {
        bRead = fread(buffer, 1, BUFSIZE, file);
        if (bRead > 0) {
            send(datafd, buffer, bRead, 0);
            // important delay for avoid problems with buffer size
            sleep(1);
        }
        if(bRead < BUFSIZE) break;
    }
    
    // close the file
    fclose(file);

    // receive the OK from the server
    recv_msg(controlfd, 226, NULL);
}

/**
 * function: operation quit
 * sd: socket descriptor
 **/
void quit(int sd) {
    // send command QUIT to the client
    send_msg(sd, "QUIT", NULL);

    // receive the answer from the server
    recv_msg(sd, 221, NULL);
}

/**
 * function: make all operations (get|quit|put)
 * controlfd: socket descriptor for control connection
 * datafd: socket descriptor for data connection 
 **/
void operate(int controlfd, int datafd) {
    char *input, *op, *param;

    while(true) {
        printf("Operation: ");
        input = read_input();
        if (input == NULL)
            continue; // avoid empty input
        op = strtok(input, " ");
        // free(input);
        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(controlfd, datafd, param);
        }
        else if(strcmp(op, "quit") == 0) {
            close(datafd);
            quit(controlfd);
            break;
        }
        else if(strcmp(op, "put") == 0) {
            param = strtok(NULL, " ");
            put(controlfd, datafd, param);
        }
        else {
            printf("TODO: unexpected command\n");
        }
        free(input);
    }
    free(input);
}

/**
 * function: get IP and port number from fd
 * fd: socket descriptor
 * ip: string where IP address is saved
 * port: int where port number is saved
 **/
void get_ip_port(int fd, char *ip, int *port) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getsockname(fd, (struct sockaddr*) &addr, &len);
    sprintf(ip, "%s", inet_ntoa(addr.sin_addr));

    *port = (uint16_t)ntohs(addr.sin_port);
}

/**
 * function: port conversion to two numbers we will use later in PORT command
 * port: the port number we want to convert
 * n5: fifth number in PORT command
 * n6: sixth number in PORT command
 **/
void convert(uint16_t port, int *n5, int *n6) {
    int i = 0, x = 1, temp = 0;
    *n5 = 0;
    *n6 = 0;    

    for(i = 0; i < 8; i++) {
        temp = port & x;
        *n6 = (*n6)|(temp);
        x = x << 1; 
    }

    port = port >> 8;
    x = 1;

    for(i = 8; i < 16; i++) {
        temp = port & x;
        *n5 = ((*n5)|(temp));
        x = x << 1; 
    }
}

/**
 * function: construction of the string used in PORT command
 * str: string to be used in PORT command
 * ip: IP address to be used in PORT command
 * n5: fifth number in PORT command
 * n6: sexth number in PORT command
 **/
void get_port_string(char *str, char *ip, int n5, int n6){
    int i = 0;
    char ip_temp[1024];
    strcpy(ip_temp, ip);

    for(i = 0; i < strlen(ip); i++) {
        if(ip_temp[i] == '.') {
            ip_temp[i] = ',';
        }
    }

    sprintf(str, "%s,%d,%d", ip_temp, n5, n6);
}

/**
 * Run with
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    int server_port, controlfd, listenfd, datafd, n5, n6, controlPort;
    uint16_t port;
    struct sockaddr_in serverAddr, data_addr;
    struct hostent *hostName;
    char ip[50], str[BUFSIZE];


    // arguments checking
    /* client must receive IP address + port where server is listening */
    if(argc != 3) {
    	printf("ERROR: enter IP address and port.\n");
    	return -1;
    }

    if((hostName = gethostbyname(argv[1])) == NULL) {
        perror("gethostbyname");
        exit(1);
    }

    //get server port
    sscanf(argv[2], "%d", &server_port);

    // create CONTROL SOCKET and check for errors
    controlfd = socket(AF_INET, SOCK_STREAM, 0);	/* creates the socket and returns its file descriptor */
    if(controlfd < 0) {
    	printf("ERROR: failed to create socket.\n");
    	return -1;
    }
    
    // set socket data (CONTROL CONNECTION)
    serverAddr.sin_family = AF_INET; /* Internet Protocol address family */
    serverAddr.sin_port = htons(server_port); // server-side port
    serverAddr.sin_addr = *((struct in_addr *)hostName->h_addr);
    memset(&(serverAddr.sin_zero), '\0', 8);  


    // connect and check for errors
    if(connect(controlfd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) < 0) {
    	printf("ERROR: failed to connect.\n");
    	return -1;
    }

    // if receive hello proceed with authenticate and operate if not warning
    if(!recv_msg(controlfd, 220, NULL)) {
    	warn("ERROR: hello message was not received.\n");
    } else {
        authenticate(controlfd);

        // set socket data (DATA CONNECTION)
        listenfd = socket(AF_INET, SOCK_STREAM, 0);

        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(0); // random port
        data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&(data_addr.sin_zero), '\0', 8);

        bind(listenfd, (struct sockaddr*) &data_addr, sizeof(data_addr));
        listen(listenfd, BACKLOG);

        // get ip address and my control port
        get_ip_port(controlfd, ip, (int *) &controlPort);

        // get data-connection ip and port from listenfd
        get_ip_port(listenfd, str, (int *) &port);

        convert(port, &n5, &n6);

        // get client-side data port and IP as a string
        get_port_string(str, ip, n5, n6);

        // send PORT command to server
        send_msg(controlfd, "PORT", str);
        bzero(str, (int)sizeof(str));

        datafd = accept(listenfd, (struct sockaddr*) NULL, NULL);

        operate(controlfd, datafd);

        close(datafd);
    }

    // close socket
    close(controlfd);

    return 0;
}