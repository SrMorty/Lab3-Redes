# Explicación de Librerías Usadas en el laboratorio Publisher–Subscriber

Este laboratorio implementa un modelo **Publicador–Suscriptor** utilizando los protocolos **TCP y UDP** en **Windows**, haciendo uso de la biblioteca **Winsock2** para la comunicación en red.

A continuación se describen las librerías empleadas y su función dentro del programa:

---

## 1. #include <stdio.h>

### Descripción:
La librería **Standard Input Output** provee las funciones básicas para manejar la entrada y salida estándar del programa.

### Funcionalidad:
Permite imprimir mensajes en consola, leer texto ingresado por el usuario y mostrar información del sistema.

### Principales funciones utilizadas:
- `printf()`: Muestra mensajes o datos por pantalla.  
- `fgets()`: Lee una línea de texto desde la entrada estándar (teclado).  



## 2. #include <stdlib.h>

### Descripción:
La librería Standard Library contiene funciones generales del lenguaje C para el manejo de memoria, control del programa y conversiones de datos.

### Funcionalidad:
Permite finalizar el programa en caso de error, gestionar memoria dinámica y convertir cadenas a valores numéricos.

### Principales funciones utilizadas:

- exit(): Termina la ejecución del programa de manera controlada.
- malloc() / free(): Reservan y liberan memoria dinámica.
- atoi(): Convierte una cadena de texto en un número entero.


## 3. #include <string.h>

### Descripción:
Esta librería contiene funciones para manipular cadenas de caracteres (strings) en C.

### Funcionalidad:
Permite copiar, comparar, concatenar o dividir cadenas de texto, lo cual es útil para procesar los mensajes enviados y recibidos entre los clientes y el broker.

### Principales funciones utilizadas:
- strcpy(): Copia el contenido de una cadena a otra.
- strcmp(): Compara dos cadenas de texto.
- strtok(): Divide una cadena en fragmentos (tokens) según un delimitador, como espacios o comas.
- strlen(): Devuelve la longitud de una cadena de texto.

## 4. #include <winsock2.h>

### Descripción:
Es la librería de red de Windows, conocida como Windows Sockets 2 (Winsock2).
Permite crear y manejar conexiones de red utilizando protocolos como TCP y UDP.

### Funcionalidad:
Proporciona todas las funciones necesarias para enviar y recibir datos, crear sockets, conectar clientes con servidores y controlar el flujo de la comunicación.

### Principales funciones utilizadas:
- WSAStartup(): Inicializa la librería Winsock antes de crear sockets.
- socket(): Crea un socket (puerto lógico de comunicación).
- bind(): Asocia un socket a una dirección IP y un puerto.
- listen() / accept(): Esperan y aceptan conexiones entrantes (solo en el servidor TCP).
- connect(): Conecta un cliente con el servidor.
- send() / recv(): Envía y recibe datos en TCP.
- sendto() / recvfrom(): Envía y recibe datos en UDP.
- closesocket(): Cierra un socket cuando ya no se usa.
- WSACleanup(): Libera los recursos utilizados por Winsock al finalizar el programa.

---

# Instrucciones para Compilar y Ejecutar el Proyecto

## Requisitos Previos

1. **Sistema operativo:** Windows 10 o superior.  
2. **Compilador C:** [MinGW](https://sourceforge.net/projects/mingw/) o [TDM-GCC](https://jmeubank.github.io/tdm-gcc/).  
   - Asegurarse de agregar la ruta de `gcc.exe` a la variable de entorno **PATH**.  
3. **Archivos del proyecto:**  
   Debes tener los seis programas C:
   - `broker_tcp.c`
   - `publisher_tcp.c`
   - `subscriber_tcp.c`
   - `broker_udp.c`
   - `publisher_udp.c`
   - `subscriber_udp.c`


## Compilación

Abrir una terminal en la carpeta del proyecto y ejecutar los siguientes comandos (si no estan los .exe en la carpeta src):

```bash
gcc broker_tcp.c -o broker_tcp.exe -lws2_32
gcc publisher_tcp.c -o publisher_tcp.exe -lws2_32
gcc subscriber_tcp.c -o subscriber_tcp.exe -lws2_32
gcc broker_udp.c -o broker_udp.exe -lws2_32
gcc publisher_udp.c -o publisher_udp.exe -lws2_32
gcc subscriber_udp.c -o subscriber_udp.exe -lws2_32
```

## Ejecución

### Paso 1. Inicia el Broker

Primero se debe ejecutar el servidor broker (ya sea el udp o tcp), ya que los demás clientes se conectan a él.
```bash
.\broker_tcp.exe 

```
o
```bash
.\broker_udp.exe 

```

### Paso 2. Iniciar uno o varios Suscriptores

En otras terminales (se necesita una terminal por suscriptor, es decir, si se quieren 3 suscriptores se tienen que abrir 3 terminales y en cada una escribir el codigo de abajo), ejecutar el suscriptor:

```bash
cd ruta_donde_este_el_subscriber_tcp.exe
.\subscriber_tcp.exe 

```
o
```bash
cd ruta_donde_este_el_subscriber_udp.exe
.\subscriber_udp.exe 

```

Al iniciar, debes escribir un comando para suscribirte a uno o varios partidos(deben estar separados por espacio):

```bash
EQUIPO1vsEQUIPO2 EQUIPO2vsEQUIPO3
```

El suscriptor quedará a la espera de mensajes relacionados con esos temas.


### Paso 3. Inicia uno o varios Publicadores

En otras terminales (se necesita una terminal por publicador, es decir, si se quieren 3 publicadores se tienen que abrir 3 terminales y en cada una escribir el codigo de abajo), ejecutar el publicador:

```bash
cd ruta_donde_este_el_publisher_tcp.exe
.\publisher_tcp.exe 
```
o
```bash
cd ruta_donde_este_el_publisher_udp.exe
.\publisher_udp.exe 
```

Enviar un mensaje indicando el tema y el contenido, por ejemplo:

```bash
EQUIPO1vsEQUIPO2 Gol del delantero!
```

El Broker recibirá ese mensaje y lo reenviará solo a los suscriptores que estén registrados al partido EQUIPO1vsEQUIPO2.
