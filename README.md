# Sistema Broker - Publisher - Subscriber (Winsock2 en C)

## Descripción general

Este proyecto implementa un sistema de mensajería tipo “publish/subscribe” usando sockets TCP con la librería Winsock2 de Windows.

- El **Broker** actúa como servidor central que recibe y distribuye mensajes.  
- El **Publisher** envía eventos o mensajes (por ejemplo, resultados de partidos).  
- El **Subscriber** se suscribe a ciertos temas y recibe solo los mensajes que coinciden con ellos.

Esta arquitectura simula el funcionamiento básico de un sistema como MQTT, pero desarrollada en C con conexiones TCP manuales.

---

## Requisitos previos

1. Sistema operativo **Windows**.  
2. Compilador compatible con **Winsock2**, como:
   - MinGW
   - Dev-C++
   - Visual Studio
3. Librería `ws2_32.lib` (ya incluida en Windows, pero debe vincularse al compilar).

---

## Compilación

## TCP

### En Dev-C++ o Code::Blocks

1. Crea un nuevo proyecto **C (no C++)**.
2. Agrega los tres archivos (`broker.c`, `publisher.c`, `subscriber.c`) al proyecto.
3. En las opciones del compilador, agrega la librería Winsock:
   ```
   -lws2_32
   ```
   (O en Dev-C++: **Project → Project Options → Parameters → Add Library → ws2_32**)
4. Compila cada archivo por separado generando tres ejecutables:
   - `broker.exe`
   - `publisher.exe`
   - `subscriber.exe`

---

### En línea de comandos (usando MinGW)

Abre una terminal en la carpeta del proyecto y ejecuta:

```bash
gcc broker.c -o broker.exe -lws2_32
gcc publisher.c -o publisher.exe -lws2_32
gcc subscriber.c -o subscriber.exe -lws2_32
```

Esto generará los tres ejecutables listos para correr.

---

### Documentación: Winsock (sockets TCP)

Esta subsección explica los conceptos y llamadas de la API de Winsock necesarios para implementar el sistema Broker - Publisher - Subscriber usando TCP en Windows. 

1) Inicialización y limpieza

- WSAStartup(): Antes de usar cualquier función de Winsock debe llamarse `WSAStartup(MAKEWORD(2,2), &wsaData)` para inicializar la biblioteca. Pide la versión 2.2 habitualmente.
- WSACleanup(): Al terminar la aplicación (o cuando ya no se usarán sockets), llamar a `WSACleanup()` para liberar recursos.

Ejemplo mínimo:

```
WSADATA wsaData;
if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
      // manejo de error
}
// ... usar sockets ...
WSACleanup();
```

2) Tipos y funciones clave

- SOCKET: tipo usado por Winsock para describir un descriptor de socket (no es un int). Valor inválido: `INVALID_SOCKET`.
- closesocket(SOCKET): cerrar un socket en Winsock (no usar close()).
- WSAGetLastError(): obtener el código de error específico de Winsock.
- htonl/htons / ntohl/ntohs: conversiones entre orden de bytes de red/host (idénticas a POSIX).

Principales llamadas para el Broker (servidor TCP):
- socket(): crear socket (AF_INET, SOCK_STREAM, IPPROTO_TCP).
- bind(): asociar dirección y puerto.
- listen(): poner socket en modo escucha.
- accept(): aceptar nuevas conexiones entrantes (devuelve un `SOCKET` por cliente).
- select() (o hilos): para multiplexar múltiples conexiones y leer/escribir sin bloquear.

Principales llamadas para Publisher/Subscriber (clientes TCP):
- socket()
- connect(): establecer conexión con el broker.
- send()/recv(): enviar y recibir datos (o sendall/recv en bucle para mensajes más largos).

3) Flujo típico (Broker)

1. Llamar a `WSAStartup()`.
2. `socket()` para crear el socket servidor.
3. `bind()` a la dirección (por ejemplo, INADDR_ANY) y puerto (ej. 6000).
4. `listen()` para empezar a aceptar conexiones.
5. Bucle principal: usar `select()` o `WSAPoll()` para detectar nuevas conexiones y datos.
    - Si `select()` indica actividad en el socket escucha -> `accept()` -> añadir nuevo `SOCKET` a la lista de monitoreo.
    - Si actividad en un socket cliente -> `recv()` leer datos y reenviarlos a los subscribers correspondientes usando `send()`.
