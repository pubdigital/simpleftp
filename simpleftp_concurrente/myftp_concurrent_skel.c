//Franco Mellano - Tercer año AUS

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


/**
 * function: si hay error, se imprime mensaje y se corta el programa
 **/
void DieWithError(char * msg){
    printf("%s\n", msg);
    exit(-1);
}

/**
 * function: receive and analize the answer from the server
 * sd: socket descriptor
 * code: three letter numerical code to check if received
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
    if( (send(sd, buffer, BUFSIZE, 0)) < 0) DieWithError("Error al enviar mensaje.");
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

    sleep(0.5);

    // ask for user
    printf("username: ");
    input = read_input();

    sleep(0.5);

    // send the command to the server
    send_msg(sd, "USER", input);
    
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    recv_msg(sd, 331, desc);

    // ask for password
    printf("passwd: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    if(!recv_msg(sd, 230, desc)) DieWithError("Autenticacion incorrecta. Terminando cliente.");
}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd, int dsd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);

    // check for the response
    if(recv_msg(sd, 550, NULL) == true) return;

    // recv COMMAND PORT Succesful
    recv_msg(sd, 200, NULL);

    // parsing the file size from the answer received
    sscanf(buffer, "150 Opening BINARY mode data connection for %*s (%d bytes)", &f_size);

    // open the file to write
    file = fopen(file_name, "wb");

    //receive the file
    while(1){
        recv_s = read(dsd, desc, r_size);
                
        fwrite(desc, 1, recv_s, file);

        if (recv_s < r_size){
            // close the file
            fflush(file);
            fclose(file);
            break;
        }
    }

    // receive the OK from the server
    recv_msg(sd, 226, NULL);
}

/**
 * function: operation put
 * enviar archivo al servidor
 **/
void put(int sd, int dsd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    int bread;
    FILE *file;

    // check if file exists if not inform error
    if ((file = fopen(file_name, "r")) == NULL) {
        printf("550 %s: no such file or directory\r\n", file_name);
        return;
    }

    // Enviar comando STOR al server
    send_msg(sd, "STOR", file_name);
    recv_msg(sd, 200, NULL);
    recv_msg(sd, 150, NULL);
 
    // send the file
    while(1){    
        bread = fread(buffer, 1, BUFSIZE, file);
        if(bread <= 0){ 
            break;
        }
        send(dsd, buffer, bread, 0);
    }
   
    // close the file
    fclose(file);

     //recieve a completed transfer message
    recv_msg(sd, 226, NULL);
}


/**
 * function: construction ip port
 **/

void get_ip_port(int sd, char *ip, int *port) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getsockname(sd, (struct sockaddr*) &addr, &len);

    sprintf(ip,"%s", inet_ntoa(addr.sin_addr));

    *port = (uint16_t)ntohs(addr.sin_port);
}


/**
 * function: construction of the string
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
 * function: conversion
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
 * function: make all operations
 * sd: socket descriptor
 **/
void operate(int sd, int dsd) {
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
            get(sd, dsd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            quit(sd);
            break;
        }
        else if (strcmp(op, "put") == 0) {
            param = strtok(NULL, " ");
            put(sd, dsd, param);
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
 * Run with
 *         ./myftp <SERVER_IP> <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // arguments checking
    if (argc!=3) DieWithError("Numero de argumentos invalido.\n");

    // reserva e iniciacion de variables
    int clientSocket, listenSocket, dataSocket, n5, n6, x;
    uint16_t port;
    char ip[50], str[BUFSIZE+1];
    struct sockaddr_in serverAddr, dataAddr;
    struct hostent *he;

    // get host by name
    if ((he = gethostbyname(argv[1])) == NULL) DieWithError("No se puedo conseguir servidor por nombre.\n");

    // create socket and check for errors
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)  DieWithError("Fallo en la creacion de socket.");

    // set socket data 
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(*argv[2]); 
    serverAddr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(serverAddr.sin_zero), '\0', 8);  
    
    // connect and check for errors
    if (connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr)) == -1) DieWithError("Fallo en el connect().");

    // if receive hello proceed with authenticate and operate if not warning
    if (!recv_msg(clientSocket, 220, NULL)) {
        DieWithError("No se pudo establecer la comunicacion correctamente (No llega mensaje del servidor). Terminando ejecucion.");
    } else {

        authenticate(clientSocket);

        listenSocket = socket(AF_INET, SOCK_STREAM, 0);

        dataAddr.sin_family = AF_INET;
        dataAddr.sin_port = htons(0); 
        dataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(&(dataAddr.sin_zero), '\0', 8);

        bind(listenSocket, (struct sockaddr*) &dataAddr, sizeof(dataAddr));
        listen(listenSocket, BACKLOG);

        get_ip_port(clientSocket, ip, (int *) &x);
        get_ip_port(listenSocket, str, (int *) &port);
        convert(port, &n5, &n6);
        get_port_string(str, ip, n5, n6);

        send_msg(clientSocket, "PORT", str);

        dataSocket = accept(listenSocket, (struct sockaddr*) NULL, NULL);

        operate(clientSocket, dataSocket);

        close(dataSocket);
    }

    // close socket
    if (close(clientSocket) > 0) 
        DieWithError("Fallo al cerrar socket.");
    else
        printf("Fin de ejecucion de cliente.\n");

}
