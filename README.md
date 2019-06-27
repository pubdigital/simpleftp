********Franco Mellano - Tercer año AUS********

# simpleftp
Skeleton for socket programming lectures

Instrucciones de Compilacion:
Se deberan compilar el cliente y el servidor en dos directorios distintos.

El servidor se compila y se ejecuta con:
>$ gcc myftp_skel.c -o servidor


>$ ./servidor 11000

El cliente se compila y se ejecuta con:
>$gcc myftp_skel.c -o cliente


>$ ./cliente 127.0.0.1 11000

Durante la ejecucion del cliente, el servidor pedira el usuario y contraseña que se comprobaran mediante el archivo ftpusers ubicado en el mismo directorio que el servidor. Una vez autenticado el cliente, este podra pedir un archivo que este ubicado en el mismo directorio que el servidor con el comando:
>get nombre_de_archivo

Para cerrar cliente se debe utilizar el comando:
>quit

===============================================================================================

# simpleftp concurrente

Instrucciones de Compilacion:
Se deberan compilar los clientes y el servidor en dos directorios distintos.

El servidor se compila y se ejecuta con:
>$ gcc myftpsrv_concurrent_skel.c -o servidor


>$ ./servidor 11000

El cliente se compila y se ejecuta con:
>$ gcc myftp_concurrent_skel.c -o cliente


>$ ./cliente 127.0.0.1 11000

o podria ejecutarse de la siguiente manera:

>$ ./cliente localhost 11000

Se pueden ejectuar varios clientes a la vez. Durante la ejecucion del cliente, el servidor pedira el usuario y contraseña que se comprobaran mediante el archivo ftpusers ubicado en el mismo directorio que el servidor concurrente. Una vez autenticado el cliente, este podra pedir un archivo que este ubicado en el mismo directorio que el servidor con el comando:
>get nombre_de_archivo

y tambien el cliente podra enviar un archivo que este ubicado dentro de su directorio a la ubicacion del servidor con:
>put nombre_de_archivo

Para cerrar cliente se debe utilizar el comando:
>quit


