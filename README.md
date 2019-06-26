# simpleftp
##### Compilar:

```bash
$ gcc myftp_skel.c -o myftp
$ gcc myftpsrv_skel.c -o myftpsrv
```



##### Prueba de interacción entre cliente y servidor:

```bash
$ ./myftp 127.0.0.1 11000
220 srvFtp version 1.0
username: user1
331 Password required for user1
passwd: pass1
230 User user1 logged in
Operation: get prueba.txt
299 File prueba.txt size 5564 bytes
226 Transfer complete
Operation: get prueba1.txt
550 prueba1.txt: no such file or directory
Operation: quit
221 Goodbye
```



##### NOTA: Ejecutar cliente y servidor en directorios diferentes.

------

##### Archivos directorio cliente:

```bash
$ ls
myftp_skel.c
```

##### Archivos directorio servidor:

```bash
$ ls
ftpusers  myftpsrv_skel.c  prueba.txt  
```



# simpleftp / servidor concurrente

##### Compilar:

```bash
$ gcc clientFtp.c -o clientFtp
$ gcc serverFtp.c -o serverFtp
```



##### Prueba de interacción entre cliente y servidor:

```bash
$ ./clientFtp localhost 11000
220 srvFtp version 1.0
username: user1
331 Password required for user1
passwd: pass1
230 User user1 logged in
Operation: get prueba.txt
200 PORT command successful
150 Opening BINARY mode data connection for prueba.txt (5564 bytes)
226 Transfer complete
Operation: put prueba1.txt
200 PORT command successful
150 Opening BINARY mode data connection for prueba1.txt
226 Transfer complete
Operation: quit
221 Goodbye
```



##### NOTA: Ejecutar cliente y servidor en directorios diferentes.

------

##### Archivos directorio cliente:

```bash
$ ls
clientFtp.c  prueba1.txt
```

##### Archivos directorio servidor:

```bash
$ ls
ftpusers  prueba.txt  serverFtp.c
```