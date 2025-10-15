#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BROKER_PORT 5000
#define BROKER_IP "172.20.224.1"
#define BUF_SIZE 512

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in broker_addr;
    char buffer[BUF_SIZE];

    WSAStartup(MAKEWORD(2,2), &wsa);
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr = inet_addr(BROKER_IP);
    broker_addr.sin_port = htons(BROKER_PORT);

    printf("Publisher conectado al broker en %s:%d\n", BROKER_IP, BROKER_PORT);
    printf("Formato de mensaje: ARGvsCOL Gol de Colombia al minuto 32\n");

    while (1) {
        printf("Ingrese evento (o 'salir'): ");
        fgets(buffer, BUF_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strcmp(buffer, "salir") == 0)
            break;

        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&broker_addr, sizeof(broker_addr));
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
