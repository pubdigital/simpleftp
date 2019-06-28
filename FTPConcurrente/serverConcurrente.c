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

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"
#define MSG_150(OPTION) ((OPTION != 0) ? ("150 Opening BINARY mode data connection for %s (%ld bytes)\r\n"):("150 Opening BINARY mode data connection for %s\r\n"))
#define MSG_200 "200 PORT command successful\r\n"

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
    if(send(sd, buffer, BUFSIZE, 0) < 0){
        printf("Error al enviar mensaje: %s\n", buffer);
        return false;
    }
    return true;
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
    if((file = fopen(path, "r")) == NULL){
    	printf("No existe el archivo.\n");
    	exit(-1);
    }

    // search for credential string
    line = (char*)malloc(sizeof(char) * PARSIZE);
    while(!feof(file)){                
        fscanf(file, "%s", line);

        if(strcmp(cred, line) == 0){
        	found = true;
        }
    }
  
    // close file and release any pointers if necessary
    free(line);
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
    if(recv_cmd(sd, "USER", user) != true){
    	return false;
    }

    // ask for password
    send_ans(sd, MSG_331, user);

    // wait to receive PASS action
    recv_cmd(sd, "PASS", pass);

    // if credentials don't check denied login
    if(!check_credentials(user, pass)){
        send_ans(sd, MSG_530);
        if (close(sd) > 0){
        	printf("No se cerro el socket.\n");
        	exit(-1);
        }
        printf("Error al autenticarse.\n");
        return false;
    }

    // confirm login
    send_ans(sd, MSG_230, user);
    printf("El cliente %s se conecto correctamente.\n", user);
    return true;
}

/**
 * function: funcion store
 **/
void stor(int sd, int sddata, char *file_path) {
    FILE *file;    
    int recv_s;
    long fsize;
    int r_size = BUFSIZE;
    char desc[BUFSIZE];
    char buffer[BUFSIZE];

    send_ans(sd, MSG_200);     
    
    send_ans(sd, MSG_150(0), file_path);
    
    // open file
    file = fopen(file_path, "wb");
    
    // receive file
    while(1) {
        recv_s = read(sddata, desc, r_size);

        if (recv_s > 0) fwrite(desc, 1, recv_s, file);

        if (recv_s < r_size){
            fclose(file);
            break;
        } 
    }

    // send succes message
    send_ans(sd, MSG_226);
}

/**
 * function: RETR operation (retrieve)
 * sd: socket descriptor
 * file_path: name of the RETR file
 **/
void retr(int sd, int sddata, char *file_path) {
    FILE *file;    
    int bread;
    long fsize;
    char buffer[BUFSIZE];

    // check if file exists if not inform error to client
    if((file = fopen(file_path, "rb")) == NULL){
        send_ans(sd, MSG_550, file_path);
        return;
    }

    send_ans(sd, MSG_200); 

    // send a success message with the file length
    fseek(file, 0, SEEK_END);
    fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    send_ans(sd, MSG_150(1), file_path, fsize);

    // important delay for avoid problems with buffer size
    sleep(1);

    // send the file
    while(1){    
        bread = fread(buffer, 1, BUFSIZE, file);
        if(bread <= 0){ 
            break;
        }
        send(sddata, buffer, bread, 0);
    }

    // close the file
    sleep(2);
    fclose(file);

    // send a completed transfer message
    send_ans(sd, MSG_226);
}

/**
 * function: separar string para formateo
 **/
