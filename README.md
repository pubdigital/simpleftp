# simpleftp
Skeleton for socket programming lectures


## Funcionamiento del socket

Para compilar el **cliente**:

```
$ gcc cliente-ftp.c -o clFtp
```

Para compilar el **servidor**:

```
$ gcc servidor-ftp.c -o srvFtp
```

### Para el debido funcionamiento del socket, el cliente y el servidor deben ejecutarse en directorios diferentes.

## Archivos en el directorio del cliente:

```
$ ls
clFtp cliente-ftp.c prueba.txt
```

## Archivos en el directorio del servidor:

```
$ ls
ftpusers letra.txt servidor-ftp.c srvFtp
```

#### Probando la interacción entre cliente y servidor:

El _servidor_ escuchará en el puerto 11000:

```
$ ./srvFtp 11000
```

Un _cliente_ se conecta para solicitarle un archivo:

```
$ ./clFtp localhost 11000
220 srvFtp version 2.0
Username: nano
331 Password required for nano
Password: passnano
230 User nano logged in
Operation: get letra.txt
200 PORT command successful
150 Opening BINARY mode data connection for letra.txt (1591 bytes)
226 Transfer complete
Operation: quit
221 Goodbye
```

Al mismo tiempo, otro _cliente_ se conecta para enviarle un archivo:

```
$ ./clFtp localhost 11000
220 srvFtp version 2.0
Username: admin
331 Password required for admin
Password: admin
230 User admin logged in
Operation: put prueba.txt
200 PORT command successful
150 Opening BINARY mode data connection for prueba.txt
226 Transfer complete
Operation: quit
221 Goodbye
```

**Para comprobar que la transferencia del fichero ha salido correctamente, es necesario utilizar desde el shell el comando diff para que compare el fichero original y el fichero transferido desde el servidor al cliente**
