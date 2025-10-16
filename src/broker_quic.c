/*
 * ============================================================================
 * BROKER QUIC - Sistema Pub/Sub con Protocolo Híbrido
 * ============================================================================
 * 
 * Descripción:
 * Este broker implementa un protocolo QUIC simplificado que combina:
 *   - UDP: Transporte rápido sin handshake de conexión (sin overhead de TCP)
 *   - TCP: Confiabilidad mediante números de secuencia, ACKs y retransmisión
 * 
 * Arquitectura:
 * El broker actúa como intermediario central en un patrón Publisher-Subscriber:
 *   1. Publishers envían mensajes con formato "TEMA:contenido"
 *   2. Subscribers se suscriben a temas específicos
 *   3. Broker distribuye mensajes solo a subscribers del tema correspondiente
 * 
 * Características Clave:
 *   ✓ Secuencias independientes por tema (evita falsos positivos de pérdida)
 *   ✓ Historial de 100 mensajes para retransmisión
 *   ✓ ACKs manuales para confirmar recepción (simulando TCP sobre UDP)
 *   ✓ Verificación de tema en retransmisión (evita enviar datos incorrectos)
 *   ✓ Soporte para múltiples suscriptores por tema
 * 
 * Limitaciones:
 *   - Historial limitado a 100 mensajes (buffer circular)
 *   - Sin persistencia (se pierde al reiniciar)
 *   - Sin cifrado (mensajes en texto plano)
 *   - Single-threaded (procesa un mensaje a la vez)
 * 
 * Autor: Lab3 - Infraestructura de Comunicaciones
 * Fecha: Octubre 2025
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

// ============================================================================
// CONSTANTES DE CONFIGURACIÓN
// ============================================================================

#define PUERTO 7000              // Puerto UDP donde escucha el broker
#define MAX_SUBS 100             // Máximo número de suscriptores simultáneos
#define BUFFER_SIZE 512          // Tamaño del buffer de recepción
#define MAX_HISTORIAL 100        // Máximo de mensajes guardados para retransmisión

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * Paquete - Unidad básica de comunicación QUIC
 * 
 * Esta estructura define el formato de TODOS los paquetes intercambiados
 * entre broker, publishers y subscribers. Similar a un "datagrama QUIC".
 * 
 * Campos:
 *   - seq: Número de secuencia para ordenar mensajes y detectar pérdidas
 *          (independiente por tema en el broker)
 *   - tipo: Indica el propósito del paquete:
 *           'S' = Suscripción (subscriber → broker)
 *           'P' = Publicación (publisher → broker → subscriber)
 *           'A' = ACK (confirmación bidireccional)
 *           'R' = Retransmisión (solicitud de reenvío)
 *   - mensaje: Payload del paquete, uso depende del tipo:
 *           'S': nombre del tema a suscribir
 *           'P': formato "tema:contenido"
 *           'A': "OK"
 *           'R': nombre del tema del mensaje perdido
 */
typedef struct {
    unsigned int seq;      
    char tipo;             
    char mensaje[500];     
} Paquete;

/**
 * Suscriptor - Registro de un subscriber conectado
 * 
 * Almacena la información necesaria para enviar mensajes a un subscriber:
 *   - tema: El tema/canal al que está suscrito (ej: "Colombia vs Argentina")
 *   - addr: Dirección IP y puerto del subscriber para envío UDP
 * 
 * Nota: Un mismo subscriber puede aparecer múltiples veces si está
 * suscrito a varios temas diferentes.
 */
typedef struct {
    char tema[50];
    struct sockaddr_in addr;
} Suscriptor;

/**
 * MensajeHistorial - Entrada en el buffer circular de historial
 * 
 * El broker guarda los últimos 100 mensajes publicados para poder
 * retransmitirlos si un subscriber los perdió.
 * 
 * Campos:
 *   - seq: Número de secuencia del mensaje (global, no por tema)
 *   - tema: Tema al que pertenece el mensaje
 *   - mensaje: Contenido del mensaje (sin tema, solo el contenido)
 * 
 * Funcionamiento del buffer circular:
 *   - Se guardan los últimos 100 mensajes
 *   - Al llegar a 100, vuelve a sobrescribir desde el índice 0
 *   - Mensajes antiguos se pierden (limitación de memoria)
 */
