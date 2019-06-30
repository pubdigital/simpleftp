#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 512
#define CMDSIZE 5
#define PARSIZE 100
#define BACKLOG 5  // maximum number of clients in queue

#define MSG_220 "220 srvFtp version 2.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"
#define MSG_200 "200 PORT command successful\r\n"
#define MSG_150 "150 Opening BINARY mode data connection for %s (%d bytes)\r\n"
#define MSG_150_2 "150 Opening BINARY mode data connection for %s\r\n"

/**
 * function: receive the commands from the client
 * sd: socket descriptor
 * operation: \0 if you want to know the operation received
 *            OP if you want to check a specific operation
 *            ex: recv_cmd(sd, "USER", param)
 * param: parameters involved in the operation
 * return: only useful if you want to check an operation
 *         ex: for login you need the seq USER PASS
 *             you can check if you receive first USER
 *             and then check if you receive PASS
 **/
bool recv_cmd(int sd, char *operation, char *param) {
    char buffer[BUFSIZE], *token;
    int recv_s;


    // receive the command in the buffer and check for errors
    recv_s = recv(sd, buffer, BUFSIZE, 0); /* receives from sd and stores in buffer */
    if (recv_s < 0) warn("error receiving data");
    if (recv_s == 0) errx(1, "connection closed by host");


    // expunge the terminator characters from the buffer
    buffer[strcspn(buffer, "\r\n")] = 0;

    // complex parsing of the buffer
    // extract command received in operation if not set \0
    // extract parameters of the operation in param if needed
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        warn("not valid ftp command");
        return false;
    } else {
        if (operation[0] == '\0') strcpy(operation, token);
        if (strcmp(operation, token)) {
            warn("abnormal client flow: did not send %s command", operation);
            return false;
        }
        token = strtok(NULL, " ");
        if (token != NULL) strcpy(param, token);
    }
    return true;
}

/**
 * function: send answer to the client
 * sd: file descriptor
 * message: formatting string in printf format
 * ...: variable arguments for economics of formats
 * return: true if no problem arise or else
 * notes: the MSG_x have preformated for this use
 **/
bool send_ans(int sd, char *message, ...) {
    char buffer[BUFSIZE];

    va_list args;
    va_start(args, message);

    vsprintf(buffer, message, args);
    va_end(args);

    // send answer preformated and check errors
    if(send(sd, buffer, sizeof(buffer), 0) < 0) {
        printf("ERROR: failed to send message.\n");
        return false;
    } else {
        return true;
    }
}

/**
 * function: RETR operation
 * controlfd: socket descriptor for control connection
 * datafd: socket descriptor for data connection
 * file_path: name of the RETR file
 **/
void retr(int sd, int datafd, char *file_path) {
    FILE *file;
    int bread;
    long fsize;
    char buffer[BUFSIZE];

    // check if file exists if not inform error to client
    file = fopen(file_path, "r");
    if(file == NULL) {
        printf(MSG_550, file_path);
        send_ans(sd, MSG_550, file_path);
        return;
    }

    // send success message 
    send_ans(sd, MSG_200);

    fseek(file, 0L, SEEK_END);
    fsize = ftell(file);

    // send a success message with the file length
    send_ans(sd, MSG_150, file_path, fsize);

    rewind(file);


    // send the file
    while(!feof(file)) {
        bread = fread(buffer, 1, BUFSIZE, file);
        if(bread > 0) {
            send(datafd, buffer, bread, 0);
            sleep(1); // important delay to avoid problems with buffer size
        }
    }

    // close the file
    fclose(file);

    // send a completed transfer message
    send_ans(sd, MSG_226);
    printf(MSG_226);
}

/**
 * function: STOR operation
 * controlfd: socket descriptor for control connection
 * datafd: socket descriptor for data connection
 * file_path: name of the STOR file
 **/
