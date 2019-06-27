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
#define BACKLOG 10

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
    if(send(sd,buffer, BUFSIZE, 0)<0){
        printf("No se pudo enviar datos al usuario\n");
        return;
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
    send_msg(sd,"USER",input);
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    if(!recv_msg(sd,331,NULL))
    {
      printf("No se pudo leer el usuario\n" );
      return;
    }

    // ask for password
    printf("passwd: ");
    input = read_input();

    // send the command to the server
    send_msg(sd,"PASS",input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    if(!recv_msg(sd,230,NULL))
    {
      exit(-1);
    }
}

/**
 * function: operation get
 * sd: socket descriptor
 * datasd: socket descriptor / data connection
 * file_name: file name to get from the server
 **/
void get(int sd, int datasd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);

    // check for the response
    if (recv_msg(sd, 550, NULL)) return;
    
    // recv COMMAND PORT Succesful
    recv_msg(sd, 200, NULL);

    // parsing the file size from the answer received
    sscanf(buffer, "150 Opening BINARY mode data connection for %*s (%d bytes)", &f_size);

    // open the file to write
    file = fopen(file_name, "w");
    
    // receive the file
	while(1) {
        recv_s = read(datasd, desc, r_size);
        if (recv_s > 0) fwrite(desc, 1, recv_s, file);
        if (recv_s < r_size) break;
    }
    
    // close the file
    fclose(file);

    // receive the OK from the server
    recv_msg(sd, 226, NULL);
}

/**
 * function: operation put
 * sd: socket descriptor
 * datasd: socket descriptor / data connection
 * file_name: file name to get from the server
 **/
void put(int sd, int datasd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    int bread;
    FILE *file;

    // open the file to read
    // check if file exists if not inform error to client
    if ((file = fopen(file_name, "r")) == NULL) {
        printf(MSG_550, file_name);
        return;
    }

    // send the STOR command to the server
    send_msg(sd, "STOR", file_name);
    
    // receive message from the server
    recv_msg(sd, 200, NULL);

    // receive message from the server
    recv_msg(sd, 150, NULL);
 
    // send the file
    while(1) {
        bread = fread(buffer, 1, BUFSIZE, file);
        if (bread > 0) {
            send(datasd, buffer, bread, 0);
            // important delay for avoid problems with buffer size
            sleep(1);
        }
        if (bread < BUFSIZE) break;
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
 * function: make all operations (get|put|quit)
 * sd: socket descriptor
 * datasd: socket descriptor / data connection
 **/
void operate(int sd, int datasd) {
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
            get(sd, datasd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            close(datasd);
            quit(sd);
            break;
        }
        else if (strcmp(op, "put") == 0) {
            param = strtok(NULL, " ");
            put(sd, datasd, param);
        }
        else {
            // new operations in the future
            printf("TODO: unexpected command\n");
        }
        free(input);
    }
    free(input);
}

/**
 * function: get ip & port from sd
 * sd: socket descriptor
 * ip: string where IP address is saved
 * port: int where PORT is saved
 **/
void get_ip_port(int sd, char *ip, int *port) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getsockname(sd, (struct sockaddr*) &addr, &len);
    sprintf(ip,"%s", inet_ntoa(addr.sin_addr));
    *port = (uint16_t)ntohs(addr.sin_port);
}

/**
 * function: port conversion to two numbers
 * we will use later in PORT command
 * port: the port we want to convet
 * n5: fifth number in PORT command
 * n6: sixth number in PORT command 
 **/
void convert(uint16_t port, int *n5, int *n6) {
    int i = 0;
    int x = 1;
    *n5 = 0;
    *n6 = 0;
    int temp = 0;
    for(i = 0; i < 8; i++) {
        temp = port & x;
        *n6 = (*n6)|(temp);
        x = x << 1; 
    }

    port = port >> 8;
    x = 1;

    for(i = 8; i < 16; i++){
        temp = port & x;
        *n5 = ((*n5)|(temp));
        x = x << 1; 
    }
}

/**
 * function: construction of the string
 * used in PORT command
 * str: string to be used in PORT command
 * ip: IP address to be used in PORT command
 * n5: fifth number in PORT command
 * n6: sixth number in PORT command 
 **/
void get_port_string(char *str, char *ip, int n5, int n6) {
    int i = 0;
    char ip_temp[1024];
    strcpy(ip_temp, ip);

    for(i = 0; i < strlen(ip); i++){
        if(ip_temp[i] == '.'){
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
    int sd, listensd, datasd, n5, n6, x;
    uint16_t port;
    char ip[50], str[BUFSIZE+1];
    struct sockaddr_in addr, data_addr;
    struct hostent *he;

    // arguments checking
    if (argc != 3) {
        printf("usage:%s <SERVER_IP> <SERVER_PORT>\n", argv[0]);
        exit(1);
    }

    // get host by name
    if ((he = gethostbyname(argv[1])) == NULL) {   
        perror("gethostbyname");
        exit(1);
    }

    // set control connection
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(*argv[2]); 
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(addr.sin_zero), '\0', 8);  
    
    // connect and check for errors
    if (connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    // if receive hello proceed with authenticate and operate if not warning
    if (!recv_msg(sd, 220, NULL)) {
        warn("error receiving data");
    } else {
        authenticate(sd);

        // set data connection
        listensd = socket(AF_INET, SOCK_STREAM, 0);

        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(0); 
        data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&(data_addr.sin_zero), '\0', 8);

        bind(listensd, (struct sockaddr*) &data_addr, sizeof(data_addr));
        listen(listensd, BACKLOG);

        // get ip address and control port
        get_ip_port(sd, ip, (int *) &x);

        // get data connection port from listensd
        get_ip_port(listensd, str, (int *) &port);

        convert(port, &n5, &n6);

        // get client-side data port and IP as a string
        get_port_string(str, ip, n5, n6);

        // send PORT command to server
        send_msg(sd, "PORT", str);

        datasd = accept(listensd, (struct sockaddr*) NULL, NULL);

        operate(sd, datasd);

        close(datasd);
    }

    // close socket
    close(sd);

    return 0;
}
