#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>

#define BUFSIZE 512
#define CMDSIZE 4
#define PARSIZE 100

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




}

/**
 * function: RETR operation
 * sd: socket descriptor
 * file_path: name of the RETR file
 **/

void retr(int sd, char *file_path) {
    FILE *file;    
    int bread;
    long fsize;
    char buffer[BUFSIZE];

    // check if file exists if not inform error to client

    // send a success message with the file length

    // important delay for avoid problems with buffer size
    sleep(1);

    // send the file

    // close the file

    // send a completed transfer message
}
/**
 * funcion: check valid credentials in ftpusers file
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

    // check if ftpusers file it's present
    file = fopen(path, "r");
    if (file == NULL)
    {
      printf(MSG_550, path);
      return -1;
    }

    // search for credential string
    line = (char*) malloc (sizeof(char)*100);

    while(!feof(file)) {
        fgets(line, 100, file);

        if(strcmp(cred, line) == 0) {
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

    // ask for password

    // wait to receive PASS action

    // if credentials don't check denied login

    // confirm login
}

/**
 *  function: execute all commands (RETR|QUIT)
 *  sd: socket descriptor
 **/

void operate(int sd) {
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit


        if (strcmp(op, "RETR") == 0) {
            retr(sd, param);
        } else if (strcmp(op, "QUIT") == 0) {
            // send goodbye and close connection




            break;
        } else {
            // invalid command
            // furute use
        }
    }
}

/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {
    
    // arguments checking
    if (argc != 2) {
    	printf("ERROR: port number not included or too many arguments.\n");
    	return -1;
    }

    // reserve sockets and variables space    
    int sockid, clientsock;         /* clientsock -> socket used for data-transfer */
    struct sockaddr_in addrport;	/* the IP address + port of the machine */
    struct sockaddr_in clientAddr;  /* address of the active participant */
    socklen_t clientAddr_size;
    int portNumber = atoi(argv[1]); /* converts the port given as an argument into an int */

    // create server socket and check errors
    sockid = socket(PF_INET, SOCK_STREAM, 0);	/* creates the socket and returns its file descriptor */
    if (sockid < 0) {
    	printf("ERROR: failed to create the socket.\n");
    	return -1;
    }

    addrport.sin_family = AF_INET; /* Internet Protocol address family */
    addrport.sin_port = htons(portNumber); /* to make sure that numbers are stored in memory in network byte order */
    addrport.sin_addr.s_addr = htonl(INADDR_ANY); /* with INADDR_ANY the socket binds to all the local interfaces */


    // bind master socket and check errors
    if(bind(sockid, (struct sockaddr *) &addrport, sizeof(addrport)) < 0) {
    	printf("ERROR: failed to associate socket with local address.\n");
    	return -1;
    }


    // make it listen    
    
    /**
     * listen is non-blocking: returns immediately.
     * The listening socket is used only as a way to get new sockets in queue
     **/
    if(listen(sockid, BACKLOG) < 0) {
    	printf("ERROR: socket failed to listen.\n");
    	return -1;
    }


    // main loop
    while (true) {

        // accept connections sequentially and check errors

        /**
         * accept() creates a new, connected socket.
         * accept is blocking: waits for connection before returning
         * and dequeues the next connectionon the queue for sockid
         **/
    	clientAddr_size = sizeof(clientAddr);

    	if((clientsock = accept(sockid, (struct sockaddr *) &clientAddr, &clientAddr_size)) < 0) {
    		printf("ERROR: socket failed to accept connection.\n");
    		return -1;
    	}


        // send hello
        send_ans(clientsock, MSG_220);
        
        // operate only if authenticate is true
        if(authenticate(clientsock) != true) {
        	printf(MSG_530);
            close(clientsock);
        } else operate(clientsock);
    }

    // close server socket
    close(sockid);

    return 0;
}
