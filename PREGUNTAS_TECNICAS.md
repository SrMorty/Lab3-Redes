# Preguntas Técnicas - Implementación QUIC vs TCP vs UDP

## 📚 Preparación para Defensa del Laboratorio

Este documento contiene preguntas técnicas clave sobre la implementación QUIC y sus diferencias con TCP y UDP del Lab3.

---

## Sección 1: Conceptos Fundamentales

### P1: ¿Qué es QUIC y por qué es un protocolo híbrido?

**Respuesta:**
QUIC (Quick UDP Internet Connections) es un protocolo híbrido porque combina:
- **Base UDP:** Utiliza UDP como transporte, lo que elimina el handshake de 3 vías de TCP
- **Características TCP:** Implementa confiabilidad mediante números de secuencia, ACKs y retransmisión automática

**Ventaja principal:** Velocidad de UDP + Confiabilidad de TCP

---

### P2: ¿Cuál es la diferencia principal entre tu implementación QUIC y las implementaciones TCP/UDP del laboratorio?

**Respuesta:**

| Característica | TCP | UDP | QUIC (Mi implementación) |
|----------------|-----|-----|--------------------------|
| **Transporte** | TCP con handshake | UDP sin conexión | UDP sin handshake |
| **Confiabilidad** | Automática por SO | Ninguna | Manual con ACKs |
| **Secuencias** | Automática por SO | No tiene | Manual por tema |
| **Retransmisión** | Automática por SO | No tiene | Manual desde historial |
| **Velocidad inicial** | Lenta (3-way handshake) | Rápida | Rápida |
| **Complejidad código** | Simple (SO maneja todo) | Muy simple | Alta (todo manual) |

**Conclusión:** QUIC requiere MÁS código pero ofrece un balance entre velocidad y confiabilidad.

---

## Sección 2: Implementación Técnica

### P3: ¿Cómo funciona el sistema de números de secuencia en tu implementación?

**Respuesta:**

**Secuencias INDEPENDIENTES por tema:**
```c
// Estructura en el broker
typedef struct {
    char tema[50];
    unsigned int seq;  // Secuencia independiente por tema
} SecuenciaTema;

SecuenciaTema secuencias_tema[50];  // Array de secuencias
```

**Ejemplo:**
- "Colombia vs Argentina" → seq = 1, 2, 3, 4...
- "Brasil vs Uruguay" → seq = 1, 2, 3, 4...
- Cada tema tiene su propia secuencia desde 1

**¿Por qué es importante?**
- Evita falsos positivos de pérdida de paquetes
- Permite suscripción múltiple sin confusión
- Cada subscriber rastrea la secuencia de SUS temas

---

### P4: ¿Cómo detecta el subscriber que se perdió un paquete?

**Respuesta:**

**Código clave en subscriber:**
```c
// Estructura para rastrear secuencias por tema
typedef struct {
    char tema[50];
    unsigned int ultimo_seq;
} SecuenciaTema;

SecuenciaTema temas_suscritos[10];

// Detección de pérdida
if (ultimo_seq_tema > 0 && pkt.seq != ultimo_seq_tema + 1) {
    // ¡Pérdida detectada!
    // Solicitar retransmisión de paquetes faltantes
    for (unsigned int s = ultimo_seq_tema + 1; s < pkt.seq; s++) {
        // Enviar solicitud de retransmisión tipo 'R'
    }
}
```

**Ejemplo visual:**
```
Recibe: seq=1 → OK (ultimo_seq = 1)
Recibe: seq=5 → ¡ALERTA! Esperaba seq=2
         ↓
Solicita retransmisión de: 2, 3, 4
```

---

### P5: ¿Cómo funciona el historial del broker para retransmisión?

**Respuesta:**

**Estructura del historial:**
```c
typedef struct {
    unsigned int seq;
    char tema[50];
    char mensaje[500];
} MensajeHistorial;

MensajeHistorial historial[100];  // Buffer circular
int idx_historial = 0;
```

**Funcionamiento:**
1. **Guardar mensaje:** Cada mensaje publicado se guarda en `historial[]`
2. **Buffer circular:** Cuando llega a 100, vuelve a 0 (sobrescribe antiguos)
3. **Retransmisión:** Cuando subscriber pide seq=X, broker busca en historial
4. **Verificación tema:** Solo retransmite si el tema coincide

