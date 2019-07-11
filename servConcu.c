#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
	
	
#define BUFSIZE 512

#define CMDSIZE 5
#define PARSIZE 100
	
#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

#define MSG_200 "200 PORT command successful\r\n"
#define MSG_150 "150 Opening BINARY mode data connection for %s (%d bytes)\r\n"
#define MSG_150B "150 Opening BINARY mode data connection for %s\r\n"

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
    if (recv_s < 0) {
        warn("error receiving data");
    }
	if (recv_s == 0){
        errx(1, "connection closed by host");
    }

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

      if(send(sd, buffer, sizeof(buffer), 0) == -1) {
          printf("No se pudo enviar el mensaje");
          return false;
          } 
          else {
	        return true;
	    }


}

/**
 * function: RETR operation
 * sd: socket descriptor
 * file_path: name of the RETR file
 **/

void retr(int sd, int data_desc, char *file_path) { //data_desc: data connection sd
    FILE *file;
    int bread;
    long fsize;
    char buffer[BUFSIZE];

    file = fopen(file_path, "r");

    // check if file exists if not inform error to client
    if (file == NULL) {
            printf(MSG_550, file_path); 
	        send_ans(sd, MSG_550, file_path);
	        return;
	    } 

    send_ans(sd, MSG_200); //port command succes


    //file size
    fseek(file, 0L, SEEK_END);
	fsize = ftell(file);
    // send a success message with the file length
    send_ans(sd, MSG_150, file_path, fsize); 

   	rewind(file);

	
    // send the file
    while (!feof(file)){
         bread = fread(buffer, 1, BUFSIZE, file);

         if (bread > 0) {
	            send (data_desc, buffer, bread, 0);
	            // important delay for avoid problems with buffer size
	            sleep(1);
         
	    }

    }
    
    // close the file
    fclose(file);
    // send a completed transfer message
    send_ans(sd, MSG_226);
}

