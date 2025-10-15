#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BROKER_PORT 5000
#define MAX_SUBS 10
#define MAX_MATCHES_PER_SUB 10
#define BUF_SIZE 512

typedef struct {
    struct sockaddr_in addr;
    int active;
    char matches[MAX_MATCHES_PER_SUB][50];
    int match_count;
} Subscriber;

int is_match_in_message(const char *message, const char *match) {
    return strstr(message, match) != NULL;
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in broker_addr, client_addr;
    int client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];
    Subscriber subs[MAX_SUBS] = {0};

    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr = INADDR_ANY;
    broker_addr.sin_port = htons(BROKER_PORT);

    bind(sock, (struct sockaddr*)&broker_addr, sizeof(broker_addr));
    printf("Broker UDP activo en el puerto %d\n", BROKER_PORT);

    while (1) {
        int recv_len = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        buffer[recv_len] = '\0';

        // Si el mensaje inicia con "SUB "
        if (strncmp(buffer, "SUB ", 4) == 0) {
            Subscriber *slot = NULL;
            for (int i = 0; i < MAX_SUBS; i++) {
                if (!subs[i].active) {
                    slot = &subs[i];
                    break;
                }
            }

            if (slot) {
                slot->addr = client_addr;
                slot->active = 1;
                slot->match_count = 0;

                // Leer múltiples partidos
                char *token = strtok(buffer + 4, " ");
                while (token && slot->match_count < MAX_MATCHES_PER_SUB) {
                    strcpy(slot->matches[slot->match_count++], token);
                    token = strtok(NULL, " ");
                }

                printf("Nuevo suscriptor (%s:%d) a partidos: ",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                for (int i = 0; i < slot->match_count; i++)
                    printf("%s ", slot->matches[i]);
                printf("\n");
            }
        } else {
            // Mensaje del publisher
            printf("Mensaje recibido del publisher: %s\n", buffer);

            // Reenviar solo a los suscriptores interesados
            for (int i = 0; i < MAX_SUBS; i++) {
                if (subs[i].active) {
                    for (int j = 0; j < subs[i].match_count; j++) {
                        if (is_match_in_message(buffer, subs[i].matches[j])) {
                            sendto(sock, buffer, strlen(buffer), 0,
                                   (struct sockaddr*)&subs[i].addr, sizeof(subs[i].addr));
                            break;
                        }
                    }
                }
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
