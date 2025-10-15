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
    struct sockaddr_in broker_addr, local_addr;
    char buffer[BUF_SIZE];
    int addr_len = sizeof(broker_addr);

    WSAStartup(MAKEWORD(2,2), &wsa);
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
    bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr));

    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr = inet_addr(BROKER_IP);
    broker_addr.sin_port = htons(BROKER_PORT);

    // Pedir partidos
    printf("Ingrese los partidos a los que desea suscribirse (ej: ARGvsCOL ITAvsCOL): ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    char mensaje[BUF_SIZE];
    sprintf(mensaje, "SUB %s", buffer);
    sendto(sock, mensaje, strlen(mensaje), 0, (struct sockaddr*)&broker_addr, sizeof(broker_addr));

    printf("Suscrito a: %s\n", buffer);
    printf("Esperando actualizaciones...\n");

    while (1) {
        int recv_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*)&broker_addr, &addr_len);
        buffer[recv_len] = '\0';
        printf("%s\n", buffer);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