6. Cuando termine, `closesocket()` para cada socket y `WSACleanup()`.

4) Flujo típico (Publisher / Subscriber)

1. `WSAStartup()`.
2. `socket()`.
3. `connect()` al broker (IP y puerto).
4. `send()` para publicar mensajes (publisher) o `send()` para suscribirse/informar interés (subscriber), según el protocolo de la app.
5. `recv()` para recibir mensajes reenviados por el broker (el subscriber normalmente queda en bucle esperando mensajes).
6. `closesocket()` y `WSACleanup()` al terminar.


---

## Ejecución paso a paso

### 1. Inicia el Broker

Ejecuta el servidor primero (solo una instancia):

```bash
broker_tcp.exe
```

Salida esperada:
```
Broker activo en puerto 6000
```

Esto indica que el servidor está escuchando nuevas conexiones.

---

### 2. Inicia uno o varios Subscribers

En otra consola (pueden ser varias):

```bash
subscriber_tcp.exe
```

Ejemplo de entrada:
```
Ingrese partidos a suscribirse (ej: MEXvsCOL): MEXvsCOL
Esperando confirmación...
Suscripcion exitosa
Esperando actualizaciones...
```

Ahora este cliente solo recibirá mensajes que contengan el texto “MEXvsCOL”.

---

### 3. Inicia uno o varios Publishers

En otra consola:

```bash
publisher_tcp.exe
```

Ejemplo:
```
Publisher conectado al broker en 127.0.0.1:6000
Evento (o 'salir'): MEXvsCOL Gol en X minutoo
```

El mensaje se enviará al broker, y este lo reenviará automáticamente a todos los subscribers suscritos al tema “MEXvsCOL”.

---

## Ejemplo de flujo completo

### Consola del Broker:
```
Broker activo en puerto 6000
Conectado 127.0.0.1
Conectado 127.0.0.1
```

### Consola del Subscriber:
```
Ingrese partidos a suscribirse (ej: MEXvsCOL): MEXvsCOL
Suscripcion exitosa
Esperando actualizaciones...
MEXvsCOL gol en el minuto 40
```

### Consola del Publisher:
```
Publisher conectado al broker en 127.0.0.1:6000
Evento (o 'salir'): MEXvsCOL Gol al minuto 32
```

---

## UDP

La ejecución del publicador-suscriptor, contrario al de TCP, se realizó en linux. Para ello, se recomienda realizar en Ubuntu usando WSL, o en la terminal de cualquier distribución linux.

En la terminal linux, se necesita descargar el gcc, descarga los siguientes comandos:
```
sudo apt update
sudo apt install build-essential -y
```
## Ejecución paso a paso

### 1. Iniciar broker 

En una terminal ejecutar el broker, con el siguiente comando:
```
./broker_udp
```
Al realizar lo anterior, el cliente ya estará listo para recibir mensajes de publicadores y reenviarlos a los suscriptores.

### 2. Inicia uno o varios Subscribers

En otra consola (pueden ser varias), ingresa el siguiente comando:
```
./subscriber_udp
```
Al realizar lo anterior, se le pedirá al suscriptor ingresar un tema para el partido que quiera 

### 3. Inicia uno o varios Publishers

En otra consola (pueden ser varias), ingresa el siguiente comando:
```
./publisher_udp
```
Al realizar lo anterior, se le pedirá al publicador ingresar un tema, y acto seguido se le pedirá ingresar el o los mensajes que recibirán los suscriptores de dicho tema.


## Finalización

- Para detener el **publisher**, escribe `salir` y presiona Enter en TCP. En UDP, usa Ctrl + C en la consola.  
- Para detener el **broker** o los **subscribers**, usa Ctrl + C en la consola.

---

## Notas técnicas

- `WSAStartup()` y `WSACleanup()` son obligatorios al usar Winsock.  
- `socket()`, `bind()`, `listen()` y `accept()` conforman la parte del servidor (Broker).  
- `connect()` y `send()` se usan en los clientes (Publisher y Subscriber).  
- El broker usa `select()` para manejar múltiples conexiones simultáneamente.  
- Los mensajes se filtran comparando el texto con los temas de suscripción.
