#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

NodoHash* tabla[TAM_TABLA];

/* pone todos los cajones de la tabla en NULL */
void iniciarTabla() {
    int i;
    for (i = 0; i < TAM_TABLA; i++)
        tabla[i] = NULL;
}

/* convierte una cadena en un numero de cason (indice) */
int calcularHash(char* clave) {
    unsigned long h = 5381;
    int c;
    while ((c = *clave++))
        h = h * 33 + c;
    return (int)(h % TAM_TABLA);
}

/* guarda origen + posicion en disco en la tabla */
void insertarHash(char* origen, long offset) {
    int idx = calcularHash(origen);

    NodoHash* nuevo = (NodoHash*)malloc(sizeof(NodoHash));
    if (!nuevo) return;

    strncpy(nuevo->origen, origen, sizeof(nuevo->origen) - 1);
    nuevo->origen[sizeof(nuevo->origen) - 1] = '\0';
    nuevo->offset    = offset;
    nuevo->siguiente = tabla[idx];
    tabla[idx]       = nuevo;
}

/* busca el primer nodo con ese origen exacto */
NodoHash* buscarHash(char* origen) {
    int idx = calcularHash(origen);
    NodoHash* actual = tabla[idx];
    while (actual) {
        if (strcmp(actual->origen, origen) == 0)
            return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

/* libera toda la memoria de la tabla */
void liberarTabla() {
    int i;
    for (i = 0; i < TAM_TABLA; i++) {
        NodoHash* actual = tabla[i];
        while (actual) {
            NodoHash* tmp = actual->siguiente;
            free(actual);
            actual = tmp;
        }
        tabla[i] = NULL;
    }
}
