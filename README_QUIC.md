# Sistema Pub/Sub con Protocolo QUIC

## ¿Qué es QUIC?

QUIC es un protocolo **híbrido** que combina:
- **UDP** (velocidad - sin handshake de conexión)
- **TCP** (confiabilidad - números de secuencia, ACKs y retransmisión)

**QUIC = UDP + Confiabilidad TCP**

---

## Características Implementadas

✅ **Transporte UDP** - Sin handshake, rápido  
✅ **Números de secuencia** - Ordenamiento de mensajes  
✅ **ACKs** - Confirmación de recepción  
✅ **Detección de pérdidas** - Identifica paquetes perdidos  
✅ **Retransmisión automática** - Recupera paquetes perdidos  
✅ **Suscripción múltiple** - Un subscriber puede seguir varios partidos  
✅ **Filtrado por tema** - Solo recibe mensajes suscritos  

---

## Compilación

### Requisitos
- Windows con MinGW (GCC)
- Terminal PowerShell o CMD

### Navegar a la Carpeta del Proyecto

**Copiar y pegar esto en PowerShell:**
```bash
cd "C:\Users\57300\OneDrive - Universidad de los Andes\Documentos\Andes\Noveno Semestre\Infracom\Laboratorio3\Lab3-Redes"
```

### Compilar los Programas

```bash
# Compilar Broker
gcc src/broker_quic.c -o broker_quic.exe -lws2_32

# Compilar Publisher
gcc src/publisher_quic.c -o publisher_quic.exe -lws2_32

# Compilar Subscriber
gcc src/subscriber_quic.c -o subscriber_quic.exe -lws2_32
```

### Verificar Compilación
```bash
dir *.exe
```

Deberías ver:
```
broker_quic.exe
publisher_quic.exe
subscriber_quic.exe
```

---

## Escenarios de Prueba

### 📋 Escenario 1: Mensajes Básicos (Sin Pérdida)

**Objetivo:** Verificar funcionamiento normal del sistema

#### Preparación: Navegar a la Carpeta
En cada terminal, primero ejecuta:
```bash
cd "C:\Users\57300\OneDrive - Universidad de los Andes\Documentos\Andes\Noveno Semestre\Infracom\Laboratorio3\Lab3-Redes"
```

#### Paso 1: Iniciar Broker
**Terminal 1:**
```bash
cls
broker_quic.exe
```

**Salida esperada:**
```
=== BROKER QUIC ===
Puerto: 7000 (UDP)
Esperando mensajes...
```

#### Paso 2: Iniciar Subscriber
**Terminal 2:**
```bash
cls
subscriber_quic.exe
```

**Ingresa:**
```
Colombia vs Argentina
```

**Salida esperada:**
```
=== SUBSCRIBER QUIC ===
Broker: 127.0.0.1:7000

Ingrese temas separados por comas (ej: Colombia vs Argentina, Brasil vs Uruguay)
O un solo tema (ej: Colombia vs Argentina): Colombia vs Argentina
[->] Enviando suscripción a 'Colombia vs Argentina'...
[<-] Confirmación: OK

Suscrito a 1 tema(s). Esperando mensajes...
```

#### Paso 3: Iniciar Publisher
**Terminal 3:**
```bash
cls
publisher_quic.exe
```

#### Paso 4: Enviar Mensajes
En el publisher, envía:
```
Colombia vs Argentina:Gol de Colombia al minuto 10
Colombia vs Argentina:Tarjeta amarilla para Díaz
Colombia vs Argentina:Fin del primer tiempo
```

#### Resultado Esperado

**Subscriber verá:**
```
[RX] [Colombia vs Argentina] seq=1: Gol de Colombia al minuto 10
[RX] [Colombia vs Argentina] seq=2: Tarjeta amarilla para Díaz
[RX] [Colombia vs Argentina] seq=3: Fin del primer tiempo
```

✅ Secuencia continua 1, 2, 3  
✅ Sin mensajes de pérdida  
✅ Sin retransmisiones  
✅ Muestra el tema entre corchetes [Tema]  

---

### 📋 Escenario 2: Suscripción Múltiple

**Objetivo:** Verificar que un subscriber puede recibir de múltiples partidos

#### Paso 1: Iniciar Broker (si no está activo)
```bash
cls
broker_quic.exe
```

#### Paso 2: Subscriber con Múltiples Temas
**Terminal 2:**
```bash
cls
subscriber_quic.exe
```

**Ingresa:**
```
Colombia vs Argentina, Brasil vs Uruguay
```

