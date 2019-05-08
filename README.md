# simpleftp
Para compilar:

```bash
$ gcc myftp_skel.c -o myftp
$ gcc myftpsrv_skel.c -o myftpsrv
```

Prueba de interacción entre cliente y servidor:

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