**Código de retransmisión:**
```c
// Buscar en historial
for (int i = 0; i < 100; i++) {
    if (historial[i].seq == seq_solicitado) {
        char tema_retrans[50];
        // Extraer tema del mensaje guardado
        
        // VERIFICAR que el tema coincida
        if (strcmp(tema_retrans, tema_solicitado) == 0) {
            // Retransmitir
        }
    }
}
```

---

### P6: ¿Por qué usas ACKs si UDP no los tiene?

**Respuesta:**

**UDP no tiene ACKs nativos**, por eso los implemento manualmente:

**En Publisher:**
```c
// Enviar mensaje
sendto(sock, &pkt, sizeof(pkt), 0, ...);

// ESPERAR ACK con timeout de 2000ms
struct timeval timeout = {2, 0};
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

int bytes = recvfrom(sock, &ack_pkt, sizeof(ack_pkt), 0, ...);
if (bytes > 0 && ack_pkt.tipo == 'A') {
    printf("[<-] ACK recibido\n");
} else {
    printf("[!] Timeout - no se recibió ACK\n");
}
```

**En Broker:**
```c
// Recibir mensaje
recvfrom(sock, &pkt, sizeof(pkt), 0, ...);

// ENVIAR ACK inmediatamente
Paquete ack;
ack.tipo = 'A';
strcpy(ack.mensaje, "OK");
sendto(sock, &ack, sizeof(ack), 0, ...);
```

**Ventaja:** Confirma que el broker recibió el mensaje (similar a TCP pero manual).

---

## Sección 3: Comparación con TCP y UDP

### P7: ¿Por qué tu implementación QUIC es más compleja que TCP?

**Respuesta:**

**TCP hace TODO automáticamente:**
- Sistema operativo maneja secuencias
- Retransmisión automática
- Control de flujo
- Control de congestión
- Solo necesitas `send()` y `recv()`

**QUIC requiere implementación manual:**
```c
// En TCP (simple):
send(sock, mensaje, len, 0);    // El SO hace todo

// En QUIC (complejo):
1. Asignar número de secuencia
2. Enviar paquete con seq
3. Esperar ACK con timeout
4. Manejar retransmisión si no hay ACK
5. Guardar en historial para retransmisión futura
6. Verificar tema en retransmisión
```

**¿Vale la pena?** Sí, porque:
- Más rápido que TCP (sin handshake)
- Más confiable que UDP (con retransmisión)
- Control total sobre el protocolo

---

### P8: ¿Cuál es la ventaja de QUIC sobre UDP básico del laboratorio?

**Respuesta:**

**UDP básico (Lab3):**
```c
// Publisher
sendto(sock, mensaje, len, 0, ...);  // Enviar y olvidar

// Subscriber
recvfrom(sock, buffer, size, 0, ...);  // Recibir sin garantías
```

**Problemas UDP básico:**
- ❌ No sabe si el mensaje llegó
- ❌ No detecta pérdida de paquetes
- ❌ No puede recuperar mensajes perdidos
- ❌ Sin orden garantizado

**QUIC (Mi implementación):**
```c
// Publisher
sendto(...);  // Enviar
recvfrom(...);  // Esperar ACK
// Si no hay ACK → Reintentar

// Subscriber
if (seq != esperado) {
    // Detectar pérdida
    solicitar_retransmision();
}
```

**Ventajas QUIC:**
- ✅ Confirma recepción con ACKs
- ✅ Detecta pérdida de paquetes
- ✅ Recupera mensajes perdidos
- ✅ Orden garantizado por tema

---

### P9: Si TCP ya tiene retransmisión, ¿por qué implementar QUIC?

**Respuesta:**

**Problema de TCP:**
```
Cliente → SYN → Servidor
Cliente ← SYN-ACK ← Servidor
Cliente → ACK → Servidor
Cliente → Datos → Servidor  ← Finalmente envía datos
```
**3 paquetes de overhead ANTES de enviar datos** (150ms - 300ms en redes reales)

**Ventaja de QUIC:**
```
Cliente → Datos (seq=1) → Servidor
Cliente ← ACK ← Servidor
```
**0 paquetes de handshake** → Envío inmediato

