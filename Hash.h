#ifndef HASH_H
#define HASH_H

#define TABLE_SIZE 1000
#define MAX_NAME 256
#define MAX_ALBUM 256

typedef struct Cancion {
    char nombre[MAX_NAME];
    char album[MAX_ALBUM];
    long pos_original;
    struct Cancion *siguiente;
} Cancion;

typedef struct {
    Cancion *tabla[TABLE_SIZE];
} TablaHash;

// Funciones principales
unsigned int Hash(const char *str);
void inicializarTabla(TablaHash *t);
void insertarCancion(TablaHash *t, const char *nombre, const char *album, long pos);
void guardarTabla(TablaHash *t, const char *nombreArchivo);
void liberarTabla(TablaHash *t);

// Funciones de búsqueda
Cancion *buscarPorNombre(TablaHash *t, const char *nombre);
void listarPorAlbum(TablaHash *t, const char *album);

#endif