void stor(int controlfd, int datafd, char *file_path) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the message "200 PORT command succesful"
    send_ans(controlfd, MSG_200);

    // send message with file information
    send_ans(controlfd, MSG_150_2, file_path);

    // open the file to write
    file = fopen(file_path, "w");

    // receive the file
    while(1) {
        recv_s = read(datafd, desc, r_size);
        if (recv_s > 0) {
            fwrite(desc, 1, recv_s, file);
        }
        if (recv_s < r_size) {
          break;  
        }
    }

    // close the file
    fclose(file);

    // send a completed transfer message
    send_ans(controlfd, MSG_226);
}

/**
 * function: check valid credentials in ftpusers file
 * user: login user name
 * pass: user password
 * return: true if found or false if not
 **/
bool check_credentials(char *user, char *pass) {
    FILE *file;
    char *path = "./ftpusers", *line = NULL, cred[100];
    size_t len = 0;
    bool found = false;

    // make the credential string
    strcpy(cred, user);
    strcat(cred, ":");
    strcat(cred, pass);
    strcat(cred, "\n");

    // check if ftpusers file is present
    file = fopen(path, "r");
    if (file == NULL) {
      printf(MSG_550, path);
      return found;
    }

    // search for credential string
    line = (char*) malloc (sizeof(char)*100);
    while (!feof(file)) {
        fgets(line, 100, file);
        if (strcmp(cred, line) == 0) {
            found = true;
        }
    }

    // close file and release any pointes if necessary
    fclose(file);
    free(line);

    // return search status
    if (found == true) {
        printf(MSG_230, user);
    } else {
        printf(MSG_530);
    }

    return found;
}

/**
 * function: login process management
 * sd: socket descriptor
 * return: true if login is succesfully, false if not
 **/
bool authenticate(int sd) {
    char user[PARSIZE], pass[PARSIZE];

    // wait to receive USER action
    recv_cmd(sd, "USER", user);

    // ask for password
    send_ans(sd, MSG_331, user);

    // wait to receive PASS action
    recv_cmd(sd, "PASS", pass);

    // if credentials don't check, deny login
    if(!check_credentials(user, pass)) {
        send_ans(sd, MSG_530);
        printf("Connection closed by server.\n");
        return false;
    }

    // confirm login
    send_ans(sd, MSG_230, user);
    return true;
}

/**
 * function: execute all commands (RETR|QUIT)
 * controlsd: socket descriptor for control connection
 * datafd: socket descriptor for data connection
 **/
void operate(int controlsd, int datafd) {
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands sent by the client if not inform and exit
        if (!recv_cmd(controlsd, op, param)) {
            exit(1);
        }
        if (strcmp(op, "RETR") == 0) {
            retr(controlsd, datafd, param);
        } else if(strcmp(op, "STOR") == 0) {
            stor(controlsd, datafd, param);
        } else if(strcmp(op, "QUIT") == 0) {
            // send goodbye and close connection
            send_ans(controlsd, MSG_221);
            close(controlsd);
            close(datafd);
            break;
        } else {
            // invalid command
            // future use
        }
    }
}

/**
 * function: split the string that contains IP address and port numbers, save IP address, calculate port number and save it
 * info: string that contains PORT command information (%s,%d,%d)
 * client_ip: variable where IP address is saved
 * client_port: variable where port is saved
 **/
void get_client_ip_port(char *info, char *client_ip, int *client_port) {    
    char *n1, *n2, *n3, *n4, *n5, *n6;
    int x5, x6;

    n1 = strtok(info, ",");
    n2 = strtok(NULL, ",");
    n3 = strtok(NULL, ",");
    n4 = strtok(NULL, ",");
    n5 = strtok(NULL, ",");
    n6 = strtok(NULL, ",");

    sprintf(client_ip, "%s.%s.%s.%s", n1, n2, n3, n4);

    x5 = atoi(n5);
    x6 = atoi(n6);
    *client_port = (256*x5)+x6;
}

/**
 * function: creates the data connection for file transfer
 * fd: socket descriptor
 * client_ip: IP address to be assigned to the socket
 * client_port: port to be assigned to the client side socket
 * server_port: port to be assigned to the server side socket
 **/