**Ejemplo real:**
- TCP: 150ms handshake + 50ms datos = **200ms total**
- QUIC: 0ms handshake + 50ms datos = **50ms total**
- **4x más rápido para el primer mensaje**

---

## Sección 4: Problemas y Limitaciones

### P10: ¿Cuáles son las limitaciones de tu implementación QUIC?

**Respuesta:**

**Limitación 1: Subscriber sin memoria persistente**
```c
// Al cerrar subscriber
SecuenciaTema temas[10];  // Se pierde al cerrar

// Al reiniciar
SecuenciaTema temas[10];  // Comienza desde 0 de nuevo
```
**Consecuencia:** No puede retomar desde donde quedó

---

**Limitación 2: Broker no envía historial completo**
- Broker NO envía mensajes antiguos al suscribirse
- Solo envía mensajes NUEVOS después de la suscripción
- Subscriber debe "descubrir" pérdidas al recibir mensaje nuevo

**Ejemplo:**
```
1. Broker tiene mensajes 1, 2, 3, 4 en historial
2. Subscriber se suscribe
3. Broker NO envía 1, 2, 3, 4 automáticamente
4. Llega mensaje 5
5. Subscriber recibe 5 → Detecta "salto" de 0 a 5 → Pide 1, 2, 3, 4
```

---

**Limitación 3: Retransmisión reactiva**
- Subscriber solo pide retransmisión cuando detecta salto en `seq`
- Si se suscribe DESPUÉS de perder mensajes, no los solicita

**Ejemplo problema:**
```
Mensajes 1, 2, 3 enviados SIN subscriber activo
Subscriber se suscribe
Mensaje 4 enviado
Subscriber recibe seq=4 → Piensa que es el primero (ultimo_seq=0)
NO solicita 1, 2, 3 (no sabe que existen)
```

---

**Limitación 4: Sin cifrado**
- Mensajes en texto plano
- QUIC real usa TLS 1.3
- Mi implementación es educativa, no segura

---

**Limitación 5: Buffer limitado**
- Historial de solo 100 mensajes
- Mensajes antiguos se sobrescriben
- No hay persistencia en disco

---

### P11: ¿Qué pasa si dos publishers envían al mismo tiempo?

**Respuesta:**

**Problema potencial - Race condition:**
```c
// Publisher 1 envía "Colombia:Gol"
seq = obtener_siguiente_seq("Colombia");  // seq = 5

// Publisher 2 envía "Colombia:Tarjeta" (al mismo tiempo)
seq = obtener_siguiente_seq("Colombia");  // seq = 5 (mismo!)
```

**Mi implementación:**
- Broker es de un solo hilo (single-threaded)
- Procesa mensajes secuencialmente con `recvfrom()` bloqueante
- No hay race condition porque procesa uno a la vez

**Limitación:** Un solo publisher debe enviar a la vez por tema
- En producción se usarían mutexes o atomic operations

---

### P12: ¿Cómo pruebas la retransmisión si UDP en localhost casi nunca pierde paquetes?

**Respuesta:**

**Métodos de prueba implementados:**

**Método 1: Detener broker temporalmente**
```bash
1. Subscriber activo con seq=1
2. Detener broker (Ctrl+C)
3. Publisher envía seq=2, 3, 4 → Se pierden
4. Reiniciar broker
5. Publisher envía seq=5
6. Subscriber detecta salto 1→5 → Solicita 2, 3, 4
7. Broker no tiene 2, 3, 4 → No puede retransmitir
```
**Demuestra:** Detección de pérdida y solicitud de retransmisión

---

**Método 2: Cerrar subscriber temporalmente**
```bash
1. Subscriber activo con seq=1
2. Cerrar subscriber (Ctrl+C)
3. Publisher envía seq=2, 3, 4 → Broker los guarda
4. Reiniciar subscriber
5. Publisher envía seq=5
6. Subscriber recibe seq=5 → Solicita 2, 3, 4
7. Broker SÍ tiene 2, 3, 4 → Retransmite exitosamente
```
**Demuestra:** Retransmisión exitosa desde historial

---

