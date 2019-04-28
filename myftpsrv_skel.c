#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <sys/.socket.h>

#include <ctype.h>
#include <signal.h>

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
    DEBUG_PRINT(("recv_cmd sd: %d operation %s param %s \n",sd,operation,param));
    recv_s = recv(sd, buffer, BUFSIZE, 0);
    DEBUG_PRINT(("buffer: %s recv_s: %d \n",buffer,recv_s));
    if (recv_s < 0) {
        warn("Error al recibir la informacion. \n");
        return false;
    }
    if (recv_s == 0) {
        warn("Se cerro la conexion con el cliente. \n");
        return false;
    }

    // expunge the terminator characters from the buffer
    buffer[strcspn(buffer, "\r\n")] = 0;
    DEBUG_PRINT(("Buffer borrado: %s\n",buffer));

    // complex parsing of the buffer
    // extract command receive in operation if not set \0
    // extract parameters of the operation in param if it needed
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        warn("El comando FTP no es valido.");
        return false;
    } else {
        if (operation[0] == '\0') strcpy(operation, token);
        if (strcmp(operation, token)) {
            warn("Error en el flujo: El comando %s no se envio", operation);
            return false;
        }
        token = strtok(NULL, " ");
        if (token != NULL) {
            strcpy(param, token);
        }
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
    bytes_sent = send(sd, buffer, strlen(buffer), 0);
    if(bytes_sent == -1) {
        warn("Error al enviar sd: %d | %s\n", sd, buffer);
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
    if (file == NULL) {
        printf(MSG_550);
    }

    // search for credential string
    line = (char *)malloc(sizeof(char)*100);
    while(feof(file) == 0) {
        fgets(line, 100, file);
    }
    if (strcmp(file, line) == 0) {
        found = true;
    }

    // close file and release any pointes if necessary
    fclose(file);

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

    int sock1 = socket (AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = INADDR_ANY;
    direccionServidor.sin_addr.s_addr = hton1 (INADDR_ANY);
    direccionServidor.sin_port = htons(5100);

    struct sockaddr_in direccionClient;

    if (sock1 == -1){ // Si el socketId es igual a -1 significa que no se pudo crear el socket.
        printf("Fallo la creacion del socket. /d");
        return -1; // Retorno -1 y salgo de programa.
    } else {
        printf("La creacion del socket fue exitosa. /d"); // Si el socketId tiene un estado distinto a -1 voy a considerar que el socket se creo correctamente.
    }

    bindStatus = bind(sock1, (struct direccionServidor *) &addrport, sizeof(direccionServidor)); /
    if (bindStatus == -1) {
        printf("Bind Failure. /d");
    }

    // arguments checking

    // reserve sockets and variables space

    // create server socket and check errors

    // bind master socket and check errors

    // make it listen
    int status = listen(sock1,100);
    if (status == -1) {
        printf("Error en el listen. /d");
        return -1;
    }else {
        printf("Escuchando... /d");
    }

    // main loop
    while (true) {
        // accept connectiones sequentially and check errors
        int len = sizeof(direccionClient);
        int sock2;
        if (sock2 = accept(sock1, (struct sockaddr*)&direccionClient, &len) < 0){
           printf("Error al aceptar a Cliente... /d");
        } else {
            printf("Cliente aceptado.");
        }

        // send hello

        // operate only if authenticate is true
    }

    // close server socket
    status = close(sock1);
    printf("El socket se cerro con estado <%d>",status);

    return 0;
}