int setup_data_connection(int *fd, char *client_ip, int client_port, int server_port) {    
    struct sockaddr_in cliAddr, tempAddr;

    if((*fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind port for data connection to be control port +1 by using a temporary struct sockaddr_in
    bzero(&tempAddr, sizeof(tempAddr));
    tempAddr.sin_family = AF_INET;
    tempAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tempAddr.sin_port = htons(server_port+1);

    while((bind(*fd, (struct sockaddr*) &tempAddr, sizeof(tempAddr))) < 0) {
        server_port++;
        tempAddr.sin_port = htons(server_port);
    }

    // initiate data connection fd with client ip and client port
    bzero(&cliAddr, sizeof(cliAddr));
    cliAddr.sin_family = AF_INET;
    cliAddr.sin_port = htons(client_port);


    if(inet_pton(AF_INET, client_ip, &cliAddr.sin_addr) <= 0) {
        perror("inet_pton error");
        return -1;
    }

    if(connect(*fd, (struct sockaddr *) &cliAddr, sizeof(cliAddr)) < 0) {
        perror("connect error");
        return -1;
    }

    return 0;
}

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // arguments checking
    if (argc != 2) {
    	printf("Error in the number of arguments.\n");
    	exit(1);
    }

    // reserve sockets and variables space    
    int sockfd, clientsock;
    struct sockaddr_in serverAddr;  // the IP address + port of the machine
    struct sockaddr_in clientAddr;  // address of the active participant
    int sin_size;
    pid_t pid;
    struct sigaction sa;
    int portNumber = atoi(argv[1]);

    serverAddr.sin_family = AF_INET; 
    serverAddr.sin_port = htons(portNumber); 
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    memset(&(serverAddr.sin_zero), '\0', 8);

    // create server socket and check errors
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) {
        printf("Socket could not create.\n");
        exit(1);
    } else {
        printf("Socket created.\n");
    }

    // bind master socket and check errors
    if(bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) < 0) {
    	printf("Error in binding.\n");
    	exit(1);
    }

    // make it listen     
    if(listen(sockfd, BACKLOG) < 0) {
    	printf("Error in listening.\n");
    	exit(1);
    } else {
	printf("Socket listening.\n");
    }

    /**
     * This code is responsible for reaping zombie processes
     * that appear as forked child processes exit
     **/
    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // main loop
    while (true) {
        // accept connections sequentially and check errors
        /* accept() creates a new, connected socket.
         * accept is blocking: waits for connection before returning
         * and dequeues the next connection on the queue for listenfd
         **/
        sin_size = sizeof(struct sockaddr_in);
    	if((clientsock = accept(sockfd, (struct sockaddr *) &clientAddr, (socklen_t *) &sin_size)) < 0) {
    		printf("Error in accept function.\n");
    		exit(1);
    	}

        // send hello
        send_ans(clientsock, MSG_220);

        // operate only if authenticate is true
        if(!authenticate(clientsock)) {
            printf(MSG_530);
            close(clientsock);
        } else {
            pid = fork();
            if (pid < 0) {
                printf("Error creating a new process.\n");
                return -1;
            }

            if (pid == 0) { // Esclavo
                close(sockfd); // se cierra el socket inicial
                int datafd, client_port = 0;
                char recvline[BUFSIZE+1], client_ip[50];

                // receive PORT command
                recv_cmd(clientsock, "PORT", recvline);

                // save client IP and port data
                get_client_ip_port(recvline, client_ip, &client_port);

                // open data connection
                if ((setup_data_connection(&datafd, client_ip, client_port, portNumber)) < 0) {
                    printf("Error setting up data connection.\n");
                    break;
                }

                operate(clientsock, datafd);
                close(datafd);

                close(clientsock);  // El proceso padre no lo necesita
                exit(0);
            }
        }
        close(clientsock);
    }

    // close server socket
    close(sockfd);

    return 0;
} 