typedef struct {
    unsigned int seq;
    char tema[50];
    char mensaje[500];
} MensajeHistorial;

/**
 * SecuenciaTema - Contador de secuencia independiente por tema
 * 
 * DISEÑO CRÍTICO: Cada tema tiene su propia secuencia independiente
 * 
 * ¿Por qué secuencias independientes?
 *   Sin esto, tendríamos un problema de "falsos positivos":
 *   
 *   Ejemplo con secuencia GLOBAL (PROBLEMA):
 *     seq=1 → "Colombia:Gol"
 *     seq=2 → "Brasil:Gol"    ← Subscriber de Colombia NO lo ve
 *     seq=3 → "Colombia:Tarjeta"
 *     
 *     Subscriber de Colombia recibe: seq=1, luego seq=3
 *     Detecta "pérdida" de seq=2 (falso positivo)
 *     Solicita retransmisión de seq=2
 *     Broker envía "Brasil:Gol" (tema incorrecto)
 * 
 *   Solución con secuencias INDEPENDIENTES:
 *     Colombia: seq=1 → "Gol",    seq=2 → "Tarjeta"
 *     Brasil:   seq=1 → "Gol"
 *     
 *     Subscriber de Colombia recibe: seq=1, luego seq=2
 *     No hay saltos, no hay falsos positivos ✓
 * 
 * Campos:
 *   - tema: Nombre del tema
 *   - seq: Último número de secuencia asignado para este tema
 */
typedef struct {
    char tema[50];
    unsigned int seq;
} SecuenciaTema;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

Suscriptor suscriptores[MAX_SUBS];  // Lista de todos los suscriptores activos
int num_subs = 0;                    // Contador de suscriptores actuales

MensajeHistorial historial[MAX_HISTORIAL];  // Buffer circular de mensajes
int historial_index = 0;                     // Índice actual en el buffer circular

SecuenciaTema secuencias_tema[50];  // Array de contadores de secuencia por tema
int num_temas_seq = 0;               // Cantidad de temas diferentes con secuencia

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * obtener_siguiente_seq - Obtiene el siguiente número de secuencia para un tema
 * 
 * Esta función implementa el sistema de secuencias independientes por tema.
 * 
 * Algoritmo:
 *   1. Buscar si el tema ya tiene una secuencia asignada
 *   2. Si existe: incrementar su secuencia y devolverla
 *   3. Si NO existe: crear nueva entrada con seq=1
 * 
 * Parámetros:
 *   @param tema: Nombre del tema (ej: "Colombia vs Argentina")
 * 
 * Retorna:
 *   Siguiente número de secuencia para ese tema
 * 
 * Ejemplo de uso:
 *   seq1 = obtener_siguiente_seq("Colombia vs Argentina");  // Retorna 1
 *   seq2 = obtener_siguiente_seq("Colombia vs Argentina");  // Retorna 2
 *   seq3 = obtener_siguiente_seq("Brasil vs Uruguay");      // Retorna 1 (tema nuevo)
 *   seq4 = obtener_siguiente_seq("Colombia vs Argentina");  // Retorna 3
 * 
 * Resultado:
 *   Colombia: seq=1, 2, 3...
 *   Brasil:   seq=1, 2, 3...
 *   (Secuencias independientes)
 */
unsigned int obtener_siguiente_seq(char *tema) {
    // Buscar si el tema ya tiene secuencia asignada
    for (int i = 0; i < num_temas_seq; i++) {
        if (strcmp(secuencias_tema[i].tema, tema) == 0) {
            // Tema encontrado: incrementar y devolver secuencia
            return ++secuencias_tema[i].seq;
        }
    }
    
    // Tema nuevo: crear entrada con seq=1
    if (num_temas_seq < 50) {
        strcpy(secuencias_tema[num_temas_seq].tema, tema);
        secuencias_tema[num_temas_seq].seq = 1;
        num_temas_seq++;
        return 1;
    }
    
    return 1;  // Fallback si llegamos al límite de temas
}

