### COMPILAR FUENTES
```
gcc myftp_skel.c -o cliente
gcc myftpsrv_skel.c -o servidor
```

### EJECUTAR
```
sudo ./servidor 8080
sudo ./cliente 127.0.0.1 8080
```

### UNA VEZ LEVANTADO CLIENTE SERVIDOR REALIZAMOS LA TRANFERENCIA (CHEQUEAR QUE LOS ARCHIVOS ESTEN EN CARPETAS SEPARADAS)
```
220 srvFtp version 1.0
Insert user: juanma
331 Password required for juanma
Insert password: juan1234
230 User juanma logged in
Operation: get test.txt
299 File test.txt size 8 bytes
226 Transfer complete
Operation: quit
221 Goodbye
```