**Salida esperada:**
```
[->] Enviando suscripción a 'Colombia vs Argentina'...
[<-] Confirmación: OK
[->] Enviando suscripción a 'Brasil vs Uruguay'...
[<-] Confirmación: OK

Suscrito a 2 tema(s). Esperando mensajes...
```

#### Paso 3: Enviar a Diferentes Partidos
**Terminal 3:**
```bash
cls
publisher_quic.exe
```

Envía:
```
Colombia vs Argentina:Gol de Colombia
Brasil vs Uruguay:Gol de Brasil
Colombia vs Argentina:Tarjeta roja
Brasil vs Uruguay:Final del partido
```

#### Resultado Esperado

**Subscriber verá:**
```
[RX] [Colombia vs Argentina] seq=1: Gol de Colombia
[RX] [Brasil vs Uruguay] seq=1: Gol de Brasil
[RX] [Colombia vs Argentina] seq=2: Tarjeta roja
[RX] [Brasil vs Uruguay] seq=2: Final del partido
```

✅ Recibe mensajes de AMBOS partidos  
✅ **Secuencias independientes por tema:** Colombia tiene seq=1,2... y Brasil tiene seq=1,2... (cada tema con su propia secuencia)  
✅ Muestra el tema de cada mensaje entre corchetes  

---

### 📋 Escenario 3: Filtrado por Tema

**Objetivo:** Verificar que subscribers solo reciben sus temas suscritos

#### Configuración

**Subscriber 1 (Terminal 2):**
```bash
cls
subscriber_quic.exe
```
Ingresa: `Colombia vs Argentina`

**Subscriber 2 (Terminal 4 - nueva):**
```bash
cls
subscriber_quic.exe
```
Ingresa: `Brasil vs Uruguay`

#### Enviar Mensajes
**Terminal 3 (Publisher):**
```bash
cls
publisher_quic.exe
```

Envía:
```
Colombia vs Argentina:Mensaje A
Brasil vs Uruguay:Mensaje B
Colombia vs Argentina:Mensaje C
```

#### Resultado Esperado

**Subscriber 1 (Colombia) verá:**
```
[RX] [Colombia vs Argentina] seq=1: Mensaje A
[RX] [Colombia vs Argentina] seq=2: Mensaje C
```

**Subscriber 2 (Brasil) verá:**
```
[RX] [Brasil vs Uruguay] seq=1: Mensaje B
```

✅ Cada subscriber solo recibe SU tema  
✅ Filtrado funciona correctamente  
✅ **Secuencias independientes:** Colombia tiene seq=1,2,3... y Brasil tiene seq=1,2,3... (cada uno con su propia secuencia)

**Nota importante:** El subscriber **ignora silenciosamente** los mensajes de temas a los que NO está suscrito. No solicita retransmisión de paquetes que no le corresponden. Cada tema tiene su propia secuencia independiente, empezando desde 1.

---

### 📋 Escenario 4: Pérdida de Paquetes con Retransmisión ⭐

**Objetivo:** Probar la retransmisión automática cuando se detiene temporalmente el broker

#### Importante: Limitaciones de la Implementación

⚠️ **El subscriber solo recibe mensajes desde que se suscribe**  
⚠️ **Al reiniciar el subscriber, pierde la memoria de su último seq**  
⚠️ **En localhost, UDP casi nunca pierde paquetes naturalmente**

Por esto, el **único método efectivo** es detener el broker temporalmente.

---

#### Método: Detener Broker Temporalmente

##### Paso 1: Sistema Completo Activo
```bash
# Terminal 1
cls
broker_quic.exe

# Terminal 2
cls
subscriber_quic.exe
# Ingresa: Colombia vs Argentina

# Terminal 3
cls
publisher_quic.exe
```

##### Paso 2: Enviar Mensaje Inicial
En publisher:
```
Colombia vs Argentina:Mensaje 1
```

**Subscriber verá:**
```
[RX] [Colombia vs Argentina] seq=1: Mensaje 1
```

##### Paso 3: Detener Broker
En Terminal 1 (broker), presiona **Ctrl+C** para detenerlo.

##### Paso 4: Enviar Mensajes (se perderán)
En publisher, intenta enviar:
```
Colombia vs Argentina:Mensaje 2
Colombia vs Argentina:Mensaje 3
Colombia vs Argentina:Mensaje 4
```

**Publisher mostrará:**
```
[->] Enviando seq=2...
[!] Timeout - no se recibió ACK

[->] Enviando seq=3...
[!] Timeout - no se recibió ACK

[->] Enviando seq=4...
[!] Timeout - no se recibió ACK
```

