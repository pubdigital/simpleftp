Antes de comenzar la prueba tener en cuenta:
*	Compilar el Cliente: 	$ gcc client-ftp.c -o cltftp
*	Compilar el Servidor: 	$ gcc server-ftp.c -o srvftp
*	El cliente y el servidor deben ejecutarse en directorios diferentes
*	Archivos en Cliente: $ ls
	cltftp   client-ftp.c   example_client.txt
*	Archivos en Servidor: $ ls
	ftpusers   example_server.txt   server-ftp.c   srvftp

Prueba a realizar:
El Servidor escuchará en el puerto 11000:
  $ ./srvftp 11000

Ahora un Cliente se conecta para solicitarle un archivo al Servidor:
  $ ./cltftp localhost 11000
  220 srvFtp version 2.0
  Username: user1
  331 Password required for user1
  Password: user1pass
  230 User user1 logged in
  Operation: get example_server.txt
  200 PORT command successful
  150 Opening BINARY mode data connection for example_server.txt (2775 bytes)
  226 Transfer complete
  Operation: quit
  221 Goodbye

Al mismo tiempo, se conecta otro Cliente para enviarle un archivo al Servidor:
  $ ./cltftp localhost 11000
  220 srvFtp version 2.0
  Username: user2
  331 Password required for user2
  Password: user2pass
  230 User user2 logged in
  Operation: put example_client.txt
  200 PORT command successful
  150 Opening BINARY mode data connection for example_client.txt
  226 Transfer complete
  Operation: quit
  221 Goodbye

Por último, podemos comprobar si es necesario, que la transferencia de los ficheros fue satisfactoria, mediante el comando diff para comparar los ficheros originales contra los ficheros transferidos.

