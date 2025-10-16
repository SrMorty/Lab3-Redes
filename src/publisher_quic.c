/*
 * PUBLISHER QUIC - Publicador con protocolo híbrido
 * 
 * Características QUIC:
 * - Envía por UDP (sin conexión previa, sin handshake)
 * - Cada mensaje tiene número de secuencia
 * - Espera ACK para confirmar entrega (confiabilidad tipo TCP)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BROKER_IP "127.0.0.1"
#define PUERTO 7000
#define TIMEOUT 2000  // 2 segundos para recibir ACK

// Estructura simple de paquete QUIC
typedef struct {
    unsigned int seq;      // Número de secuencia
    char tipo;             // 'P'=publicación, 'A'=ACK
    char mensaje[500];     // Contenido del mensaje
} Paquete;


int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in broker;
    Paquete pkt, ack;
    char entrada[500];
    unsigned int seq = 1;
    int tam_broker = sizeof(broker);
    
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    // Crear socket UDP (QUIC usa UDP como base)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Configurar timeout para esperar ACK
    DWORD timeout = TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    // Dirección del broker
    broker.sin_family = AF_INET;
    broker.sin_addr.s_addr = inet_addr(BROKER_IP);
    broker.sin_port = htons(PUERTO);
    
    printf("=== PUBLISHER QUIC ===\n");
    printf("Broker: %s:%d\n", BROKER_IP, PUERTO);
    printf("Formato: TEMA:Mensaje\n");
    printf("Ejemplo: Colombia vs Argentina:Gol al minuto 45\n\n");
    
    while (1) {
        printf("Mensaje (o 'salir'): ");
        fgets(entrada, sizeof(entrada), stdin);
        entrada[strcspn(entrada, "\n")] = '\0';
        
        if (strcmp(entrada, "salir") == 0) break;
        
        // Crear paquete de publicación con número de secuencia
        pkt.seq = seq++;
        pkt.tipo = 'P';
        strcpy(pkt.mensaje, entrada);
        
        // Enviar por UDP (sin conexión establecida)
        printf("[->] Enviando seq=%u...\n", pkt.seq);
        sendto(sock, (char*)&pkt, sizeof(Paquete), 0,
               (struct sockaddr*)&broker, sizeof(broker));
        
        // Esperar ACK (confiabilidad tipo TCP)
        int bytes = recvfrom(sock, (char*)&ack, sizeof(Paquete), 0,
                            (struct sockaddr*)&broker, &tam_broker);
        
        if (bytes > 0 && ack.tipo == 'A') {
            printf("[<-] ACK recibido: %s\n\n", ack.mensaje);
        } else {
            printf("[!] Timeout - no se recibió ACK\n\n");
        }
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}
