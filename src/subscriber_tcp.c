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

    WSAStartup(MAKEWORD(2,2), &datos);
    canal = socket(AF_INET, SOCK_STREAM, 0);
    destino.sin_family = AF_INET;
    destino.sin_addr.s_addr = inet_addr(IP_BROKER);
    destino.sin_port = htons(PUERTO);

    if (connect(canal, (struct sockaddr*)&destino, sizeof(destino)) != 0) {
        printf("No se pudo conectar al broker.\n");
        return 1;
    }

    printf("Ingrese partidos a suscribirse (ej: MEXvsCOL): ");
    fgets(entrada, TAM, stdin);
    entrada[strcspn(entrada, "\n")] = 0;
    sprintf(mensaje, "SUB %s", entrada);
    send(canal, mensaje, strlen(mensaje), 0);
    printf("Esperando confirmacion...\n");

    bytes = recv(canal, recibido, TAM - 1, 0);
    if (bytes > 0) {
        recibido[bytes] = '\0';
        printf("%s", recibido);
    }

    printf("Esperando actualizaciones...\n");

    while (1) {
        bytes = recv(canal, recibido, TAM - 1, 0);
        if (bytes > 0) {
            recibido[bytes] = '\0';
            printf("%s\n", recibido);
        } else {
            Sleep(200);
        }
    }

    closesocket(canal);
    WSACleanup();
    return 0;
}
