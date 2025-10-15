#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Se usaron otras dos librerías
// unistd.h sirve para close(), que cierra el socket
// arpa/inet.h sirve para inet_addr() y htons(), que convierten direcciones y puertos
#include <arpa/inet.h>
#include <unistd.h>

#define BROKER_IP "127.0.0.1" // IP del broker (local)
#define BROKER_PORT 8080 // Puerto del broker
#define LOCAL_PORT 0 // el sistema elige el puerto local
#define MAX_MSG 512 // Máximo tamaño del mensaje

int main() {
    int sock;
    struct sockaddr_in local_addr, broker_addr;
    char topic[50], buffer[MAX_MSG];
    socklen_t addr_len = sizeof(local_addr);

    // Crear socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        exit(1);
    }

    // Configurar dirección local
    memset(&local_addr, 0, sizeof(local_addr)); // Limpiar estructura
    local_addr.sin_family = AF_INET; // IPv4
    local_addr.sin_port = htons(LOCAL_PORT); // Puerto
    local_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces

    // Enlazar el socket
    bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));

    // Obtener el puerto asignado
    memset(&broker_addr, 0, sizeof(broker_addr)); // Limpiar estructura
    broker_addr.sin_family = AF_INET; // IPv4
    broker_addr.sin_port = htons(BROKER_PORT); // Puerto del broker
    inet_pton(AF_INET, BROKER_IP, &broker_addr.sin_addr); // Convertir IP

    // Tema a suscribir
    printf("Ingrese el tema al cual se va a suscribir (partido): ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = 0;

    // Enviar solicitud de suscripción al broker
    char sub_msg[100];
    snprintf(sub_msg, sizeof(sub_msg), "SUBSCRIBE:%s", topic);
    sendto(sock, sub_msg, strlen(sub_msg), 0,
           (struct sockaddr *)&broker_addr, sizeof(broker_addr));

    printf("Suscrito al tema: '%s'. Esperando mensajes...\n", topic);

    while (1) {
        // Esperar mensajes del broker
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             NULL, NULL);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            // Mostrar mensaje recibido
            printf("[Mensaje recibido] %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
