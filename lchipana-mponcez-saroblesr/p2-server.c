/*
 * p2-server.c
 * servidor de consulta de vuelos
 * practica 3 - sistemas operativos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hash.h"
#include "protocolo.h"

#define CSV_FILE  "flights.csv"
#define LOG_FILE  "/mnt/c/Users/Luis1/Downloads/practica3_final/server.log"

// se guarda al inicio para que el proceso hijo sepa donde estan los archivos
static char dirbase[512];

// pasa el texto a minusculas para poder comparar sin importar mayusculas
void aMinusculas(char* src, char* dst, int tam) {
    int i;
    for (i = 0; i < tam - 1 && src[i]; i++)
        dst[i] = tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

// escribe en busquedas.log cada vez que un cliente hace una busqueda
// el log se guarda directo en la carpeta SistemasO3 de windows
void escribirLog(char* ip, char* origen, char* destino) {
    FILE* log = fopen(LOG_FILE, "a"); // abrir en modo append para no borrar lo anterior
    if (!log) return;

    time_t ahora = time(NULL);
    struct tm* t  = localtime(&ahora);
    char fecha[20];
    strftime(fecha, sizeof(fecha), "%Y%m%dT%H%M%S", t); // formato del pdf

    fprintf(log, "[%s] Cliente [%s] [busqueda - %s - %s]\n",
            fecha, ip, origen, destino);
    fclose(log);
}

// lee el csv una sola vez y guarda en la tabla hash
// solo guarda el nombre del origen y su posicion en el archivo (offset)
// los datos del vuelo quedan en disco, no en ram
void indexarCSV(FILE* csv) {
    char linea[512];
    fgets(linea, sizeof(linea), csv); // saltar encabezado

    while (!feof(csv)) {
        long pos = ftell(csv); // guardar posicion antes de leer la linea
        if (!fgets(linea, sizeof(linea), csv)) break;
        if (strlen(linea) < 5) continue;

        char c1[20], c2[20], origen[100];
        if (sscanf(linea, "%[^,],%[^,],%[^,]", c1, c2, origen) < 3)
            continue;

        insertarHash(origen, pos); // guardar origen + posicion en la tabla
    }
}

// busca vuelos que coincidan con los criterios del cliente
// usa busqueda parcial: "recife" encuentra "Recife (PE)"
// va directo al registro en disco con fseek, no carga todo el csv
Respuesta buscarVuelo(char* origen, char* destino, char* tipodevuelo) {
    Respuesta resp;
    memset(&resp, 0, sizeof(resp));

    char rutacsv[600];
    snprintf(rutacsv, sizeof(rutacsv), "%s/%s", dirbase, CSV_FILE);

    FILE* csv = fopen(rutacsv, "r");
    if (!csv) {
        strcpy(resp.mensaje, "error abriendo el archivo");
        return resp;
    }

    // convertir a minusculas para no distinguir entre Recife y recife
    char origenMin[100], destinoMin[100], tipoMin[30];
    aMinusculas(origen,      origenMin,  sizeof(origenMin));
    aMinusculas(destino,     destinoMin, sizeof(destinoMin));
    aMinusculas(tipodevuelo, tipoMin,    sizeof(tipoMin));

    float suma  = 0.0f;
    int   total = 0;
    int   i;

    // recorrer toda la tabla buscando origenes que coincidan
    for (i = 0; i < TAM_TABLA; i++) {
        NodoHash* nodo = tabla[i];

        while (nodo != NULL) {
            char nodoMin[100];
            aMinusculas(nodo->origen, nodoMin, sizeof(nodoMin));

            // strstr verifica si el origen del nodo contiene el texto buscado
            if (strstr(nodoMin, origenMin) == NULL) {
                nodo = nodo->siguiente;
                continue;
            }

            // ir directo al byte donde esta el registro en el archivo
            fseek(csv, nodo->offset, SEEK_SET);
            char linea[512];
            if (!fgets(linea, sizeof(linea), csv)) {
                nodo = nodo->siguiente;
                continue;
            }

            // leer todos los campos del registro
            char c1[20], c2[20], from[100], to[100], tipo[30];
            char agencia[50], fecha[20];
            float precio, tiempo, distancia;

            int campos = sscanf(linea,
                "%[^,],%[^,],%[^,],%[^,],%[^,],%f,%f,%f,%[^,],%s",
                c1, c2, from, to, tipo,
                &precio, &tiempo, &distancia, agencia, fecha);

            if (campos < 7) {
                nodo = nodo->siguiente;
                continue;
            }

            // filtrar por destino si el cliente ingreso uno
            if (strlen(destinoMin) > 0) {
                char toMin[100];
                aMinusculas(to, toMin, sizeof(toMin));
                if (strstr(toMin, destinoMin) == NULL) {
                    nodo = nodo->siguiente;
                    continue;
                }
            }

            // filtrar por tipo de vuelo si el cliente ingreso uno
            if (strlen(tipoMin) > 0) {
                char tipoRegMin[30];
                aMinusculas(tipo, tipoRegMin, sizeof(tipoRegMin));
                if (strstr(tipoRegMin, tipoMin) == NULL) {
                    nodo = nodo->siguiente;
                    continue;
                }
            }

            suma += tiempo; // acumular tiempo del vuelo
            total++;
            nodo = nodo->siguiente;
        }
    }

    fclose(csv);

    if (total == 0) {
        resp.encontrado = 0;
        strcpy(resp.mensaje, "NA");
    } else {
        resp.encontrado  = 1;
        resp.tiempoMedio = suma / total; // calcular promedio
        resp.cantVuelos  = total;
        snprintf(resp.mensaje, sizeof(resp.mensaje),
                 "tiempo medio: %.2f horas (%d vuelos encontrados)",
                 resp.tiempoMedio, total);
    }

    return resp;
}

// atiende a un cliente hasta que se desconecte o mande OP_SALIR
void atenderCliente(int fd, char* ip) {
    Peticion  pet;
    Respuesta resp;

    while (1) {
        int n = recv(fd, &pet, sizeof(Peticion), 0);
        if (n <= 0) break; // cliente desconectado
        if (pet.opcion == OP_SALIR) break;

        if (pet.opcion == OP_BUSCAR) {
            escribirLog(ip, pet.origen, pet.destino);
            resp = buscarVuelo(pet.origen, pet.destino, pet.tipodevuelo);
            send(fd, &resp, sizeof(Respuesta), 0);
        }
    }

    close(fd);
}

// cuando un hijo termina, esta funcion lo recoge para evitar procesos zombie
void sigchld(int s) {
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    // guardar carpeta actual para que el proceso hijo encuentre los archivos
    if (getcwd(dirbase, sizeof(dirbase)) == NULL) {
        perror("getcwd");
        return 1;
    }

    // abrir e indexar el csv al inicio
    char rutacsv[600];
    snprintf(rutacsv, sizeof(rutacsv), "%s/%s", dirbase, CSV_FILE);

    FILE* csv = fopen(rutacsv, "r");
    if (!csv) {
        perror("no se pudo abrir flights.csv");
        return 1;
    }
    iniciarTabla();
    indexarCSV(csv);
    fclose(csv);

    // crear socket tcp
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    // permite reusar el puerto si el servidor se reinicia rapido
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // configurar en que puerto escucha
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PUERTO);
    addr.sin_addr.s_addr = INADDR_ANY; // acepta conexiones de cualquier ip

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    // empezar a escuchar, maximo MAX_CLIENTES en cola
    if (listen(srv, MAX_CLIENTES) < 0) {
        perror("listen"); return 1;
    }

    signal(SIGCHLD, sigchld); // registrar funcion para recoger hijos

    // bucle principal: esperar clientes y crear un hijo por cada uno
    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);
        int cli = accept(srv, (struct sockaddr*)&cli_addr, &len);
        if (cli < 0) continue;

        // obtener la ip del cliente en texto
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));

        pid_t hijo = fork(); // duplicar el proceso

        if (hijo == 0) {
            // soy el hijo: atiendo a este cliente y termino
            close(srv);
            atenderCliente(cli, ip);
            liberarTabla();
            exit(0);
        } else {
            // soy el padre: cierro este cliente y sigo esperando mas
            close(cli);
        }
    }

    close(srv);
    liberarTabla();
    return 0;
}
