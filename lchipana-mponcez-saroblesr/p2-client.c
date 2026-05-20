/*
 * p2-client.c
 * cliente de consulta de vuelos
 * practica 3 - sistemas operativos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocolo.h"

#define IP_SERVIDOR "172.20.10.4"  // ip del servidor


// limpia lo que queda en el buffer despues de scanf
void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// pausa antes de volver al menu
void esperarTecla() {
    printf("presione enter para continuar...");
    getchar();
}

// muestra el menu con el estado actual de los filtros
int mostrarMenu(char* origen, char* destino, char* tipo) {
    int op;
    printf("\nBienvenido\n");
    printf("origen       : %s\n", strlen(origen)  ? origen  : "sin ingresar");
    printf("destino      : %s\n", strlen(destino) ? destino : "sin ingresar");
    printf("tipo de vuelo: %s\n", strlen(tipo)    ? tipo    : "sin ingresar");
    printf("\n");
    printf("1. Ingresar origen\n");
    printf("2. Ingresar destino\n");
    printf("3. Ingresar tipo de vuelo\n");
    printf("4. Buscar tiempo de viaje medio\n");
    printf("5. Salir\n");
    printf("opcion: ");
    if (scanf("%d", &op) != 1) op = -1;
    limpiarBuffer();
    return op;
}

int main() {
    // crear socket tcp
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // configurar a donde conectarse
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PUERTO);
    inet_pton(AF_INET, IP_SERVIDOR, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        printf("verifique que el servidor este corriendo en %s:%d\n",
               IP_SERVIDOR, PUERTO);
        return 1;
    }

    // variables locales donde se guarda lo que el usuario va ingresando
    char origen[100]  = "";
    char destino[100] = "";
    char tipo[20]     = "";

    int op;
    while ((op = mostrarMenu(origen, destino, tipo)) != OP_SALIR) {

        if (op == OP_ORIGEN) {
            printf("ciudad de origen: ");
            fgets(origen, sizeof(origen), stdin);
            origen[strcspn(origen, "\n")] = '\0'; // quitar el enter
            printf("origen guardado: %s\n", origen);
            esperarTecla();

        } else if (op == OP_DESTINO) {
            printf("ciudad de destino: ");
            fgets(destino, sizeof(destino), stdin);
            destino[strcspn(destino, "\n")] = '\0';
            printf("destino guardado: %s\n", destino);
            esperarTecla();

        } else if (op == OP_TIPODEVUELO) {
            printf("tipo de vuelo (firstClass / economic / premium)\n");
            printf("presione enter para omitir: ");
            fgets(tipo, sizeof(tipo), stdin);
            tipo[strcspn(tipo, "\n")] = '\0';
            if (strlen(tipo) == 0)
                printf("sin filtro de tipo de vuelo\n");
            else
                printf("tipo de vuelo guardado: %s\n", tipo);
            esperarTecla();

        } else if (op == OP_BUSCAR) {
            if (strlen(origen) == 0) {
                printf("debe ingresar un origen primero (opcion 1)\n");
                esperarTecla();
                continue;
            }

            // armar la peticion con todo lo que ingreso el usuario
            Peticion pet;
            memset(&pet, 0, sizeof(pet));
            pet.opcion = OP_BUSCAR;
            strncpy(pet.origen,      origen,  sizeof(pet.origen)      - 1);
            strncpy(pet.destino,     destino, sizeof(pet.destino)     - 1);
            strncpy(pet.tipodevuelo, tipo,    sizeof(pet.tipodevuelo) - 1);
            send(sock, &pet, sizeof(Peticion), 0); // enviar al servidor

            // esperar y mostrar la respuesta
            Respuesta resp;
            memset(&resp, 0, sizeof(resp));
            int n = recv(sock, &resp, sizeof(Respuesta), 0);

            if (n <= 0) {
                printf("sin respuesta del servidor\n");
            } else if (!resp.encontrado) {
                printf("resultado: NA\n");
            } else {
                printf("resultado: %s\n", resp.mensaje);
            }
            esperarTecla();

        } else {
            printf("opcion no valida\n");
            esperarTecla();
        }
    }

    // avisar al servidor que nos desconectamos
    Peticion pet;
    memset(&pet, 0, sizeof(pet));
    pet.opcion = OP_SALIR;
    send(sock, &pet, sizeof(Peticion), 0);

    close(sock);
    printf("hasta luego\n");
    return 0;
}
