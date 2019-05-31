# simpleftp
Skeleton for socket programming lectures


## Funcionamiento del socket

Para compilar el **cliente**:

```
$ gcc myftp_skel.c -o clFtp
```

Para compilar el **servidor**:

```
$ gcc myftpsrv_skel.c -o srvFtp
```


### Probando la interacción entre cliente y servidor:

El _servidor_ escuchará en el puerto 11000:

```
$ ./srvFtp 11000
```

El _cliente_ se conecta y le solicita un archivo:

```
$ ./clFtp 127.0.0.1 11000
220 srvFtp version 1.0
Username: nano
331 Password required for nano
Password: passnano
230 User nano logged in
Operation: get letra.txt
299 File letra.txt size 1589 bytes
226 Transfer complete
Operation: quit
221 Goodbye
```
**Para comprobar que la transferencia del fichero ha salido correctamente, es necesario
utilizar desde el shell el comando diff para que compare el fichero original y el fichero
transferido desde el servidor al cliente
