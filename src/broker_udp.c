#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Se usaron otras dos librerías
// unistd.h sirve para close(), que cierra el socket
// arpa/inet.h sirve para inet_addr() y htons(), que convierten direcciones y puertos
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080 // Puerto donde escucha el broker
#define MAX_SUBS 100 // Máximo número de suscriptores
#define MAX_TOPIC 50 // Máximo tamaño del tema
#define MAX_MSG 512 // Máximo tamaño del mensaje

typedef struct {
    char topic[MAX_TOPIC];
    struct sockaddr_in addr;
} Subscription; // Estructura para almacenar suscripciones

// Para saber el número de suscriptores actuales
Subscription subs[MAX_SUBS];
int sub_count = 0;

// Función para agregar una suscripción
void add_subscription(char *topic, struct sockaddr_in addr) {
    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subs[i].topic, topic) == 0 &&
            subs[i].addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            subs[i].addr.sin_port == addr.sin_port) {
            return; // ya suscrito
        }
    }
    // Agregar nueva suscripción
    if (sub_count < MAX_SUBS) {
        strcpy(subs[sub_count].topic, topic);
        subs[sub_count].addr = addr;
        sub_count++;
        printf("Nuevo suscriptor para el tema: '%s'\n", topic);
    }
}

// Función para reenviar mensajes a suscriptores
void publish_message(int sock, char *topic, char *msg) {
    for (int i = 0; i < sub_count; i++) {
        // Enviar mensaje a todos los suscriptores del tema
        if (strcmp(subs[i].topic, topic) == 0) {
            sendto(sock, msg, strlen(msg), 0,
                   (struct sockaddr *)&subs[i].addr, sizeof(subs[i].addr));
        }
    }
    printf("Mensaje reenviado a tema '%s': %s\n", topic, msg);
}

int main() {
    int sock;
    struct sockaddr_in broker_addr, client_addr;
    char buffer[MAX_MSG];
    socklen_t addr_len = sizeof(client_addr);

    // Crear socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        exit(1);
    }

    // Configurar dirección del broker
    memset(&broker_addr, 0, sizeof(broker_addr)); // Limpiar estructura
    broker_addr.sin_family = AF_INET; // IPv4
    broker_addr.sin_port = htons(PORT); // Puerto
    broker_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces

    // Enlazar el socket
    if (bind(sock, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        perror("Error en bind");
        exit(1);
    }

    printf("Broker escuchando en puerto %d...\n", PORT);

    while (1) {
        // Esperar mensajes de suscripción o publicación
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&client_addr, &addr_len);
        if (bytes < 0) continue;
        buffer[bytes] = '\0';

        // Procesar mensaje recibido
        if (strncmp(buffer, "SUBSCRIBE:", 10) == 0) {
            char *topic = buffer + 10;
            add_subscription(topic, client_addr);
        } else if (strncmp(buffer, "PUBLISH:", 8) == 0) { // Mensaje de publicación
            char *topic = strtok(buffer + 8, ":");
            char *msg = strtok(NULL, "");
            if (topic && msg)
                publish_message(sock, topic, msg);
        }
    }

    close(sock);
    return 0;
}