**Nota importante:** En producción se usaría:
- Simulación de pérdida aleatoria (`rand() % 100 < 20` para 20% de pérdida)
- Herramientas como `tc` (Traffic Control) en Linux
- Wireshark con filtros para descartar paquetes

---

## Sección 5: Preguntas de Diseño

### P13: ¿Por qué usas secuencias independientes por tema y no una secuencia global?

**Respuesta:**

**Secuencia global (PROBLEMA):**
```
seq=1 → "Colombia:Gol"
seq=2 → "Brasil:Gol"      ← Subscriber solo suscrito a Colombia NO lo ve
seq=3 → "Colombia:Tarjeta"

Subscriber de Colombia:
Recibe seq=1 ✅
Recibe seq=3 ❌ ¡Detecta "pérdida" de seq=2!
Pide retransmisión de seq=2
Broker envía "Brasil:Gol"
Subscriber rechaza (tema incorrecto)
```

**Falso positivo de pérdida** → Solicitud innecesaria

---

**Secuencia independiente (SOLUCIÓN):**
```
Colombia: seq=1, 2, 3...
Brasil: seq=1, 2, 3...

seq_colombia=1 → "Colombia:Gol"
seq_brasil=1 → "Brasil:Gol"      ← Ignorado por subscriber de Colombia
seq_colombia=2 → "Colombia:Tarjeta"

Subscriber de Colombia:
Recibe seq=1 de Colombia ✅
Recibe seq=2 de Colombia ✅
Sin falsos positivos ✅
```

**Código clave:**
```c
// En broker
unsigned int obtener_siguiente_seq(char *tema) {
    for (int i = 0; i < num_temas_seq; i++) {
        if (strcmp(secuencias_tema[i].tema, tema) == 0) {
            return ++secuencias_tema[i].seq;  // Incrementar seq de ESTE tema
        }
    }
    // Tema nuevo → comenzar en 1
    strcpy(secuencias_tema[num_temas_seq].tema, tema);
    secuencias_tema[num_temas_seq].seq = 1;
    num_temas_seq++;
    return 1;
}
```

---

### P14: ¿Por qué el formato de mensaje es "tema:mensaje" y no solo "mensaje"?

**Respuesta:**

**Necesidad de incluir tema en CADA paquete:**

**Razón 1: Retransmisión con verificación**
```c
// Subscriber solicita retransmisión
pkt.tipo = 'R';
pkt.seq = 5;
sprintf(pkt.mensaje, "Colombia vs Argentina");  // Incluye tema solicitado

// Broker verifica
if (strcmp(tema_en_historial, tema_solicitado) == 0) {
    // Solo retransmite si coincide
}
```
Sin tema en mensaje, el broker podría retransmitir mensaje de tema incorrecto.

---

**Razón 2: Subscriber conoce el tema de CADA paquete**
```c
// Recibir paquete
char *separador = strchr(pkt.mensaje, ':');
*separador = '\0';
char *tema_msg = pkt.mensaje;         // "Colombia vs Argentina"
char *contenido = separador + 1;      // "Gol al minuto 45"

// Verificar si está suscrito
for (int i = 0; i < num_temas; i++) {
    if (strcmp(temas_suscritos[i].tema, tema_msg) == 0) {
        // Procesar mensaje
    }
}
```
Sin tema, no podría filtrar mensajes de temas no suscritos.

---

**Razón 3: Display claro**
```
[RX] [Colombia vs Argentina] seq=1: Gol al minuto 45
     └─ Tema entre corchetes          └─ Contenido
```
Usuario sabe de qué partido es cada mensaje.

---

### P15: ¿Qué harías diferente en una versión 2.0 de tu implementación?

**Respuesta:**

**Mejora 1: Memoria persistente en subscriber**
```c
// Guardar estado en archivo
void guardar_estado() {
    FILE *f = fopen("subscriber_state.dat", "wb");
    fwrite(temas_suscritos, sizeof(SecuenciaTema), num_temas, f);
    fclose(f);
}

// Cargar estado al iniciar
void cargar_estado() {
    FILE *f = fopen("subscriber_state.dat", "rb");
    if (f) {
        fread(temas_suscritos, sizeof(SecuenciaTema), 10, f);
        fclose(f);
    }
}
```
**Beneficio:** Subscriber recuerda su último seq al reiniciar

