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

    WSAStartup(MAKEWORD(2,2), &datos);
    canal = socket(AF_INET, SOCK_STREAM, 0);
    destino.sin_family = AF_INET;
    destino.sin_addr.s_addr = inet_addr(IP_BROKER);
    destino.sin_port = htons(PUERTO);

    if (connect(canal, (struct sockaddr*)&destino, sizeof(destino)) != 0) {
        printf("No se pudo conectar al broker.\n");
        return 1;
    }

    printf("Publisher conectado al broker en %s:%d\n", IP_BROKER, PUERTO);
    printf("Formato: PARvsBOL Gol al minuto 32\n");

    while (1) {
        printf("Evento (o 'salir'): ");
        fgets(mensaje, TAM, stdin);
        mensaje[strcspn(mensaje, "\n")] = 0;
        if (strcmp(mensaje, "salir") == 0) break;
        send(canal, mensaje, strlen(mensaje), 0);
    }

    closesocket(canal);
    WSACleanup();
    return 0;
}
