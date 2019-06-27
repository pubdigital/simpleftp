#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>              

#include <signal.h>
#include <sys/wait.h>

#define BUFSIZE 512
#define CMDSIZE 5
#define PARSIZE 100
#define BACKLOG 10

#define MSG_150(OPTION) ((OPTION != 0) ? ("150 Opening BINARY mode data connection for %s (%ld bytes)\r\n"):("150 Opening BINARY mode data connection for %s\r\n"))
#define MSG_200 "200 PORT command successful\r\n"
#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

/**
 * function: receive the commands from the client
 * sd: socket descriptor
 * operation: \0 if you want to know the operation received
 *            OP if you want to check an especific operation
 *            ex: recv_cmd(sd, "USER", param)
 * param: parameters for the operation involve
 * return: only usefull if you want to check an operation
 *         ex: for login you need the seq USER PASS
 *             you can check if you receive first USER
 *             and then check if you receive PASS
 **/
bool recv_cmd(int sd, char *operation, char *param) {
    char buffer[BUFSIZE], *token;
    int recv_s;

    // receive the command in the buffer and check for errors
    recv_s = recv(sd, buffer, BUFSIZE, 0);
    if (recv_s < 0) warn("error receiving data");
    if (recv_s == 0) errx(1, "connection closed by host");

    // expunge the terminator characters from the buffer
    buffer[strcspn(buffer, "\r\n")] = 0;

    // complex parsing of the buffer
    // extract command receive in operation if not set \0
    // extract parameters of the operation in param if it needed
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
 * return: true if not problem arise or else
 * notes: the MSG_x have preformated for these use
 **/
bool send_ans(int sd, char *message, ...){
    char buffer[BUFSIZE];

    va_list args;
    va_start(args, message);

    vsprintf(buffer, message, args);
    va_end(args);

    // send answer preformated and check errors
    if ((send(sd, buffer, sizeof(buffer), 0)) == -1) {
        perror("send");
        return false;
    } else return true;
}

/**
 * function: RETR operation
 * sd: socket descriptor
 * datasd: socket descriptor / data connection
 * file_path: name of the RETR file
 **/
void retr(int sd, int datasd, char *file_path) {
      FILE *file;
    int bread;
    long fsize;
    char buffer[BUFSIZE];
    printf("%s\n",file_path );
    // check if file exists if not inform error to client
    if((file=fopen(file_path,"r"))==NULL){
        send_ans(sd,MSG_550,file_path);
        return;
    }
    // send a success message with the file length
    fseek(file,0L, SEEK_END);            // me ubico en el final del archivo.
    fsize=ftell(file);
    send_ans(sd,MSG_299,file_path,fsize);
    // important delay for avoid problems with buffer size
    sleep(1);

    // send the file
    rewind(file);
    while(1) {
        bread = fread(buffer, 1, BUFSIZE, file);
        if (bread > 0) {
            send(sd, buffer, bread, 0);
            // important delay for avoid problems with buffer size
            sleep(1);
        }
        if (bread < BUFSIZE) break;
    }
    // close the file
    fclose(file);
    // send a completed transfer message
    send_ans(sd,MSG_226);
}

/**
 * function: STOR operation
 * sd: socket descriptor
 * datasd: socket descriptor / data connection
 * file_path: name of the STOR file
 **/
void stor(int sd, int datasd, char *file_path) {
    FILE *file;    
    int recv_s;
    long fsize;
    int r_size = BUFSIZE;
    char desc[BUFSIZE];
    char buffer[BUFSIZE];

    // send a success message
    send_ans(sd, MSG_200);     
    
    // send a success message
    send_ans(sd, MSG_150(0), file_path);
    
    // open the file to write
    file = fopen(file_path, "w");
    
    // receive the file
	while(1) {
        recv_s = read(datasd, desc, r_size);
        if (recv_s > 0) fwrite(desc, 1, recv_s, file);
        if (recv_s < r_size) break;
    }

    // close the file
    fclose(file);

    // send a completed transfer message
    send_ans(sd, MSG_226);
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

    // check if ftpusers file it's present
    if ((file = fopen(path, "r")) == NULL) {
        printf(MSG_550, path);
        exit(1);
    } else {
    // search for credential string
        len = strlen(cred)+1;
        line = (char*)(malloc(len));
        while (!feof(file)) {
            fgets(line, len, file);
            if (strcmp(cred, line) == 0) {
                found = true;
                break;
            }
        }
    // close file and release any pointes if necessary
        fclose(file);
        free(line);
    }
    
    // return search status
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

    // if credentials don't check denied login
    if(!check_credentials(user, pass)) {
        send_ans(sd, MSG_530);
        return false;
    } else {      
    // confirm login
        send_ans(sd, MSG_230, user);
        return true;
    }
}

/**
 * function: execute all commands (RETR|STOR|QUIT)
 * sd: socket descriptor
 * datasd: socket descriptor / data connection 
 **/
void operate(int sd, int datasd) {
    char op[CMDSIZE], param[PARSIZE];
    
    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit
        if(!recv_cmd(sd, op, param)) exit(1);
         
        if (strcmp(op, "RETR") == 0) {
            retr(sd, datasd, param);
        } else if(strcmp(op, "STOR") == 0) {
            stor(sd, datasd, param);
        } else if (strcmp(op, "QUIT") == 0) {
            // send goodbye and close connection
            send_ans(sd, MSG_221);
            close(datasd);
            close(sd);
            break;
        } else {
            // invalid command
            // furute use
        }
    }
}

