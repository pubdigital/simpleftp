# simpleftp
Skeleton for socket programming lectures

Instrucciones de Compilacion:
Se deberan compilar el clientes y el servidor en dos directorios distintos.

El servidor se compila y se ejecuta con:
$gcc myftp_skel.c -o servidor
$./servidor 11000

El cliente se compila y se ejecuta con:
$gcc myftp_skel.c -o cliente
$./cliente 127.0.0.1 11000

Durante la ejecucion el servidor debera pedir el usuario y contraseña que se comprobaran mediante el archivo ftpusers ubicado en el mismo directorio que el servidor. Una vez autenticado el cliente, este podra pedir un archivo que este ubicado en el mismo directorio que el servidor con el comando "get nombre_de_archivo".