Estos mensajes **NO llegan al broker** y se pierden permanentemente.

##### Paso 5: Reiniciar Broker
En Terminal 1:
```bash
broker_quic.exe
```

##### Paso 6: Enviar Nuevo Mensaje
En publisher:
```
Colombia vs Argentina:Mensaje 5
```

##### Paso 7: Observar Comportamiento

**Subscriber mostrará:**
```
[RX] [Colombia vs Argentina] seq=5: Mensaje 5

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=2
[!] No se pudo recuperar seq=2 de 'Colombia vs Argentina'

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=3
[!] No se pudo recuperar seq=3 de 'Colombia vs Argentina'

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=4
[!] No se pudo recuperar seq=4 de 'Colombia vs Argentina'
```

#### Resultado Esperado

✅ **Subscriber detecta pérdida** - Salta de seq=1 a seq=5  
✅ **Subscriber solicita retransmisión** - Pide seq=2, 3, 4  
⚠️ **Broker no puede retransmitir** - Nunca recibió esos mensajes  
✅ **Sistema continúa funcionando** - Procesa seq=5 correctamente  

**Conclusión:** El sistema de retransmisión **funciona correctamente**, pero solo puede recuperar mensajes que el broker haya recibido y guardado en su historial  

---

### 📋 Escenario 5: Retransmisión Exitosa con Broker en Ejecución ⭐⭐

**Objetivo:** Demostrar retransmisión exitosa cuando el broker SÍ tiene los mensajes en su historial

#### Preparación: Entender el Historial del Broker

El broker guarda los últimos **100 mensajes** en memoria. Puede retransmitir cualquier mensaje que haya recibido y procesado.

#### Método: Usar Tema Diferente para Crear "Saltos"

##### Paso 1: Sistema Completo Activo
```bash
# Terminal 1
cls
broker_quic.exe

# Terminal 2
cls
subscriber_quic.exe
# Ingresa SOLO: Colombia vs Argentina

# Terminal 3
cls
publisher_quic.exe
```

##### Paso 2: Enviar Mensaje Inicial
```
Colombia vs Argentina:Mensaje 1
```

**Subscriber verá:**
```
[RX] [Colombia vs Argentina] seq=1: Mensaje 1
```

##### Paso 3: Cerrar Subscriber Temporalmente
En Terminal 2, presiona **Ctrl+C**

##### Paso 4: Enviar Más Mensajes (broker los guarda)
En publisher:
```
Colombia vs Argentina:Mensaje 2
Colombia vs Argentina:Mensaje 3
Colombia vs Argentina:Mensaje 4
```

**Publisher mostrará:**
```
[!] Timeout - no se recibió ACK
```

**Pero el broker SÍ recibió y guardó los mensajes en su historial.**

##### Paso 5: Reiniciar Subscriber
En Terminal 2:
```bash
subscriber_quic.exe
# Ingresa: Colombia vs Argentina
```

##### Paso 6: Enviar Nuevo Mensaje
En publisher:
```
Colombia vs Argentina:Mensaje 5
```

##### Paso 7: Observar Retransmisión Exitosa

**Subscriber mostrará:**
```
[RX] [Colombia vs Argentina] seq=5: Mensaje 5

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=2
[<-] RETRANSMITIDO [Colombia vs Argentina] seq=2: Mensaje 2

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=3
[<-] RETRANSMITIDO [Colombia vs Argentina] seq=3: Mensaje 3

[!] Pérdida detectada en 'Colombia vs Argentina' - solicitando retransmisión de seq=4
[<-] RETRANSMITIDO [Colombia vs Argentina] seq=4: Mensaje 4
```

#### Resultado Esperado

✅ **Subscriber detecta pérdida** - Detecta salto de seq=1 a seq=5  
✅ **Subscriber solicita retransmisión** - Pide seq=2, 3, 4 individualmente  
✅ **Broker retransmite exitosamente** - Encuentra mensajes en historial  
✅ **Recuperación completa** - Subscriber obtiene todos los mensajes  
✅ **Secuencia final:** 1, 2, 3, 4, 5 (completa)

**Nota importante:** El subscriber pierde su memoria de `ultimo_seq` al reiniciarse, pero como el broker SÍ recibió los mensajes 2, 3, 4 (están en su historial), puede retransmitirlos exitosamente.

---

## Limitaciones de la Implementación Actual

⚠️ **Subscriber sin memoria persistente:**  
- Al cerrar y reiniciar, el subscriber olvida su `ultimo_seq`
- No puede retomar desde donde quedó
- Siempre comienza con `ultimo_seq = 0`

