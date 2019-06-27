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
 * function: si hay error, se imprime mensaje y se corta el programa
 **/
void exitwithmsg(char * msg){
    printf("%s\n", msg);
    exit(-1);
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
    if((send(sd, buffer, BUFSIZE, 0)) < 0){
        exitwithmsg("Error en el envio de mensaje");
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

    sleep(1);

    // ask for user
    printf("Usuario: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "USER", input);

    sleep(1);
    
    // relese memory
    free(input);

    // wait to receive password requirement and check for errors
    recv_msg(sd, 331, desc);

    // ask for password
    printf("Contraseña: ");
    input = read_input();

    // send the command to the server
    send_msg(sd, "PASS", input);

    // release memory
    free(input);

    // wait for answer and process it and check for errors
    if(!recv_msg(sd, 230, desc)){
       exitwithmsg("Error en Autenticacion!");
    }
}

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/
void get(int sd,int dsd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    send_msg(sd, "RETR", file_name);

    // check for the response
    if(recv_msg(sd, 550, NULL) == true) return;

    recv_msg(sd, 200, NULL);
    // parsing the file size from the answer received
    // "File %s size %ld bytes"
    sscanf(buffer, "File %*s size %d bytes", &f_size);

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
    fclose(file);
    // receive the OK from the server
    recv_msg(sd, 226, NULL);
}
void put(int sd, int datasd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    int bread;
    FILE *file;

    if ((file = fopen(file_name, "r")) == NULL) {
        printf(MSG_550, file_name);
        return;
    }

    send_msg(sd, "STOR", file_name);
    
    recv_msg(sd, 200, NULL);

    recv_msg(sd, 150, NULL);
 
    while(1) {
        bread = fread(buffer, 1, BUFSIZE, file);
        if (bread > 0) {
            send(datasd, buffer, bread, 0);
            sleep(1);
        }
        if (bread < BUFSIZE) break;
    }
    
    fclose(file);

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
 * function: make all operations (get|quit)
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
void get_ip_port(int sd, char *ip, int *port) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    getsockname(sd, (struct sockaddr*) &addr, &len);

    sprintf(ip,"%s", inet_ntoa(addr.sin_addr));

    *port = (uint16_t)ntohs(addr.sin_port);
}
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
    if(argc<2){
        printf("<host> <puerto>\n");
        return 1;
    }
    int sd,resp_size,valor,puerto;
    int listensd, datasd, n5, n6, x;
    struct sockaddr_in addr, addr2;
    char buffer[BUFSIZE];
    struct hostent *he;
    char ip[50], str[BUFSIZE+1];
    
    puerto = (atoi(argv[2]));
        
    if ((he = gethostbyname(argv[1])) == NULL) {   
        exitwithmsg("gethostbyname");
    }
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1){
        exitwithmsg("No se puede crear el socket\n");
    }
    if(puerto < 1  || puerto > 65535){
        exitwithmsg("No existe ese puerto \n");
    }

    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto);

    // Connect to server
    if (connect(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sd);
        exitwithmsg("Connection failed\n"); 
    }else{
        if((resp_size = recv(sd, buffer, BUFSIZE, 0))> 0) {
            printf( "Me llego del servidor: %s \n", buffer);
            authenticate(sd);

            listensd = socket(AF_INET, SOCK_STREAM, 0);

            addr2.sin_family = AF_INET;
            addr2.sin_port = htons(0); 
            addr2.sin_addr.s_addr = htonl(INADDR_ANY);
            memset(&(addr2.sin_zero), '\0', 8);

            bind(listensd, (struct sockaddr*) &addr2, sizeof(addr2));
            listen(listensd, BACKLOG);

            get_ip_port(sd, ip, (int *) &x);

            get_ip_port(listensd, str, (int *) &puerto);

            convert(puerto, &n5, &n6);

            get_port_string(str, ip, n5, n6);

            send_msg(sd, "PORT", str);

            datasd = accept(listensd, (struct sockaddr*) NULL, NULL);

            operate(sd, datasd);

            close(datasd);
        }else{
            exitwithmsg("Servidor desconectado!");
        }
    }
    return 0;
}
