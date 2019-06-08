# simpleftp
Skeleton for socket programming lectures

#Funcionamiento
Se debe compilar el cliente y servidor
gcc myftp_skel.c -o myftp
gcc myftpsrv_skel.c -o ftpsrv

#Servidor
El servidor va a escuchar en el puerto 11000
$ ./ftpsrv 11000

#Cliente
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