⚠️ **Broker no envía historial automáticamente:**  
- El broker NO envía mensajes antiguos cuando un subscriber se suscribe
- Solo envía mensajes NUEVOS que lleguen después de la suscripción
- El subscriber debe detectar "saltos" para solicitar retransmisión

⚠️ **Retransmisión reactiva, no proactiva:**  
- El subscriber solo pide retransmisión cuando detecta un salto en `seq`
- Si se suscribe después de perder mensajes, NO los solicita (porque no sabe que existen)

✅ **Lo que SÍ funciona:**  
- Detección de pérdida cuando hay saltos de secuencia
- Solicitud automática de retransmisión
- Broker retransmite si tiene el mensaje en historial
- Secuencias independientes por tema
- Filtrado correcto por tema
- Verificación de tema en retransmisión

  

---

## Formato de Mensajes

### Para Publisher

**Formato obligatorio:**
```
Equipo A vs Equipo B:contenido del mensaje
```

**Ejemplos correctos:**
```
Colombia vs Argentina:Gol al minuto 45
Real Madrid vs Barcelona:Tarjeta amarilla
Brasil vs Uruguay:Inicio del segundo tiempo
```

**Ejemplos incorrectos:**
```
Colombia vs Argentina Gol           ❌ (falta :)
Colombia-Argentina:Gol              ❌ (debe usar "vs")
ColombiavsArgentina:Gol             ❌ (faltan espacios)
```

### Para Subscriber

**Un solo tema:**
```
Colombia vs Argentina
```

**Múltiples temas (separados por comas):**
```
Colombia vs Argentina, Brasil vs Uruguay, Real Madrid vs Barcelona
```

---

## Tipos de Paquetes

| Tipo | Nombre | Dirección | Descripción |
|------|--------|-----------|-------------|
| 'S' | Suscripción | Subscriber → Broker | Suscribirse a tema(s) |
| 'P' | Publicación | Broker → Subscriber | Enviar mensaje |
| 'A' | ACK | Bidireccional | Confirmar recepción |
| 'R' | Retransmisión | Subscriber → Broker | Solicitar paquete perdido |

---

## Símbolos en los Mensajes

| Símbolo | Significado | Descripción |
|---------|-------------|-------------|
| **[RX]** | **Receive** (Recibir) | Mensaje recibido normalmente |
| **[TX]** | **Transmit** (Transmitir) | Mensaje enviado (en publisher) |
| **[->]** | Envío | Indica que se está enviando un mensaje |
| **[<-]** | Recepción | Indica que se recibió una respuesta |
| **[!]** | Alerta | Indica pérdida de paquete o error |

**Ejemplo de lectura:**
```
[RX] [Colombia vs Argentina] seq=1: Gol
```
- **[RX]** = Recibiendo mensaje
- **[Colombia vs Argentina]** = Tema/Partido
- **seq=1** = Número de secuencia
- **Gol** = Contenido del mensaje

---

## Flujo de Retransmisión

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Publisher  │         │   Broker    │         │ Subscriber  │
└──────┬──────┘         └──────┬──────┘         └──────┬──────┘
       │                       │                        │
       │  Mensaje 1            │  seq=1                 │
       │──────────────────────>│───────────────────────>│
       │                       │                        │
       │  Mensaje 2            │  seq=2 (PERDIDO) ❌     │
       │──────────────────────>│  X──────────────────   │
       │                       │ (guardado en historial)│
       │  Mensaje 3            │  seq=3                 │
       │──────────────────────>│───────────────────────>│
       │                       │                        │
       │                       │  Solicitud 'R' seq=2   │
       │                       │<───────────────────────│
       │                       │                        │
       │                       │  RETRANS seq=2 ✅       │
       │                       │───────────────────────>│
