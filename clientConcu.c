 #include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
	

#define BUFSIZE 512

#define MSG_550 "550 %s: no such file or directory\r\n"

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
     recv_s = recv (sd, buffer, BUFSIZE, 0); 


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
    if (send(sd, buffer, sizeof(buffer), 0) < 0) { 
	        printf("Command Error");
	        
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
    send_msg (sd, "USER", input);
    
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    code = 331;
	    if(!(recv_msg(sd, code, desc)) ) {
	        exit(1);
	    }


    // ask for password
    printf("password: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);


    // release memory
    free(input);

    // wait for answer and process it and check for errors
    code = 230;
     if(!(recv_msg(sd, code, desc))) {
	        exit(1);
     }

}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd, int dcsd, char *file_name) {  //dcsd: data connection, socket desc.
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);

    // check for the response
    if(recv_msg(sd, 550, NULL)) {
	    return;
	    }

    recv_msg(sd, 200, NULL); // PORT command

    // parsing the file size from the answer received
    sscanf(buffer, "150 Opening BINARY mode data connection for %*s (%d bytes)", &f_size); 

    // open the file to write
    file = fopen(file_name, "w");

    //receive the file
    while(1) {
	    recv_s = read(dcsd, desc, r_size);
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
    recv_msg(sd, 226, NULL);

}
//put op.
void put(int sd, int dcsd, char *file_name) { //dcsd: data connection, socket desc.
	    char desc[BUFSIZE], buffer[BUFSIZE];
	    int f_size, recv_s, r_size = BUFSIZE;
	    int bRead;
	    FILE *file;
	
	    // open the file to read and check for errors
        file = fopen(file_name, "r");
	    if(file == NULL) {
	        printf(MSG_550, file_name);
	        return;
	    }
	
	    
	    send_msg(sd, "STOR", file_name); // servidor recive comando STOR
	
	    recv_msg(sd, 200, NULL); // PORT command
	
	    // receive message with file information
	    recv_msg(sd, 150, NULL);
	
	    // send the file
	    while(1) {
	        bRead = fread(buffer, 1, BUFSIZE, file);
	        if (bRead > 0) {
	            send(dcsd, buffer, bRead, 0);
	            // important delay for avoid problems with buffer size
	            sleep(1);
	        }
	        if(bRead < BUFSIZE) break;
	    }
	
	    // close the file
	    fclose(file);
	
	    // receive the OK from the server
	    recv_msg(sd, 226, NULL);
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
 * sd: socket descriptor
 **/
void operate(int sd, int dcsd) { //dcsd: data connection, socket desc.
    char *input, *op, *param;

    while (true) {
        printf("Operation: ");
        input = read_input();
        if (input == NULL)
            continue; // avoid empty input
        op = strtok(input, " ");

        if (strcmp(op, "get") == 0) {
            param = strtok(NULL, " ");
            get(sd, dcsd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            close(dcsd);
            quit(sd);
            break;
        }
        else if(strcmp(op, "put") == 0) {
	            param = strtok(NULL, " ");
	            put(sd, dcsd, param);
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

        int i = 0, x = 1;
        int temp = 0;

        *n5 = 0; *n6 = 0;
	
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
    int clientSocket, connection_status, listen_desc, data_desc;
    int n5, n6, p;
    char str[BUFSIZE];
    char ip[50];
    uint16_t puerto;
    struct sockaddr_in serverAddr, daddr;
    struct hostent *hn;

    // arguments checking
     if(argc =! 3){
      printf("Arguments Error\n" );
      exit(1);
    }
    
    hn = gethostbyname (argv[1]);
    if (hn==NULL){
        perror("ERROR: gethostbyname");
        exit(1);
    }

    // create socket and check for errors
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1){
      perror("Falla socket");
	    exit (1);
    }
    
    // set socket data    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons (atoi (argv[2]));
    serverAddr.sin_addr = *((struct in_addr *)hn->h_addr);
    memset(&(serverAddr.sin_zero), '\0', 8); 


    // connect and check for errors
    
    connection_status =  connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr));
      
      if (connection_status == -1){
        printf("Error al coenctar");
        exit(1);
    }
    // if receive hello proceed with authenticate and operate if not warning
    if(!recv_msg(clientSocket,220,NULL)){
       warn("ERROR");
    }else
    {
        authenticate(clientSocket);
        listen_desc = socket(AF_INET, SOCK_STREAM,0);

        daddr.sin_family = AF_INET;
	    daddr.sin_port = htons(0); // random port
	    daddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    memset(&(daddr.sin_zero), '\0', 8);
	
	    bind(listen_desc, (struct sockaddr*) &daddr, sizeof(daddr));
	    listen(listen_desc, 5);


	    get_ip_port(clientSocket, ip, (int *) &p); // ip and control port
	
	        
	    get_ip_port(listen_desc, str, (int *) &puerto); // get data-connection ip and port from listen_desc
	
        convert(puerto, &n5, &n6);
	
	        // get client-side data port and IP as a string
        get_port_string(str, ip, n5, n6);
	
	        
        send_msg(clientSocket, "PORT", str);  // send PORT command to server
        bzero(str, (int)sizeof(str));
	
        data_desc = accept(listen_desc, (struct sockaddr*) NULL, NULL);
	
        operate(clientSocket, data_desc);
        close(data_desc);
	    }
	
	    // close socket
	    close(clientSocket);
	
	    return 0;

}