/**
 * guardar_historial - Guarda un mensaje en el buffer circular de historial
 * 
 * El historial permite retransmitir mensajes perdidos. Funciona como un
 * buffer circular: al llegar a MAX_HISTORIAL (100), vuelve al inicio.
 * 
 * Comportamiento del buffer circular:
 *   Posiciones: [0] [1] [2] ... [98] [99]
 *                ↑                      ↑
 *              inicio                 final
 *   
 *   Mensajes 1-100: llenan posiciones 0-99
 *   Mensaje 101: sobrescribe posición 0
 *   Mensaje 102: sobrescribe posición 1
 *   ...
 * 
 * Parámetros:
 *   @param seq: Número de secuencia del mensaje
 *   @param tema: Tema al que pertenece
 *   @param mensaje: Contenido del mensaje (sin tema)
 * 
 * Nota: Se guarda el mensaje SIN el prefijo "tema:", solo el contenido.
 */
void guardar_historial(unsigned int seq, char *tema, char *mensaje) {
    historial[historial_index].seq = seq;
    strcpy(historial[historial_index].tema, tema);
    strcpy(historial[historial_index].mensaje, mensaje);
    
    // Avanzar índice con wrap-around (circular)
    historial_index = (historial_index + 1) % MAX_HISTORIAL;
}

/**
 * buscar_en_historial - Busca un mensaje por su número de secuencia
 * 
 * Cuando un subscriber detecta pérdida de paquete, solicita retransmisión
 * enviando el seq del mensaje perdido. Esta función busca ese mensaje
 * en el historial.
 * 
 * IMPORTANTE: Busca por seq GLOBAL, no por seq de tema.
 * El broker luego verifica que el tema coincida antes de retransmitir.
 * 
 * Parámetros:
 *   @param seq: Número de secuencia a buscar
 *   @param tema_out: [OUTPUT] Tema del mensaje encontrado
 *   @param mensaje_out: [OUTPUT] Contenido del mensaje encontrado
 * 
 * Retorna:
 *   1 si el mensaje fue encontrado
 *   0 si no se encontró (mensaje muy antiguo o nunca existió)
 * 
 * Ejemplo:
 *   char tema[50], msg[500];
 *   if (buscar_en_historial(42, tema, msg)) {
 *       printf("Encontrado: tema=%s msg=%s\n", tema, msg);
 *   } else {
 *       printf("Mensaje seq=42 no encontrado en historial\n");
 *   }
 */
int buscar_en_historial(unsigned int seq, char *tema_out, char *mensaje_out) {
    // Buscar linealmente en todo el historial
    for (int i = 0; i < MAX_HISTORIAL; i++) {
        if (historial[i].seq == seq) {
            // Mensaje encontrado: copiar datos a las variables de salida
            strcpy(tema_out, historial[i].tema);
            strcpy(mensaje_out, historial[i].mensaje);
            return 1;  // Éxito
        }
    }
    return 0;  // No encontrado
}

/**
 * agregar_suscripcion - Registra un nuevo suscriptor para un tema
 * 
 * Cuando un subscriber envía paquete tipo 'S', se llama a esta función
 * para agregarlo a la lista de suscriptores.
 * 
 * Nota: Si un subscriber se suscribe a múltiples temas, aparecerá
 * múltiples veces en el array (una por cada tema).
 * 
 * Parámetros:
 *   @param tema: Tema al que se suscribe
 *   @param addr: Dirección IP y puerto del subscriber
 * 
 * Ejemplo:
 *   Subscriber 192.168.1.100:5000 se suscribe a:
 *     - "Colombia vs Argentina"
 *     - "Brasil vs Uruguay"
 *   
 *   Resultado en array suscriptores[]:
 *     [0] tema="Colombia vs Argentina" addr=192.168.1.100:5000
 *     [1] tema="Brasil vs Uruguay"     addr=192.168.1.100:5000
 */
