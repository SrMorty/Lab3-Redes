/*
 * SUBSCRIBER QUIC - Suscriptor con protocolo híbrido
 * 
 * Características QUIC:
 * - Recibe por UDP (sin conexión persistente)
 * - Verifica números de secuencia para detectar pérdidas
 * - Envía ACKs para confirmar recepción (confiabilidad)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define BROKER_IP "127.0.0.1"
#define PUERTO 7000
#define TIMEOUT 5000  // Timeout general en ms

// Estructura simple de paquete QUIC
typedef struct {
    unsigned int seq;      // Número de secuencia
    char tipo;             // 'S'=suscripción, 'P'=publicación, 'A'=ACK, 'R'=retransmisión
    char mensaje[500];     // Contenido del mensaje
} Paquete;

// Estructura para rastrear secuencias por tema
typedef struct {
    char tema[50];
    unsigned int ultimo_seq;
} SecuenciaTema;

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in broker;
    Paquete pkt, ack;
    char input[200];
    char tema[50];
    int tam_broker = sizeof(broker);
    int num_temas = 0;
    int bytes;
    SecuenciaTema temas_suscritos[10];  // Hasta 10 temas
    memset(temas_suscritos, 0, sizeof(temas_suscritos));
    
    WSAStartup(MAKEWORD(2,2), &wsa);
    
    // Crear socket UDP (QUIC usa UDP como base)
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Dirección del broker
    broker.sin_family = AF_INET;
    broker.sin_addr.s_addr = inet_addr(BROKER_IP);
    broker.sin_port = htons(PUERTO);
    
    printf("=== SUBSCRIBER QUIC ===\n");
    printf("Broker: %s:%d\n\n", BROKER_IP, PUERTO);
    
    // Suscribirse a múltiples temas
    printf("Ingrese temas separados por comas (ej: Colombia vs Argentina, Brasil vs Uruguay)\n");
    printf("O un solo tema (ej: Colombia vs Argentina): ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';
    
    // Procesar y enviar cada suscripción
    char *token = strtok(input, ",");
    while (token != NULL) {
        // Eliminar espacios al inicio
        while (*token == ' ') token++;
        
        // Copiar tema limpio
        strcpy(tema, token);
        
        // Guardar tema en la lista de suscritos
        strcpy(temas_suscritos[num_temas].tema, tema);
        temas_suscritos[num_temas].ultimo_seq = 0;
        
        // Enviar suscripción (sin conexión previa - característica UDP)
        pkt.seq = num_temas + 1;
        pkt.tipo = 'S';
        strcpy(pkt.mensaje, tema);
        
        printf("[->] Enviando suscripción a '%s'...\n", tema);
        sendto(sock, (char*)&pkt, sizeof(Paquete), 0,
               (struct sockaddr*)&broker, sizeof(broker));
        
        // Esperar ACK de confirmación
        bytes = recvfrom(sock, (char*)&ack, sizeof(Paquete), 0,
                        (struct sockaddr*)&broker, &tam_broker);
        
        if (bytes > 0 && ack.tipo == 'A') {
            printf("[<-] Confirmación: %s\n", ack.mensaje);
        }
        
        num_temas++;
        token = strtok(NULL, ",");
    }
    
    printf("\nSuscrito a %d tema(s). Esperando mensajes...\n", num_temas);
    printf("(Presiona Ctrl+C para salir)\n\n");
    
    // Bucle para recibir publicaciones
    while (1) {
        bytes = recvfrom(sock, (char*)&pkt, sizeof(Paquete), 0,
                        (struct sockaddr*)&broker, &tam_broker);
        
        if (bytes > 0 && pkt.tipo == 'P') {
            // Extraer el tema del mensaje (formato: "Tema:Mensaje")
            char tema_msg[50] = {0};
            char contenido[500] = {0};
            char *separador = strchr(pkt.mensaje, ':');
            
            if (separador != NULL) {
                int len_tema = separador - pkt.mensaje;
                strncpy(tema_msg, pkt.mensaje, len_tema);
                tema_msg[len_tema] = '\0';
                strcpy(contenido, separador + 1);
            } else {
                strcpy(contenido, pkt.mensaje);
            }
            
            // Buscar el tema en la lista de suscritos
            int tema_encontrado = -1;
            for (int i = 0; i < num_temas; i++) {
                if (strcmp(temas_suscritos[i].tema, tema_msg) == 0) {
                    tema_encontrado = i;
                    break;
                }
            }
            
            // Solo procesar si estamos suscritos a este tema
            if (tema_encontrado >= 0) {
                unsigned int ultimo_seq_tema = temas_suscritos[tema_encontrado].ultimo_seq;
                
                // Verificar secuencia para detectar pérdidas SOLO en este tema
                if (ultimo_seq_tema > 0 && pkt.seq != ultimo_seq_tema + 1) {
                    // RETRANSMISIÓN: Solicitar paquetes perdidos
                    for (unsigned int seq_perdido = ultimo_seq_tema + 1; seq_perdido < pkt.seq; seq_perdido++) {
                        printf("[!] Pérdida detectada en '%s' - solicitando retransmisión de seq=%u\n", 
                               tema_msg, seq_perdido);
                        
                        // Solicitar retransmisión - INCLUIR EL TEMA ESPERADO
                        Paquete retrans_req;
                        retrans_req.seq = seq_perdido;
                        retrans_req.tipo = 'R';  // Tipo Retransmisión
                        sprintf(retrans_req.mensaje, "%s", tema_msg);  // Enviar tema esperado
                        
                        sendto(sock, (char*)&retrans_req, sizeof(Paquete), 0,
                               (struct sockaddr*)&broker, sizeof(broker));
                        
                        // Esperar retransmisión (timeout corto)
                        DWORD timeout_original = TIMEOUT;
                        DWORD timeout_retrans = 500;  // 500ms para retransmisión
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_retrans, sizeof(timeout_retrans));
                        
                        int bytes_retrans = recvfrom(sock, (char*)&ack, sizeof(Paquete), 0,
                                                    (struct sockaddr*)&broker, &tam_broker);
                        
                        if (bytes_retrans > 0 && ack.tipo == 'P' && ack.seq == seq_perdido) {
                            // Extraer contenido del mensaje retransmitido
                            char *sep_retrans = strchr(ack.mensaje, ':');
                            char contenido_retrans[500];
                            if (sep_retrans != NULL) {
                                strcpy(contenido_retrans, sep_retrans + 1);
                            } else {
                                strcpy(contenido_retrans, ack.mensaje);
                            }
                            
                            printf("[<-] RETRANSMITIDO [%s] seq=%u: %s\n", 
                                   tema_msg, ack.seq, contenido_retrans);
                            
                            // Enviar ACK
                            Paquete ack_retrans;
                            ack_retrans.seq = ack.seq;
                            ack_retrans.tipo = 'A';
                            strcpy(ack_retrans.mensaje, "OK");
                            sendto(sock, (char*)&ack_retrans, sizeof(Paquete), 0,
                                   (struct sockaddr*)&broker, sizeof(broker));
                        } else {
                            printf("[!] No se pudo recuperar seq=%u de '%s'\n", seq_perdido, tema_msg);
                        }
                        
                        // Restaurar timeout original
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_original, sizeof(timeout_original));
                    }
                }
                
                // Mostrar mensaje recibido CON EL TEMA
                printf("[RX] [%s] seq=%u: %s\n", tema_msg, pkt.seq, contenido);
                
                // Actualizar última secuencia de ESTE tema
                temas_suscritos[tema_encontrado].ultimo_seq = pkt.seq;
                
                // Enviar ACK al broker (confirmar recepción)
                ack.seq = pkt.seq;
                ack.tipo = 'A';
                strcpy(ack.mensaje, "OK");
                sendto(sock, (char*)&ack, sizeof(Paquete), 0,
                       (struct sockaddr*)&broker, sizeof(broker));
            }
            // Si no estamos suscritos, simplemente ignoramos el mensaje
        }
        
        Sleep(50);  // Pequeña pausa para no saturar CPU
    }
    
    closesocket(sock);
    WSACleanup();
    return 0;
}
