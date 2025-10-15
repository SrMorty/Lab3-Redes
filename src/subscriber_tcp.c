#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define IP_BROKER "127.0.0.1"
#define PUERTO 6000
#define TAM 512

int main() {
    WSADATA datos;
    SOCKET canal;
    struct sockaddr_in destino;
    char entrada[TAM], mensaje[TAM], recibido[TAM];
    int bytes;

    // Inicializa Winsock versión 2.2
    WSAStartup(MAKEWORD(2,2), &datos);

    // Crea un socket TCP
    canal = socket(AF_INET, SOCK_STREAM, 0);

    // Configura la dirección del broker
    destino.sin_family = AF_INET;
    destino.sin_addr.s_addr = inet_addr(IP_BROKER);
    destino.sin_port = htons(PUERTO);

    // Conecta con el broker
    if (connect(canal, (struct sockaddr*)&destino, sizeof(destino)) != 0) {
        printf("No se pudo conectar al broker.\n");
        return 1;
    }

    // El usuario ingresa los partidos a los que quiere suscribirse
    printf("Ingrese partidos a suscribirse (ej: MEXvsCOL): ");
    fgets(entrada, TAM, stdin);
    entrada[strcspn(entrada, "\n")] = 0;

    // Envía el mensaje de suscripción
    sprintf(mensaje, "SUB %s", entrada);
    send(canal, mensaje, strlen(mensaje), 0);

    printf("Esperando confirmación...\n");

    // Recibe confirmación del broker
    bytes = recv(canal, recibido, TAM - 1, 0);
    if (bytes > 0) {
        recibido[bytes] = '\0';
        printf("%s", recibido);
    }

    printf("Esperando actualizaciones...\n");

    // Espera mensajes publicados relacionados con los temas suscritos
    while (1) {
        bytes = recv(canal, recibido, TAM - 1, 0);
        if (bytes > 0) {
            recibido[bytes] = '\0';
            printf("%s\n", recibido);
        } else {
            Sleep(200); // Pausa corta para evitar sobrecarga del CPU
        }
    }

    // Cierre del socket y limpieza
    closesocket(canal);
    WSACleanup();
    return 0;
}
