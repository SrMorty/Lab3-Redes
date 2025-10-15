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
#define MAX_MSG 512 // Máximo tamaño del mensaje

int main() {
    int sock;
    struct sockaddr_in broker_addr;
    char topic[50], message[256], buffer[MAX_MSG];

    // Crear socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        exit(1);
    }

    // Configurar dirección del broker
    memset(&broker_addr, 0, sizeof(broker_addr)); // Limpiar estructura
    broker_addr.sin_family = AF_INET; // IPv4
    broker_addr.sin_port = htons(BROKER_PORT); // Puerto del broker
    inet_pton(AF_INET, BROKER_IP, &broker_addr.sin_addr); // Convertir IP

    // Tema a publicar
    printf("Ingrese el tema al cual va a publicar (partido): ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = 0;

    while (1) {
        // Mensaje a publicar
        printf("Ingrese el mensaje el cual va a publicar: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;

        // Guardar en el buffer el mensaje con formato "PUBLISH:topic:message"
        snprintf(buffer, sizeof(buffer), "PUBLISH:%s:%s", topic, message);

        // Enviar mensaje al broker
        sendto(sock, buffer, strlen(buffer), 0,
               (struct sockaddr *)&broker_addr, sizeof(broker_addr));

        printf("Mensaje publicado en: '%s'\n", topic);
    }

    close(sock);
    return 0;
}