void agregar_suscripcion(char *tema, struct sockaddr_in addr) {
    if (num_subs < MAX_SUBS) {
        strcpy(suscriptores[num_subs].tema, tema);
        suscriptores[num_subs].addr = addr;
        num_subs++;
        printf("[+] Suscriptor agregado para tema: %s\n", tema);
    } else {
        printf("[!] ERROR: Máximo de suscriptores alcanzado (%d)\n", MAX_SUBS);
    }
}

/**
 * publicar - Distribuye un mensaje a todos los suscriptores del tema
 * 
 * Esta es la función CORE del patrón Publisher-Subscriber.
 * 
 * Flujo de operación:
 *   1. Obtener número de secuencia específico para el tema
 *   2. Guardar mensaje en historial (para posible retransmisión)
 *   3. Iterar sobre todos los suscriptores
 *   4. Para cada suscriptor del tema correspondiente:
 *      a. Crear paquete tipo 'P' con seq y "tema:mensaje"
 *      b. Enviar por UDP al suscriptor
 * 
 * Formato del paquete enviado:
 *   pkt.seq = <número de secuencia del tema>
 *   pkt.tipo = 'P'
 *   pkt.mensaje = "tema:contenido"
 *   
 *   Ejemplo: "Colombia vs Argentina:Gol al minuto 45"
 * 
 * ¿Por qué incluir el tema en pkt.mensaje?
 *   - El subscriber necesita saber el tema para:
 *     a. Verificar que está suscrito a ese tema
 *     b. Rastrear la secuencia correcta (cada tema tiene su propia secuencia)
 *     c. Solicitar retransmisión con el tema correcto
 * 
 * Parámetros:
 *   @param sock: Socket UDP para envío
 *   @param tema: Tema del mensaje (ej: "Colombia vs Argentina")
 *   @param mensaje: Contenido del mensaje (ej: "Gol al minuto 45")
 * 
 * Ejemplo de ejecución:
 *   publicar(sock, "Colombia vs Argentina", "Gol al minuto 10");
 *   
 *   Output:
 *     seq = 1 (primera publicación de este tema)
 *     Guarda en historial: seq=1, tema="Colombia vs Argentina", msg="Gol al minuto 10"
 *     Para cada suscriptor de "Colombia vs Argentina":
 *       Envía paquete con: seq=1, tipo='P', mensaje="Colombia vs Argentina:Gol al minuto 10"
 */
void publicar(SOCKET sock, char *tema, char *mensaje) {
    Paquete pkt;
    
    // 1. Obtener secuencia específica para este tema
    //    (Colombia seq=1, Brasil seq=1 son independientes)
    unsigned int seq_actual = obtener_siguiente_seq(tema);
    
    // 2. Guardar en historial para retransmisión futura
    guardar_historial(seq_actual, tema, mensaje);
    
    // 3. Enviar a todos los suscriptores del tema
    for (int i = 0; i < num_subs; i++) {
        // Verificar si este suscriptor está suscrito a este tema
        if (strcmp(suscriptores[i].tema, tema) == 0) {
            // Crear paquete QUIC
            pkt.seq = seq_actual;  // Secuencia específica del tema
            pkt.tipo = 'P';        // Publicación
            
            // Incluir tema en el mensaje: "tema:contenido"
            // Esto permite al subscriber identificar y filtrar mensajes
            sprintf(pkt.mensaje, "%s:%s", tema, mensaje);
            
            // Enviar por UDP (sin confirmación en esta etapa)
            sendto(sock, (char*)&pkt, sizeof(Paquete), 0,
                   (struct sockaddr*)&suscriptores[i].addr, 
                   sizeof(suscriptores[i].addr));
            
            printf("[->] Enviado seq=%u a suscriptor de '%s'\n", pkt.seq, tema);
        }
    }
}

// ============================================================================
// FUNCIÓN PRINCIPAL
// ============================================================================