void getClientIPPort(char *str, char *client_ip, int *client_port) {
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
 * function: crear coneccion de datos
 **/
int setupDataConnection(int *datasd, char *client_ip, int client_port, int server_port) {
    struct sockaddr_in cliaddr, tempaddr;

    if ((*datasd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket error");
        return -1;
    }

    server_port++;
    tempaddr.sin_family = AF_INET;
    tempaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tempaddr.sin_port = htons(server_port); 
    memset(&(tempaddr.sin_zero), '\0', 8); 
     
    while((bind(*datasd, (struct sockaddr*) &tempaddr, sizeof(tempaddr))) < 0) {
        server_port++;        
        tempaddr.sin_port = htons(server_port);
    }
           
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(client_port);
    memset(&(cliaddr.sin_zero), '\0', 8); 
    if (inet_pton(AF_INET, client_ip, &cliaddr.sin_addr) <= 0){
        perror("inet_pton error");
        return -1;
    }

    if (connect(*datasd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0){
        perror("connect error");
        return -1;
    }

    return 0;
}

void sigchld_handler(int s) {
    while(wait(NULL) > 0);
}

/**
 *  function: execute all commands
 *  sd: socket descriptor
 **/
void operate(int sd, int sddata) {
    char op[CMDSIZE], param[PARSIZE];

    while (true) {
        op[0] = param[0] = '\0';
        // check for commands send by the client if not inform and exit
        recv_cmd(sd, op, param);

        if (strcmp(op, "RETR") == 0) {
            retr(sd, sddata, param);
        } else if(strcmp(op, "STOR") == 0){
            stor(sd, sddata, param);
        } else if(strcmp(op, "QUIT") == 0){
            // send goodbye and close connection
            send_ans(sd, MSG_221);
            if (close(sd) > 0 || close(sddata) > 0){
            	printf("Error al cerrar el socket.\n");
            	exit(-1);
            }
            else{
                printf("Finalizo la sesion con QUIT.\n");
            }
            break;
        } else{
            // invalid command
            printf("Se recibio un comando invalido.\n");
            // future use
        }
    }
}

/**
 * Run with
 *         ./mysrv <SERVER_PORT>
 **/
int main (int argc, char *argv[]) {

    // arguments checking
    if (argc!=2){
    	printf("Cantidad invalida de argumentos.\n");
    	exit(-1);
    }

    // reserve sockets and variables space
    int socketServer, fileDescriptor, serverPort, sin_size;
    struct sockaddr_in serverDir, clienteDir;
    struct sigaction sig;

    serverDir.sin_family = AF_INET;         
    serverDir.sin_port = htons(*argv[1]);   
    serverDir.sin_addr.s_addr = INADDR_ANY; 
    memset(&(serverDir.sin_zero), '\0', 8); 
    
    sscanf(argv[1], "%d", &serverPort);

    // create server socket and check errors
    if ((socketServer = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    	printf("Error en creacion del socket.\n");
    	exit(-1);
    } else{
        printf("Creacion del socket exitosa.\n");
    }

    // bind master socket and check errors
    if (bind(socketServer, (struct sockaddr *)&serverDir, sizeof(struct sockaddr_in)) == -1){
    	printf("No se completo el bind.\n");
    	exit(-1);
    } else{
        printf("Se creo el enlace con el puerto %d con exito.\n", serverPort);
    }

    // make it listen
    if (listen(socketServer, BACKLOG) < 0){
    	printf("No se pudo hacer el listen.\n");
    	exit(-1);
    } else{
        printf("Escuchando.\n");
    }

    // chequeando procesos zombies
    sig.sa_handler = sigchld_handler; 
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sig, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // main loop
    while (true) {
    	
        // accept connections sequentially and check errors
        sin_size = sizeof(struct sockaddr_in);
        if ((fileDescriptor = accept(socketServer, (struct sockaddr *)&clienteDir,(socklen_t *)&sin_size)) == -1) {
            perror("No se pudo aceptar.\n");
            continue;
        }
        printf("Se acepto la conexion con un nuevo cliente: %s\n", inet_ntoa(clienteDir.sin_addr));
        
        // send hello
        send_ans(fileDescriptor, MSG_220);

        // operate only if authenticate is true
        if(!authenticate(fileDescriptor)) close(fileDescriptor);
        else {
            if (!fork()) {

                close(socketServer);

                int dataSocket, client_port = 0;
                char recvline[BUFSIZE+1];
                char client_ip[50];
                
                // pedir puerto y guardarlo
                recv_cmd(fileDescriptor, "PORT", recvline);
                getClientIPPort(recvline, client_ip, &client_port);
                    
                // abrir connexion de datos
                if((setupDataConnection(&dataSocket, client_ip, client_port, serverPort)) < 0) {
                   break;
                }
                
                operate(fileDescriptor, dataSocket);
                
                close(dataSocket);
                close(fileDescriptor);
                exit(0);
            }
        }
        close(fileDescriptor);
    }

    // close server socket
    if (close(socketServer) > 0){
    	printf("No se pudo cerrar el socket.\n");
    	exit(-1);
    }

    return 0;
}