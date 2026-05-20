#ifndef HASH_H
#define HASH_H

#define TAM_TABLA 2003  /* tamanio de la tabla hash, numero primo */

/* cada nodo guarda la ciudad de origen y la posicion en el archivo */
typedef struct NodoHash {
    char origen[100];
    long offset;                /* posicion en bytes dentro del CSV */
    struct NodoHash* siguiente;
} NodoHash;

extern NodoHash* tabla[TAM_TABLA];

void  iniciarTabla();
int   calcularHash(char* clave);
void  insertarHash(char* origen, long offset);
NodoHash* buscarHash(char* origen);
void  liberarTabla();

#endif
