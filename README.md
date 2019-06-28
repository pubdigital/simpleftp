# simpleftp
Skeleton for socket programming lectures

# FTP Concurrente
**Asegurarse de compilar al servidor en un lugar distinto a los clientes.**

Compilar el SERVIDOR de manera:
> gcc serverConcurrente.c -o server

Y ejecutarlo de la forma:
> sudo ./server 11000

Para compilar el CLIENTE será:
> gcc clienteConcurrente.c -o cliente

Y ejecutarlo:
> ./cliente 127.0.0.1 11000

Los usuarios y contraseñas permitidos figuran en el archivo "ftpusers".
Finalmente los comandos del cliente son:
> get [archivo]

> put [archivo]

> quit

para obtener un archivo del servidor, guardar un archivo del usuario en el servidor y terminar la ejecucion del cliente respectivamente.
