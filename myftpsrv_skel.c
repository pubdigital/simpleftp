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
    if( (send(sd, buffer, BUFSIZE, 0)) < 0){
        printf("Error en envio de la respuesta : %s\n", buffer);
        return false;
    }
    
    return true;
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
    char *path = "./ftpusers", *line = NULL, cred[200];
    size_t len = 0;
    bool found = false;
    int encontrado = 0;

    strcat(cred,user);
    strcat(cred,":");
    strcat(cred,pass);   
    printf("cadena de busqueda:'%s'\n",cred);
    file = fopen(path,"r");
    line = (char*)malloc(sizeof(char) * 100);
    //printf("abriendo archivo");
    if(file == NULL){
       printf("No existe el archivo");
    }else{
          while(!feof(file)){
    	    fscanf(file,"%s", line);
    	    printf("LINE:'%s'",line);
            if(strcmp(cred,line) == 0){
                found = true;
                encontrado=1;
		        break;
            }
          }
        free(line);
        fclose(file);
        }
    printf("%d\n",encontrado);
    return found;
    //ahora imprime 1 si lo encontro...
}
/**
 * function: login process management
 * sd: socket descriptor
 * return: true if login is succesfully, false if not
 **/
bool authenticate(int sd) {
    char user[PARSIZE], pass[PARSIZE];

    // wait to receive USER action
    if(recv_cmd(sd, "USER", user) != true){
        return false;
    } 

    // ask for password
    send_ans(sd, MSG_331);

    // wait to receive PASS action
    recv_cmd(sd, "PASS", pass);

    // if credentials don't check denied login
    if(!check_credentials(user, pass)){
        printf("No encontrado");
        send_ans(sd, MSG_530);
        if (close(sd) > 0);
        return false;
    }else{
         // confirm login
        printf("Encontrado");
        send_ans(sd, MSG_230, user);
    }

   
    return true;
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
    if(argc<1){
        printf("<puerto>\n");
        return 1;
    }

    int socket_desc,socket_client, *new_sock, c = sizeof(struct sockaddr_in);
    char buffer[BUFSIZE], userpass[BUFSIZE],cadena[BUFSIZE], a [100], cod [5];
    char *ptr;
    FILE *buscar;
    struct  sockaddr_in  server;
    struct  sockaddr_in  client;
    int resp_size;
    int puerto = (atoi(argv[1]));

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1){
        printf("No se puede crear el servidor");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(puerto);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("Fallo el bind");
        return 1;
    }

    listen(socket_desc , 3);
    
    printf("Escuchando al cliente en el puerto %d...\n", puerto);
    while (true) {
        socklen_t longc = sizeof(client);
        socket_client = accept(socket_desc, (struct sockaddr *)&client, &longc);
        if (socket_client<0)
        {
            printf("Fallo el Accept");
            return 1;
        }else{
            printf("Conectando con:%d\n",htons(client.sin_port));
            send(socket_client, MSG_220, sizeof(MSG_220), 0);
            authenticate(socket_client);
        }
       
    }

    return 0;
}
