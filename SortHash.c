#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Hash.h"

#define MAX_LINE 512
#define MAX_NAME 128
#define CHUNK_SIZE 5000     
#define TABLE_SIZE 1000

typedef struct Cancion {
    int hash;
    char nombre[MAX_NAME];
    char album[MAX_NAME];
    long pos_original;
} Cancion;

typedef struct NodoStack {
    int inicio;
    int fin;
    int estado; // 0 = procesar, 1 = escribir
    long pos;
} NodoStack;

// --- Construcción del árbol balanceado ---
long construir_balanceado(Cancion *arr, int inicio, int fin, FILE *f) {
    if (inicio > fin) return -1;

    int capacidad = fin - inicio + 1;
    NodoStack *stack = malloc(sizeof(NodoStack) * capacidad);
    if (!stack) {
        fprintf(stderr, "Error: no hay memoria para pila (%d nodos).\n", capacidad);
        exit(1);
    }

    int top = 0;
    stack[top++] = (NodoStack){inicio, fin, 0, -1};
    long rootPos = -1;

    while (top > 0) {
        NodoStack nodo = stack[--top];
        int mid = (nodo.inicio + nodo.fin) / 2;

        if (nodo.estado == 0) {
            stack[top++] = (NodoStack){nodo.inicio, nodo.fin, 1, -1};
            if (mid + 1 <= nodo.fin)
                stack[top++] = (NodoStack){mid + 1, nodo.fin, 0, -1};
            if (nodo.inicio <= mid - 1)
                stack[top++] = (NodoStack){nodo.inicio, mid - 1, 0, -1};
        } else {
            long pos_actual = ftell(f);
            fwrite(&arr[mid], sizeof(Cancion), 1, f);
            if (rootPos == -1)
                rootPos = pos_actual;
        }
    }

    free(stack);
    return rootPos;
}

int comparar_canciones(const void *a, const void *b) {
    return strcmp(((Cancion *)a)->nombre, ((Cancion *)b)->nombre);
}

// --- Procesamiento por bloques ---
void procesar_archivo(const char *archivo_entrada,
                      const char *archivo_tree,
                      const char *archivo_hashmap) {

    printf("[INFO] Procesando: %s\n", archivo_entrada);

    remove(archivo_tree);
    remove(archivo_hashmap);

    FILE *f_csv = fopen(archivo_entrada, "r");
    if (!f_csv) {
        perror("Error al abrir CSV de entrada");
        return;
    }

    FILE *f_tree = fopen(archivo_tree, "ab+");
    FILE *f_hash = fopen(archivo_hashmap, "ab+");
    if (!f_tree || !f_hash) {
        perror("Error creando archivos de salida");
        fclose(f_csv);
        return;
    }

    Cancion *lote = malloc(sizeof(Cancion) * CHUNK_SIZE);
    if (!lote) {
        fprintf(stderr, "Error: no se pudo reservar memoria para el lote.\n");
        fclose(f_csv);
        fclose(f_tree);
        fclose(f_hash);
        return;
    }

    int totalCanciones = 0;
    int hashesUsados = 0;
    char linea[MAX_LINE];

    // --- Procesar el archivo en lotes ---
    while (!feof(f_csv)) {
        int n = 0;
        while (n < CHUNK_SIZE && fgets(linea, sizeof(linea), f_csv)) {
            char nombre[MAX_NAME], album[MAX_NAME];
            if (sscanf(linea, "%127[^,],%127[^,\n]", nombre, album) == 2) {
                strcpy(lote[n].nombre, nombre);
                strcpy(lote[n].album, album);
                lote[n].hash = Hash(nombre) % TABLE_SIZE;
                lote[n].pos_original = totalCanciones + n;
                n++;
            }
        }

        if (n == 0) break; // Fin del archivo
        totalCanciones += n;

        // --- Crear árboles por hash dentro del lote ---
        for (int i = 0; i < TABLE_SIZE; i++) {
            int count = 0;
            for (int j = 0; j < n; j++) {
                if (lote[j].hash == i)
                    count++;
            }

            if (count == 0)
                continue;

            Cancion *grupo = malloc(sizeof(Cancion) * count);
            if (!grupo) {
                fprintf(stderr, "Error: memoria insuficiente para hash %d.\n", i);
                continue;
            }

            int idx = 0;
            for (int j = 0; j < n; j++) {
                if (lote[j].hash == i)
                    grupo[idx++] = lote[j];
            }

            qsort(grupo, count, sizeof(Cancion), comparar_canciones);
            long root = construir_balanceado(grupo, 0, count - 1, f_tree);
            fprintf(f_hash, "%d,%ld\n", i, root);

            free(grupo);
            hashesUsados++;
        }
    }

    fclose(f_csv);
    fclose(f_tree);
    fclose(f_hash);
    free(lote);

    printf("[OK] %s procesado (%d hashes usados, %d canciones)\n\n",
           archivo_entrada, hashesUsados, totalCanciones);
}

int main() {
    printf("=== Construcción de árboles para tablas hash (por lotes de 5000 canciones) ===\n\n");

    procesar_archivo("hash_por_nombre.csv",
                     "result/tree_por_nombre.csv",
                     "result/hashmap_por_nombre.csv");

    procesar_archivo("hash_por_album.csv",
                     "result/tree_por_album.csv",
                     "result/hashmap_por_album.csv");

    printf("Finalizado correctamente.\n");
    return 0;
}

