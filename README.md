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

## Ejecución paso a paso

### 1. Inicia el Broker

Ejecuta el servidor primero (solo una instancia):

```bash
broker.exe
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
subscriber.exe
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
publisher.exe
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

## Finalización

- Para detener el **publisher**, escribe `salir` y presiona Enter.  
- Para detener el **broker** o los **subscribers**, usa Ctrl + C en la consola.

---

## Notas técnicas

- `WSAStartup()` y `WSACleanup()` son obligatorios al usar Winsock.  
- `socket()`, `bind()`, `listen()` y `accept()` conforman la parte del servidor (Broker).  
- `connect()` y `send()` se usan en los clientes (Publisher y Subscriber).  
- El broker usa `select()` para manejar múltiples conexiones simultáneamente.  
- Los mensajes se filtran comparando el texto con los temas de suscripción.
