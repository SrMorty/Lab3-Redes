#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define PUERTO 6000
#define MAX_CONEXIONES 50
#define MAX_TEMAS 10
#define TAM 512

typedef struct {
    SOCKET canal;
    int activo;
    int tipo;
    char temas[MAX_TEMAS][40];
    int cantidad;
} Conexion;

int coincide(char *mensaje, char *tema) {
    return strstr(mensaje, tema) != NULL;
}

int main() {
    WSADATA datos;
    SOCKET servidor, cliente;
    struct sockaddr_in dir_servidor, dir_cliente;
    Conexion lista[MAX_CONEXIONES];
    fd_set lista_lectura;
    char mensaje[TAM];
    int tam_dir = sizeof(dir_cliente);

    WSAStartup(MAKEWORD(2,2), &datos);
    servidor = socket(AF_INET, SOCK_STREAM, 0);
    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_addr.s_addr = INADDR_ANY;
    dir_servidor.sin_port = htons(PUERTO);
    bind(servidor, (struct sockaddr*)&dir_servidor, sizeof(dir_servidor));
    listen(servidor, 10);

    for (int i = 0; i < MAX_CONEXIONES; i++) lista[i].activo = 0;
    printf("Broker activo en puerto %d\n", PUERTO);

    while (1) {
        FD_ZERO(&lista_lectura);
        FD_SET(servidor, &lista_lectura);
        SOCKET mayor = servidor;

        for (int i = 0; i < MAX_CONEXIONES; i++) {
            if (lista[i].activo) {
                FD_SET(lista[i].canal, &lista_lectura);
                if (lista[i].canal > mayor) mayor = lista[i].canal;
            }
        }

        select(0, &lista_lectura, NULL, NULL, NULL);

        if (FD_ISSET(servidor, &lista_lectura)) {
            cliente = accept(servidor, (struct sockaddr*)&dir_cliente, &tam_dir);
            for (int j = 0; j < MAX_CONEXIONES; j++) {
                if (!lista[j].activo) {
                    lista[j].canal = cliente;
                    lista[j].activo = 1;
                    lista[j].tipo = 0;
                    lista[j].cantidad = 0;
                    break;
                }
            }
            printf("Conectado %s\n", inet_ntoa(dir_cliente.sin_addr));
        }

        for (int i = 0; i < MAX_CONEXIONES; i++) {
            if (!lista[i].activo) continue;
            if (!FD_ISSET(lista[i].canal, &lista_lectura)) continue;

            int recibidos = recv(lista[i].canal, mensaje, TAM - 1, 0);
            if (recibidos <= 0) continue;
            mensaje[recibidos] = '\0';

            if (strncmp(mensaje, "SUB ", 4) == 0) {
                lista[i].tipo = 1;
                lista[i].cantidad = 0;
                char *token = strtok(mensaje + 4, " ");
                while (token && lista[i].cantidad < MAX_TEMAS) {
                    strcpy(lista[i].temas[lista[i].cantidad++], token);
                    token = strtok(NULL, " ");
                }
                send(lista[i].canal, "Suscripcion exitosa\n", 21, 0);
            } else {
                for (int k = 0; k < MAX_CONEXIONES; k++) {
                    if (!lista[k].activo || lista[k].tipo != 1) continue;
                    for (int t = 0; t < lista[k].cantidad; t++) {
                        if (coincide(mensaje, lista[k].temas[t])) {
                            send(lista[k].canal, mensaje, strlen(mensaje), 0);
                            break;
                        }
                    }
                }
            }
        }
    }

    closesocket(servidor);
    WSACleanup();
    return 0;
}