/**
 * function: split the string "%s,%d,%d" into numbers,
 * save IP address, calculate port number and save it
 * str: string that contains params "%s,%d,%d"
 * client_ip: variable where IP address is saved
 * client_port: variable where port is saved
 **/
void get_client_ip_port(char *str, char *client_ip, int *client_port) {
	char *n1, *n2, *n3, *n4, *n5, *n6;
	int x5, x6;
    
    n1 = strtok(str, ",");
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
 * datasd: socket descriptor
 * client_ip: IP address to be assigned to the socket
 * client_port: port to be assigned to the client side socket
 * server_port: port to be assigned to the server side socket
 **/
int setup_data_connection(int *datasd, char *client_ip, int client_port, int server_port) {
	struct sockaddr_in cliaddr, tempaddr;

	if ((*datasd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    	perror("socket error");
    	return -1;
    }

	// bind port for data connection to be server port + 1 by using a temporary struct sockaddr_in
    server_port++;
    tempaddr.sin_family = AF_INET;
    tempaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tempaddr.sin_port = htons(server_port); 
    memset(&(tempaddr.sin_zero), '\0', 8); 
     
    while((bind(*datasd, (struct sockaddr*) &tempaddr, sizeof(tempaddr))) < 0) {
        server_port++;        
    	tempaddr.sin_port = htons(server_port);
    }

	// initiate data connection with client ip and client port             
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(client_port);
    memset(&(cliaddr.sin_zero), '\0', 8); 
    if (inet_pton(AF_INET, client_ip, &cliaddr.sin_addr) <= 0){
    	perror("inet_pton error");
    	return -1;
    }

    if (connect(*datasd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
    	perror("connect error");
    	return -1;
    }

    return 0;
}

/**
 * function: define a handler for SIGCHLD 
 **/
void sigchld_handler(int s) {
    while(wait(NULL) > 0);
}

/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    // arguments checking
    if(argc != 2){
        printf("Error, ingrese el puerto\n");
        return -1;
    }

    // reserve sockets and variables space
    int sockfd, new_fd, port;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;
    struct sigaction sa;

    my_addr.sin_family = AF_INET;  // addres family       
    my_addr.sin_port = htons(*argv[1]);   // funcion que cambia a little o big endian
    my_addr.sin_addr.s_addr = INADDR_ANY; // ubicada en inet.h
    memset(&(my_addr.sin_zero), '\0', 8); 
    
    sscanf(argv[1], "%d", &port);

    // create server socket and check errors
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // bind master socket and check errors
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // make it listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // reaping zombie processes
    sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // main loop
    while (true) {
        // accept connectiones sequentially and check errors
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,(socklen_t *)&sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
        
        // send hello
        send_ans(new_fd, MSG_220);

        // operate only if authenticate is true
        if(!authenticate(new_fd)) close(new_fd);
        else {
            if (!fork()) {
                close(sockfd);

                int datasd, client_port = 0;
			    char recvline[BUFSIZE+1];
			    char client_ip[50];
                
                // receive PORT command from the client
                recv_cmd(new_fd, "PORT", recvline);

                // save client IP and port data
                get_client_ip_port(recvline, client_ip, &client_port);
                    
                // open data connection
                if((setup_data_connection(&datasd, client_ip, client_port, port)) < 0) {
                   break;
                }
                
                operate(new_fd, datasd);
                
                close(datasd);
                close(new_fd);
                exit(0);
            }
        }
        close(new_fd);
    }

    // close server socket
    close(sockfd);

    return 0;
}
