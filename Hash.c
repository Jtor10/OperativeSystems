#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Hash.h"

unsigned int Hash(const char *str) {
    if (str == NULL) return 0;

    unsigned int hash = 0;
    int c;
    int count = 0;

    while ((c = *str++) != '\0' && count < 10) {
        hash = (hash * 10000007 + c) % TABLE_SIZE;
        count++;
    }

    return hash;
}


void inicializarTabla(TablaHash *t) {
    for (int i = 0; i < TABLE_SIZE; i++)
        t->tabla[i] = NULL;
}

void insertarCancion(TablaHash *t, const char *nombre, const char *album, long pos) {
    if (!nombre || !album) return;

    unsigned int idx = Hash(nombre);
    Cancion *nuevo = malloc(sizeof(Cancion));
    if (!nuevo) return;

    strncpy(nuevo->nombre, nombre, MAX_NAME);
    strncpy(nuevo->album, album, MAX_ALBUM);
    nuevo->pos_original = pos;
    nuevo->siguiente = t->tabla[idx];
    t->tabla[idx] = nuevo;
}

void guardarTabla(TablaHash *t, const char *nombreArchivo) {
    FILE *out = fopen(nombreArchivo, "w");
    if (!out) return;

    for (int i = 0; i < TABLE_SIZE; i++) {
        Cancion *actual = t->tabla[i];
        while (actual) {
            fprintf(out, "%d,%s,%s,%ld\n", i, actual->nombre, actual->album, actual->pos_original);
            actual = actual->siguiente;
        }
    }
    fclose(out);
}

void liberarTabla(TablaHash *t) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Cancion *actual = t->tabla[i];
        while (actual) {
            Cancion *temp = actual;
            actual = actual->siguiente;
            free(temp);
        }
    }
}

// Buscar una canción por nombre
Cancion *buscarPorNombre(TablaHash *t, const char *nombre) {
    unsigned int idx = Hash(nombre);
    Cancion *actual = t->tabla[idx];

    while (actual) {
        if (strcmp(actual->nombre, nombre) == 0)
            return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

// Listar todas las canciones de un álbum
void listarPorAlbum(TablaHash *t, const char *album) {
    unsigned int idx = Hash(album);
    Cancion *actual = t->tabla[idx];
    int encontrado = 0;

    while (actual) {
        if (strcmp(actual->album, album) == 0) {
            printf(" - %s\n", actual->nombre);
            encontrado = 1;
        }
        actual = actual->siguiente;
    }

    if (!encontrado)
        printf("No se encontraron canciones en el álbum '%s'.\n", album);
}
