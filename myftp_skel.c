#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <math.h>

#define BUFSIZE 512

#define MSG_550 "550 %s: no such file or directory\r\n"

/**
 * Funcion que extrae la ip y puerto del socket
 *
 **/

void getIpAndPort(int sd,char * portStr){
    char ip[20];
    int port;

    struct sockaddr_in addr;
    socklen_t len=sizeof(addr);
    getsockname(sd,(struct sockaddr*)&addr,&len);
    sprintf(ip,"%s",inet_ntoa(addr.sin_addr));
    port=(uint16_t)ntohs(addr.sin_port);
    for(int i=0;i<strlen(ip);i++){
        if(ip[i]=='.')
            ip[i]=',';
    }
    if(portStr){
       sprintf(portStr,"%s,%d,%d",ip,(port/256),(port%256));
    }
}


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
    recv_s=recv(sd,buffer,BUFSIZE,0);

    // error checking
    if (recv_s < 0) {printf("warn error\n");warn("error receiving data");}
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
    send(sd,buffer,sizeof(buffer),0);

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
 *
 *
 **/
int serverConnection(int sdCtrl){
    // reserve sockets and variables space
    int sd,sd2,sin_size;
    struct sockaddr_in addr,cliente;
    char desc[100],ipAndPortStr[30];

    // create server socket and check errors
    if((sd = socket(AF_INET,SOCK_STREAM,0))==-1){
        close(sd);
        perror("Socket error: ");
        return -1;
    }
#ifdef DEBUG
    printf("Socket creado: %d\n",sd);
#endif
    
    // bind master socket and check errors
    addr.sin_family=AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(sd,(struct sockaddr *) &addr,sizeof(addr))<0){
        close(sd);
        perror("Bind error: ");
        return -1;
    }
#ifdef DEBUG
    printf("Bind ok!\n");
#endif
    
    // make it listen
    if(listen(sd,BUFSIZE)==-1){
        close(sd);
        perror("Listen error: ");
        return -1;
    }

#ifdef DEBUG
    printf("Listen ok!\n");
#endif

    getIpAndPort(sd,ipAndPortStr);
    send_msg(sdCtrl,"PORT",ipAndPortStr);
    // check for the response
    if(!recv_msg(sdCtrl,200,NULL)){
        printf("Unexpected command: %s\n",desc);
        return -1;
    }

    if((sd2 = accept(sd,(struct sockaddr *)&cliente,(socklen_t *)&sin_size))==-1){
        close(sd2);
        return -1;
    }

    return sd2;
}


/**
 * function: login process from the client side
 * sd: socket descriptor
 **/
void authenticate(int sd) {
    char *input, desc[100];
//    int code;

    // ask for user
    printf("username: ");
    input = read_input();

    // send the command to the server
    send_msg(sd,"USER",input);
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    if(!recv_msg(sd,331,desc)){
        printf("%s\n",desc);
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
    if(recv_msg(sd,530,desc)){
        printf("%s\n",desc);
        exit(0);
    }
#ifdef DEBUG
    printf("Login ok\n");
#endif
}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sdCtrl, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, sdData;
    FILE *file;

    sdData=serverConnection(sdCtrl);
    // send the RETR command to the server
    send_msg(sdCtrl,"RETR",file_name);

    // check for the response
    if(!recv_msg(sdCtrl,150,desc)){
        printf("Unexpected command: %s\n",desc);
        return;
    }
    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write
    strcat(file_name,"1");
    file = fopen(file_name, "w");

    //receive the file
    recv_s=BUFSIZE;
    while( recv_s==BUFSIZE){
        recv_s = recv(sdData,buffer,BUFSIZE,0);
        if (recv_s < 0) warn("error receiving data *");
        fwrite(buffer,1,recv_s,file);
    }
    // close the file
    fclose(file);
#ifdef DEBUG
        printf("Se copiÃ³ el archivo %s\n",file_name);
#endif

    // receive the OK from the server
    if(!recv_msg(sdCtrl,226,desc)){
        printf("%s\n",desc);
        return;
    }
}

/**
 * function: operation put
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void put(int sd, char *file_name) {
    char buffer[BUFSIZE];
    int bread;
    FILE *file;
    int datasd=serverConnection(sd);

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
    send_msg(sd,"QUIT",NULL);
    // receive the answer from the server
    if(!recv_msg(sd,221,NULL)){
        printf("Error en goodbye");
    }
}

/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/
void operate(int sdCtrl) {
    char *input, *op, *param;
 
    while (true) {
        printf("Operation: ");
        input = read_input();

#ifdef DEBUG
        printf("Input: %s\n",input);
#endif
        if (input == NULL)
            continue; // avoid empty input
        op = strtok(input, " ");
        // free(input);
        if (strcmp(op, "get") == 0) {
#ifdef DEBUG
            printf("Comando get\n");
#endif
            param = strtok(NULL, " ");
            get(sdCtrl, param);
        } 
        else if (strcmp(op, "put") == 0) {
            param = strtok(NULL, " ");
            put(sdCtrl, param);
        }
        else if (strcmp(op, "quit") == 0) {
#ifdef DEBUG
        printf("Comando quit\n");
#endif
            quit(sdCtrl);
            break;
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
   int sd;
    struct sockaddr_in addr;
    struct hostent *hostIp;
    char msg[50];
    bzero((char*)&addr,16);

    // arguments checking
    if(argc!=3){                                      //Reviso la cantidad de argumentos.
        printf("Try %s <ip-server> <port>.\n",argv[0]);
        return -1;
    }

    // get host by name
    if ((hostIp = gethostbyname(argv[1])) == NULL) {   
        perror("gethostbyname");
        exit(1);
    }
    
    for(int i = 0; i < strlen(argv[2]);i++){         //Compruebo el puerto.
        if(argv[2][i] <'0'|| argv[2][i]>'9'){
            printf("\nPort must be numeric.\n");
            return -1;
        }
    }
#ifdef DEBUG
    printf("Argumentos ok\n");
#endif

    addr.sin_port=htons(atoi(argv[2]));
    addr.sin_addr = *((struct in_addr *)hostIp->h_addr);
    addr.sin_family=AF_INET;
    
    // create socket and check for errors
    if((sd = socket(AF_INET,SOCK_STREAM,0))==-1){
        close(sd);
        perror("Socket error: ");
        exit(errno);
    }

#ifdef DEBUG
    printf("Socket creado\n");
#endif
    // set socket data


    // connect and check for errors
    if(connect(sd,(struct sockaddr *)&addr,sizeof(addr))<0){
        close(sd);
        perror("Connect error: ");
        exit(errno);
    }

#ifdef DEBUG
    printf("Connect ok\n");
#endif

    // if receive hello proceed with authenticate and operate if not warning
    if(recv_msg(sd,220,msg)){
        printf("Rcv: %s\n",msg);
    }
    else{
        printf("Warning\n");
    }

    authenticate(sd);
    operate(sd);
    // close socket
    close(sd);
#ifdef DEBUG
    printf("Fin\n");
#endif
    return 0;
}
