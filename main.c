#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Hash.h"

#define MAX_LINE 16384  // el texto puede ser muy largo
#define MAX_COLS 40

// Función que separa una línea CSV respetando comas dentro de comillas
int splitCSVLine(char *line, char *columns[], int maxCols) {
    int count = 0;
    int inQuotes = 0;
    char *ptr = line;
    columns[count++] = ptr;

    while (*ptr && count < maxCols) {
        if (*ptr == '"') {
            inQuotes = !inQuotes;
        } else if (*ptr == ',' && !inQuotes) {
            *ptr = '\0';
            if (count < maxCols)
                columns[count++] = ptr + 1;
        }
        ptr++;
    }
    return count;
}

int main() {
    FILE *csv = fopen("/home/jepherson/Descargas/tracks_features.csv", "r");
    if (!csv) {
        perror("Error abriendo el archivo CSV");
        return 1;
    }

    TablaHash tablaPorNombre;
    TablaHash tablaPorAlbum;
    inicializarTabla(&tablaPorNombre);
    inicializarTabla(&tablaPorAlbum);

    char linea[MAX_LINE];
    fgets(linea, sizeof(linea), csv); // Saltar cabecera

    long numLineas = 0;
    while (fgets(linea, sizeof(linea), csv)) {
        numLineas++;
        long pos = ftell(csv);
        linea[strcspn(linea, "\n")] = '\0';

        if (strlen(linea) < 5) continue;

        char *columnas[MAX_COLS] = {0};
        int nCols = splitCSVLine(linea, columnas, MAX_COLS);

        if (nCols < 7) continue; // línea incompleta

        // columnas importantes
        char *song = columnas[1];   // song
        char *album = columnas[6];  // Album

        if (song && album && strlen(song) > 0 && strlen(album) > 0) {
            insertarCancion(&tablaPorNombre, song, album, pos);
            insertarCancion(&tablaPorAlbum, album, song, pos);
        }

        if (numLineas % 10000 == 0)
            printf("Procesadas %ld líneas...\n", numLineas);
    }

    fclose(csv);

    guardarTabla(&tablaPorNombre, "hash_por_nombre.csv");
    guardarTabla(&tablaPorAlbum, "hash_por_album.csv");

    printf("\n Tablas hash creadas correctamente.\n");
    printf("   - hash_por_nombre.csv\n");
    printf("   - hash_por_album.csv\n\n");

    liberarTabla(&tablaPorNombre);
    liberarTabla(&tablaPorAlbum);
    return 0;
}