//STOR op.
//sd: socket desciptor
//data_desc: data connection socket desc
// file_path: SOTRE's file name
void stor(int sd, int data_desc, char *file_path) {
	    char desc[BUFSIZE], buffer[BUFSIZE];
	    int f_size, recv_s, r_size = BUFSIZE;
	    FILE *file;
	
	    send_ans(sd, MSG_200); //port message
	
	    send_ans(sd, MSG_150B, file_path); //file info
	
	    file = fopen(file_path, "w");
	
	    // receive the file
	    while(1) {
	        recv_s = read(data_desc, desc, r_size);
	
	        if (recv_s > 0) {
	            fwrite(desc, 1, recv_s, file);
	        }
	
	        if (recv_s < r_size) {
	          break;  
	        }
	    }
	
	    // close the file
	    fclose(file);
	
	    send_ans(sd, MSG_226); //transfer message
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
    file = fopen (path, "r");
    if (file==NULL){
        printf("No se puede abrir el archivo\n");
    }


    // search for credential string
    line = (char*)malloc(100*sizeof(char));

    while(!feof(file)){
        fgets(line,100, file);
        if(strcmp(cred,line)==0){
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

    // if credentials don't check denied login

	if(!(check_credentials(user, pass))) {
	      send_ans(sd, MSG_530);
	      return false;
      }

    // confirm login
       
	send_ans(sd, MSG_230, user);
	return true;
	    
}

/**
 *  function: execute all commands (RETR|QUIT)
 *  sd: socket descriptor
 **/

void operate(int sd, int data_desc) {//data_desc: data connection descriptor
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit
        if(!recv_cmd(sd, op, param)) {
	            exit(1);
	        }


        if (strcmp(op, "RETR") == 0) {
            retr(sd, data_desc, param);
        }
            else if(strcmp(op, "STOR") == 0) {
	                stor(sd, data_desc, param);
            }
            else if (strcmp(op, "QUIT") == 0) {
            
        // send goodbye and close connection
            send_ans(sd, MSG_221);
             close(data_desc);
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

		int x5, x6;

		char *n1, *n2, *n3, *n4, *n5, *n6;
	
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
	 * sd: socket descriptor
	 * client_ip: IP address to be assigned to the socket
	 * client_port: port to be assigned to the client side socket
	 * server_port: port to be assigned to the server side socket
	 **/
	int setup_data_connection(int *sd, char *client_ip, int client_port, int server_port) {
		struct sockaddr_in clientaddr, tempaddr;
	
		if ((*sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    	perror("socket error");
	    	return -1;
	    }
	
		// bind port for data connection to be server port + 1 by using a temporary struct sockaddr_in
	    server_port++;
	    tempaddr.sin_family = AF_INET;
	    tempaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    tempaddr.sin_port = htons(server_port); 
	    memset(&(tempaddr.sin_zero), '\0', 8); 
	
	    while((bind(*sd, (struct sockaddr*) &tempaddr, sizeof(tempaddr))) < 0) {
	        server_port++;        
	    	tempaddr.sin_port = htons(server_port);
	    }
	// initiate data connection with client ip and client port             
	    clientaddr.sin_family = AF_INET;
	    clientaddr.sin_port = htons(client_port);
	    memset(&(clientaddr.sin_zero), '\0', 8); 
	    if (inet_pton(AF_INET, client_ip, &clientaddr.sin_addr) <= 0){
	    	perror("inet_pton");
	    	return -1;
	    }
	
	    if (connect(*sd, (struct sockaddr *) &clientaddr, sizeof(clientaddr)) < 0) {
	    	perror("connect error");
	    	return -1;
	    }
	
	    return 0;
	}
    void sigchld_handler(int s) {
	    while(wait(NULL) > 0);
	}
/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // arguments checking
    if(argc =! 2){
      printf("Arguments Error\n" );
      exit(1);
    }
    // reserve sockets and variables space
    int socket_descriptor, newsockfd;
    struct sockaddr_in servaddr, clieaddr;
    int puerto = atoi (argv[1]);
    int sin_size;
	pid_t pid;
    struct sigaction sa;
	

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(puerto);
    servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    memset(&(servaddr.sin_zero), '\0', 8); 

     // create server socket and check errors
    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1){
      perror("Falla socket");
	    exit (1);
    }
        else{
            printf("socket created succesfully");
        }
    

    // bind master socket and check errors
    if (bind(socket_descriptor, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0){
        printf("binding failed");
    }

    // make it listen

    if (listen (socket_descriptor, 5) < 0){
        printf("Listen error");
        exit(1);
    }
    

    //zombies
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
        newsockfd = accept(socket_descriptor, (struct sockaddr *) &clieaddr, (socklen_t *) &sin_size);

	    	if(newsockfd < 0) {
	    		printf("Error on Accept.\n");
	    		return -1;
	    	}



        // send hello
        send_ans(newsockfd, MSG_220);

        // operate only if authenticate is true
        if(authenticate(newsockfd) != true){
            close(newsockfd);
        }else{
            pid = fork();
            if (pid==0){
                close(socket_descriptor); // se cierra el primer socket creado
                 int data_desc, client_port = 0;
				 char recvline[BUFSIZE+1], client_ip[50];
	
	             // receive PORT command from the client
	             recv_cmd(newsockfd, "PORT", recvline);
	
	             // save client IP and port data
	             get_client_ip_port(recvline, client_ip, &client_port);
	
	             // open data connectionx   
	             if((setup_data_connection(&data_desc, client_ip, client_port, puerto)) < 0) {
	                 break;
	               }
            if (pid < 0){
                printf("hijo no creado");
                return -1;
                
            }

             operate(newsockfd, data_desc);
	
	         close(data_desc);
	         close(newsockfd);
	         exit(0);
            }
        }
        close (newsockfd);
    }

    // close server socket
  close(socket_descriptor);

    return 0;
}