/**
 * main - Punto de entrada del broker QUIC
 * 
 * Ciclo principal del broker:
 *   1. Inicializar socket UDP en puerto 7000
 *   2. Escuchar mensajes indefinidamente
 *   3. Procesar según tipo de paquete:
 *      - 'S': Agregar suscripción y enviar ACK
 *      - 'P': Distribuir publicación y enviar ACK
 *      - 'R': Buscar en historial y retransmitir si corresponde
 *      - 'A': Ignorar (ACKs de subscribers)
 * 
 * Tipos de paquetes manejados:
 *   
 *   TIPO 'S' - SUSCRIPCIÓN:
 *     Subscriber → Broker
 *     pkt.mensaje = "Colombia vs Argentina"
 *     Acción: agregar_suscripcion() y enviar ACK
 *   
 *   TIPO 'P' - PUBLICACIÓN:
 *     Publisher → Broker
 *     pkt.mensaje = "Colombia vs Argentina:Gol"
 *     Acción: publicar() a suscriptores y enviar ACK
 *   
 *   TIPO 'R' - RETRANSMISIÓN:
 *     Subscriber → Broker
 *     pkt.seq = 5 (mensaje perdido)
 *     pkt.mensaje = "Colombia vs Argentina" (tema esperado)
 *     Acción: buscar seq=5 en historial, verificar tema, retransmitir
 *   
 *   TIPO 'A' - ACK:
 *     Subscriber → Broker (confirmación)
 *     Acción: ignorar (no requiere respuesta)
 * 
 * Retorna:
 *   0 en operación normal (nunca sale del bucle while)
 */
