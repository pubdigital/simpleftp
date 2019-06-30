# simpleftp
##### Funcionamiento
Se debe compilar el cliente y servidor
gcc myftp_skel.c -o myftp
gcc myftpsrv_skel.c -o ftpsrv

##### Servidor
El servidor va a escuchar en el puerto 11000
$ ./ftpsrv 11000

##### Cliente
El cliente se conecta y solicita la transferencia del archivo "prueba.txt"
$ ./myftp 127.0.0.1 11000
220 srvFtp version 1.0
username: estefi
331 Password required for estefi
password: est123
230 User estefi logged in
Operation: get prueba.txt
299 File prueba.txt size 3528 bytes
226 Transfer complete
Operation: quit
221 Goodbye

##### Cliente y servidor concurrentes
##### Se debe tener el cliente y el servidor en directorios separados. En el directorio del servidor deben estar los archivos:  ftpusers, el archivo de prueba y el código del servidor. En el directorio del cliente debe estar el archivo de prueba y el código del cliente.

##### Funcionamiento
Se debe compilar el cliente y servidor
gcc clientecon.c -o cliente
gcc servidorcon.c -o servidor

##### Servidor
$ ./servidor 11000

##### Cliente
$ ./cliente 127.0.0.1 11000
220 srvFtp version 2.0
Username: estefi
331 Password required for estefi
Password: est123
230 User estefi logged in
Operation: get prueba.txt
200 PORT command successful
150 Opening BINARY mode data connection for prueba.txt (3528 bytes)
226 Transfer complete
Operation: put prueba2.txt
200 PORT command successful
150 Opening BINARY mode data connection for prueba2.txt
226 Transfer complete
Operation: quit
221 Goodbye