---

**Mejora 2: Historial persistente en broker**
```c
// Guardar historial en disco
void guardar_historial() {
    FILE *f = fopen("broker_historial.dat", "wb");
    fwrite(historial, sizeof(MensajeHistorial), 100, f);
    fclose(f);
}
```
**Beneficio:** Broker no pierde historial al reiniciar

---

**Mejora 3: Envío proactivo de historial**
```c
// Cuando subscriber se suscribe
void enviar_historial_tema(char *tema, struct sockaddr_in *cliente) {
    for (int i = 0; i < 100; i++) {
        if (strcmp(historial[i].tema, tema) == 0) {
            // Enviar todos los mensajes de este tema
            sendto(sock, &historial[i], sizeof(Paquete), 0, cliente, ...);
        }
    }
}
```
**Beneficio:** Subscriber recibe mensajes perdidos automáticamente

---

**Mejora 4: Control de congestión**
```c
// Limitar tasa de envío
void rate_limit() {
    static time_t last_send = 0;
    time_t now = time(NULL);
    if (now - last_send < 1) {  // Max 1 mensaje/segundo
        sleep(1);
    }
    last_send = now;
}
```
**Beneficio:** No saturar la red

---

**Mejora 5: Cifrado (TLS)**
```c
// Usar OpenSSL para cifrar mensajes
SSL_CTX *ctx = SSL_CTX_new(TLS_method());
// Cifrar paquetes antes de enviar
```
**Beneficio:** Seguridad en mensajes

---

**Mejora 6: Múltiples hilos en broker**
```c
// Un hilo por cliente
void *manejar_cliente(void *arg) {
    // Procesar mensajes de este cliente
}

pthread_create(&thread, NULL, manejar_cliente, cliente);
```
**Beneficio:** Manejar múltiples publishers simultáneamente

---

## Resumen de Diferencias Clave

| Aspecto | TCP (Lab3) | UDP (Lab3) | QUIC (Mi implementación) |
|---------|------------|------------|--------------------------|
| **Handshake** | 3-way (lento) | No (rápido) | No (rápido) |
| **Confiabilidad** | SO automática | Ninguna | Manual implementada |
| **Secuencias** | SO automática | No tiene | Manual por tema |
| **ACKs** | SO automática | No tiene | Manual con timeout |
| **Retransmisión** | SO automática | No tiene | Manual desde historial |
| **Orden** | Garantizado por SO | No garantizado | Garantizado por tema |
| **Complejidad** | Baja | Muy baja | Alta |
| **Velocidad inicial** | Lenta | Rápida | Rápida |
| **Latencia** | Alta (handshake) | Baja | Baja |
| **Código líneas** | ~150 | ~100 | ~750 (3 archivos) |

---

## Puntos Clave para la Defensa

### ✅ Qué SÍ funciona y puedes demostrar:
1. **Secuencias independientes por tema** - Evita falsos positivos
2. **Detección de pérdida** - Subscriber detecta saltos en seq
3. **Solicitud de retransmisión** - Subscriber pide paquetes faltantes
4. **Retransmisión desde historial** - Broker reenvía si tiene el mensaje
5. **Filtrado por tema** - Subscriber solo procesa sus temas
6. **ACKs manuales** - Publisher confirma recepción
7. **Múltiples suscripciones** - Un subscriber puede seguir varios temas

### ⚠️ Limitaciones que debes reconocer:
1. Subscriber pierde memoria al reiniciar
2. Broker no envía historial automáticamente
3. Retransmisión reactiva (no proactiva)
4. Sin cifrado (texto plano)
5. Historial limitado a 100 mensajes
6. Sin control de congestión
7. Sin soporte multi-hilo en broker

### 🎯 Mensaje Final:
**"Mi implementación QUIC demuestra los principios fundamentales de un protocolo híbrido: velocidad de UDP con confiabilidad manual similar a TCP, enfocándose en secuencias independientes por tema para soportar pub/sub de múltiples tópicos sin falsos positivos de pérdida."**

---

**Documento preparado para defensa técnica del Laboratorio 3 - Protocolos de Red**  
**Fecha:** Octubre 2025