```

---

## Solución de Problemas

### Error: "gcc no se reconoce como comando"
**Solución:** Instalar MinGW y agregarlo al PATH de Windows.

### Error: "No se puede enlazar al puerto 7000"
**Solución:** El broker ya está ejecutándose. Detener:
```bash
taskkill /F /IM broker_quic.exe
```

### Error: "Permission denied" al compilar
**Solución:** Detener el programa antes de compilar:
```bash
taskkill /F /IM broker_quic.exe
taskkill /F /IM subscriber_quic.exe
taskkill /F /IM publisher_quic.exe
```

### Subscriber no recibe mensajes
**Verificar:**
1. Broker está activo
2. Tema coincide **exactamente** (mayúsculas, espacios)
3. Formato correcto: `Equipo A vs Equipo B:mensaje`

### Retransmisión no funciona
**Verificar:**
1. Mensaje perdido está en el historial del broker (últimos 100 mensajes)
2. Broker se reinició correctamente
3. Subscriber está suscrito al tema correcto

---

## Limpieza de Terminal

Para limpiar la terminal sin cerrarla:

```bash
cls
```

O usar atajo de teclado: **`Ctrl + L`**

Para limpiar y ejecutar:
```bash
cls; broker_quic.exe
cls; subscriber_quic.exe
cls; publisher_quic.exe
```

---

## Comandos Útiles

### Compilar Todo
```bash
cd "C:\Users\57300\OneDrive - Universidad de los Andes\Documentos\Andes\Noveno Semestre\Infracom\Laboratorio3\Lab3-Redes"
gcc src/broker_quic.c -o broker_quic.exe -lws2_32; gcc src/publisher_quic.c -o publisher_quic.exe -lws2_32; gcc src/subscriber_quic.c -o subscriber_quic.exe -lws2_32
```

### Detener Todos los Procesos
```bash
taskkill /F /IM broker_quic.exe; taskkill /F /IM subscriber_quic.exe; taskkill /F /IM publisher_quic.exe
```

### Compilar y Ejecutar Broker
```bash
cd "C:\Users\57300\OneDrive - Universidad de los Andes\Documentos\Andes\Noveno Semestre\Infracom\Laboratorio3\Lab3-Redes"
cls; taskkill /F /IM broker_quic.exe 2>$null; gcc src/broker_quic.c -o broker_quic.exe -lws2_32; broker_quic.exe
```

---

## Comparación: QUIC vs TCP vs UDP

| Característica | TCP | UDP | QUIC |
|----------------|-----|-----|------|
| Handshake | Sí (3-way) | No | No |
| Velocidad inicial | Lenta | Rápida | Rápida |
| Confiabilidad | Sí | No | Sí |
| Números de secuencia | Sí | No | Sí |
| ACKs | Sí | No | Sí |
| Retransmisión | Automática | No | Automática |
| Detección de pérdida | Sí | No | Sí |
| Overhead | Alto | Bajo | Medio |

**Conclusión:** QUIC combina lo mejor de TCP (confiabilidad) y UDP (velocidad).

---

## Resumen de Pruebas

| Escenario | Objetivo | Método | Estado |
|-----------|----------|--------|--------|
| 1. Mensajes Básicos | Funcionamiento normal | Envío continuo | ✅ |
| 2. Suscripción Múltiple | Múltiples temas | Varios partidos | ✅ |
| 3. Filtrado por Tema | Solo recibe suscritos | 2 subscribers | ✅ |
| 4. Detección de Pérdida | Identificar saltos de seq | Detener broker | ✅ |
| 5. Retransmisión Exitosa | Recuperar mensajes | Cerrar subscriber | ✅ |

### Notas sobre Pruebas de Retransmisión

**✅ Escenario 4** demuestra:
- Detección correcta de pérdida de paquetes
- Solicitud automática de retransmisión
- Comportamiento cuando el broker no tiene los mensajes

**✅ Escenario 5** demuestra:
- Retransmisión exitosa desde el historial del broker
- Recuperación de múltiples mensajes perdidos
- Sistema completo de confiabilidad funcionando

---

## Características QUIC Implementadas

✅ **Transporte UDP** - Sin conexión, rápido  
✅ **Números de Secuencia** - Ordenamiento (1, 2, 3...)  
✅ **ACKs** - Confirmación bidireccional  
✅ **Detección de Pérdida** - Comparación de seq por tema  
✅ **Retransmisión Automática** - Recupera paquetes perdidos  
✅ **Historial de Mensajes** - Buffer circular de 100 mensajes  
✅ **Suscripción Múltiple** - Varios temas simultáneos  
✅ **Filtrado por Tema** - Recibe solo suscritos  
✅ **Secuencias Independientes** - Cada tema tiene su propia secuencia  
✅ **Visualización de Tema** - Muestra el partido en cada mensaje  

---

## Archivos del Proyecto

```
src/
├── broker_quic.c      - Servidor central con historial
├── publisher_quic.c   - Cliente publicador
└── subscriber_quic.c  - Cliente suscriptor con retransmisión

broker_quic.exe        - Ejecutable del broker
publisher_quic.exe     - Ejecutable del publisher
subscriber_quic.exe    - Ejecutable del subscriber
README_QUIC.md         - Esta documentación
```

---

**Sistema desarrollado como implementación educativa del protocolo QUIC**  
**Fecha:** Octubre 2025  
**Versión:** 2.0 (con retransmisión automática)
