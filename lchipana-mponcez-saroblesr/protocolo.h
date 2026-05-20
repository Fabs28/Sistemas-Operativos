#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#define PUERTO         8080
#define MAX_CLIENTES   32

#define OP_ORIGEN      1
#define OP_DESTINO     2
#define OP_TIPODEVUELO 3
#define OP_BUSCAR      4
#define OP_SALIR       5

typedef struct {
    int  opcion;
    char origen[100];
    char destino[100];
    char tipodevuelo[20];
} Peticion;

typedef struct {
    int   encontrado;
    float tiempoMedio;
    int   cantVuelos;
    char  mensaje[256];
} Respuesta;

#endif