int main() {
int main() {
    // Variables locales
    WSADATA wsa;                    // Datos de inicialización de Winsock
    SOCKET sock;                    // Socket UDP del broker
    struct sockaddr_in servidor, cliente;  // Direcciones de red
    Paquete pkt, ack;              // Paquetes de entrada y salida
    int tam_cliente = sizeof(cliente);
    
    // Inicializar Winsock (requerido en Windows para sockets)
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    // Crear socket UDP (SOCK_DGRAM)
    // QUIC trabaja sobre UDP para evitar el handshake de TCP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Configurar dirección del servidor
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
    servidor.sin_port = htons(PUERTO);       // Puerto 7000
    
    // Enlazar socket al puerto
    bind(sock, (struct sockaddr*)&servidor, sizeof(servidor));
    
    printf("=== BROKER QUIC ===\n");
    printf("Puerto: %d (UDP)\n", PUERTO);
    printf("Esperando mensajes...\n\n");
    
    // ========================================================================
    // BUCLE PRINCIPAL - Procesar mensajes indefinidamente
    // ========================================================================
    while (1) {
        // Recibir paquete UDP (bloqueante)
        // recvfrom() espera hasta que llegue un paquete
        int bytes = recvfrom(sock, (char*)&pkt, sizeof(Paquete), 0,
                            (struct sockaddr*)&cliente, &tam_cliente);
        
        if (bytes > 0) {
            printf("\n[RX] Tipo='%c' Seq=%u\n", pkt.tipo, pkt.seq);
            
            // ================================================================
            // CASO 1: SUSCRIPCIÓN (tipo 'S')
            // ================================================================
            // Subscriber envía: pkt.mensaje = "Colombia vs Argentina"
            // Acción: Agregar a lista de suscriptores y confirmar con ACK
            if (pkt.tipo == 'S') {
                printf("     Suscripción a: %s\n", pkt.mensaje);
                
                // Registrar suscriptor en la lista
                agregar_suscripcion(pkt.mensaje, cliente);
                
                // Enviar ACK de confirmación
                ack.seq = pkt.seq;  // Eco del seq recibido
                ack.tipo = 'A';
                strcpy(ack.mensaje, "OK");
                sendto(sock, (char*)&ack, sizeof(Paquete), 0,
                       (struct sockaddr*)&cliente, tam_cliente);
                printf("[<-] ACK enviado\n");
                
            // ================================================================
            // CASO 2: PUBLICACIÓN (tipo 'P')
            // ================================================================
            // Publisher envía: pkt.mensaje = "TEMA:contenido"
            // Acción: Distribuir a suscriptores del tema y confirmar con ACK
            } else if (pkt.tipo == 'P') {
                // Parsear mensaje: formato "TEMA:contenido"
                char *tema = strtok(pkt.mensaje, ":");  // Extraer tema
                char *msg = strtok(NULL, "");           // Extraer contenido
                
                if (tema && msg) {
                    printf("     Publicación: tema='%s' msg='%s'\n", tema, msg);
                    
                    // Distribuir mensaje a todos los suscriptores del tema
                    publicar(sock, tema, msg);
                    
                    // Enviar ACK al publisher para confirmar recepción
                    ack.seq = pkt.seq;  // Eco del seq del publisher
                    ack.tipo = 'A';
                    strcpy(ack.mensaje, "OK");
                    sendto(sock, (char*)&ack, sizeof(Paquete), 0,
                           (struct sockaddr*)&cliente, tam_cliente);
                    printf("[<-] ACK enviado\n");
                } else {
                    printf("[!] ERROR: Formato incorrecto de publicación\n");
                }
                
            // ================================================================
            // CASO 3: RETRANSMISIÓN (tipo 'R')
            // ================================================================
            // Subscriber envía:
            //   pkt.seq = 5 (mensaje perdido)
            //   pkt.mensaje = "Colombia vs Argentina" (tema esperado)
            // Acción: Buscar seq=5 en historial, verificar tema, retransmitir
            } else if (pkt.tipo == 'R') {
                unsigned int seq_solicitado = pkt.seq;
                char tema_solicitado[50];
                strcpy(tema_solicitado, pkt.mensaje);  // Tema esperado por subscriber
                
                printf("     Solicitud de retransmisión: seq=%u tema='%s'\n", 
                       seq_solicitado, tema_solicitado);
                
                char tema_retrans[50];
                char msg_retrans[500];
                
                // Buscar mensaje en historial por seq
                if (buscar_en_historial(seq_solicitado, tema_retrans, msg_retrans)) {
                    // Mensaje encontrado en historial
                    
                    // VERIFICACIÓN CRÍTICA: El tema debe coincidir
                    // Evita enviar "Brasil:Gol" a subscriber de "Colombia"
                    if (strcmp(tema_retrans, tema_solicitado) == 0) {
                        // Tema correcto: verificar que el subscriber está suscrito
                        for (int i = 0; i < num_subs; i++) {
                            // Verificar: mismo tema + misma IP + mismo puerto
                            if (strcmp(suscriptores[i].tema, tema_retrans) == 0 &&
                                cliente.sin_addr.s_addr == suscriptores[i].addr.sin_addr.s_addr &&
                                cliente.sin_port == suscriptores[i].addr.sin_port) {
                                
                                // Crear paquete de retransmisión
                                Paquete retrans;
                                retrans.seq = seq_solicitado;  // Mismo seq del mensaje original
                                retrans.tipo = 'P';             // Enviarlo como publicación normal
                                sprintf(retrans.mensaje, "%s:%s", tema_retrans, msg_retrans);
                                
                                // Reenviar mensaje
                                sendto(sock, (char*)&retrans, sizeof(Paquete), 0,
                                       (struct sockaddr*)&cliente, sizeof(cliente));
                                
                                printf("[->] RETRANSMITIDO seq=%u de tema '%s' a suscriptor\n", 
                                       seq_solicitado, tema_retrans);
                                break;
                            }
                        }
                    } else {
                        // Tema incorrecto: no retransmitir
                        printf("[!] seq=%u es de tema '%s', pero se solicitó tema '%s' - ignorando\n", 
                               seq_solicitado, tema_retrans, tema_solicitado);
                    }
                } else {
                    // Mensaje no encontrado en historial (muy antiguo o nunca existió)
                    printf("[!] Mensaje seq=%u no encontrado en historial\n", seq_solicitado);
                }
                
            // ================================================================
            // CASO 4: ACK (tipo 'A')
            // ================================================================
            // Subscribers pueden enviar ACKs, pero el broker no los necesita
            } else if (pkt.tipo == 'A') {
                // Ignorar ACKs de subscribers
                // (solo registrar en log si se desea)
            }
        }
    }
    
    // Limpieza (código inalcanzable en operación normal)
    closesocket(sock);
    WSACleanup();
    return 0;
}
}
