#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <err.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 512

#define FINPUERTO 65535

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
       if (send(sd, res, strlen(res) + 1, 0)<0){
	      warn("Error al enviar comando\n");
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
    
    send_msg(sd, "USER", input);  
 
    // relese memory

    free(input);

    // wait to receive password requirement and check for errors
   
    code = 331;
    if (!recv_msg(sd, code, NULL)) {
   	errx(1,"El servidor no responde");
    
    // ask for password

    printf("passwd: ");
    input = read_input();

    // send the command to the server
     
    send_msg(sd, "PASS", input);
     
    // release memory

    free(input);

    // wait for answer and process it and check for errors
    
     code=230;
    if (!recv_msg(sd, code, NULL)) {
	   errx(1,"Error al loguearse");
    }      
 }

/**
 * function: operation get
 * sd: socket descriptor
 * file_name: file name to get from the server
 **/

void get(int sd, char *file_name) {
    char desc[BUFSIZE], buffer[BUFSIZE];
    int f_size, recv_s, r_size = BUFSIZE;
    FILE *file;

    // send the RETR command to the server
    // check for the response
    // parsing the file size from the answer received
    // "File %s size %ld bytes"

    sscanf(buffer, "File %*s size %d bytes", &f_size);

    // open the file to write

    file = fopen(file_name, "w");

    //receive the file
    // close the file

    fclose(file);

    // receive the OK from the server
}

/**
 * function: operation quit
 * sd: socket descriptor
 **/

void quit(int sd) {

    // send command QUIT to the client
    // receive the answer from the server

}

/**
 * function: make all operations (get|quit)
 * sd: socket descriptor
 **/

void operate(int sd) {
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
            get(sd, param);
        }
        else if (strcmp(op, "quit") == 0) {
            quit(sd);
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

    // arguments checking

    int i, cont_ip, cont_puerto;

    if(argc!=3){    //se comprueba si se recibe la cantidad de argumentos indicados
       printf("Ingrese la direccion IP y puerto del servidor\n");
       exit(1);
    }
    else{
       for(i = 0; i < strlen(argv[1]);i++){ //se toma el primer argumento recibido y analiza caracter por caracter si es un numero o punto
        cont_ip = argv[1][i]-48;    //resta 48 para convertir el caracter en numero
        if((cont_ip < 0 || cont_ip >9) && (cont_ip != -2)){ //-2 es el resultado de restar 48 al codigo ascii del punto (.)
           printf("IP ingresada no valida\n");
                   exit(1);
                }
       }
    }
       for(i = 0; i < strlen(argv[2]);i++){ //se toma el segundo argumento y analiza caracter por caracter si es un numero o no
           cont_puerto = argv[2][i]-48;
           if(cont_puerto < 0 || cont_puerto >9){   
              printf("Puerto ingresado no valido\n");
                  exit(1);
               }
       }
       if ((atoi(argv[2])<0)||(atoi(argv[2])>FINPUERTO)){   //se analiza si el puerto esta dentro del rango adecuado (0-65535)
        printf("Ingrese un puerto valido\n");
        exit(1);
       }

    // create socket and check for errors

    sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0) {
           printf("Error en la creacion del socket\n");
           exit(1);
    }

    // set socket data    


    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    // connect and check for errors
 
    if(connect(sd,(struct sockaddr *)&addr, sizeof(addr)) < 0){
           printf("Error al conectar el socket\n");
           exit(1);
   }  
  
    // if receive hello proceed with authenticate and operate if not warning
 
     	if recv_msg(sd, 220, NULL){
           authenticate(sd);
	          operate(sd);	
	}
      else{
         warn("Error al recibir mensaje\n");
	}    
 
    // close socket
 
     close(sd);
 
 return 0;
}
