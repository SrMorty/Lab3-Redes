#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define IP_BROKER "127.0.0.1"
#define PUERTO 6000
#define TAM 512

int main() {
    WSADATA datos;
    SOCKET canal;
    struct sockaddr_in destino;
    char mensaje[TAM];

    // Inicializa Winsock versión 2.2
    WSAStartup(MAKEWORD(2,2), &datos);

    // Crea el socket TCP
    canal = socket(AF_INET, SOCK_STREAM, 0);

    // Configura la dirección del broker
    destino.sin_family = AF_INET;
    destino.sin_addr.s_addr = inet_addr(IP_BROKER);
    destino.sin_port = htons(PUERTO);

    // Intenta conectar con el broker
    if (connect(canal, (struct sockaddr*)&destino, sizeof(destino)) != 0) {
        printf("No se pudo conectar al broker.\n");
        return 1;
    }

    printf("Publisher conectado al broker en %s:%d\n", IP_BROKER, PUERTO);
    printf("Formato del mensaje: PARTIDO Gol al minuto 32\n");

    // Bucle para enviar eventos
    while (1) {
        printf("Evento (o 'salir'): ");
        fgets(mensaje, TAM, stdin);
        mensaje[strcspn(mensaje, "\n")] = 0;

        if (strcmp(mensaje, "salir") == 0) break;

        // Envía el mensaje al broker
        send(canal, mensaje, strlen(mensaje), 0);
    }

    // Cierra el socket y libera Winsock
    closesocket(canal);
    WSACleanup();
    return 0;
}
