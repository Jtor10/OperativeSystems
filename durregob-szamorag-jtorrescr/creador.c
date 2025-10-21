#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define HASH_SIZE 1000
#define MAX_TITLE 256
#define MAX_ARTIST 256
#define MAX_ALBUM 256
#define MAX_LINE 1024

typedef struct Song {
    char id[64];
    char name[MAX_TITLE];
    char album[MAX_ALBUM];
    char artists[MAX_ARTIST];
    int year;
    int duration_ms;
    double danceability;
    double energy;
    double tempo;
    long next;
} Song;

typedef struct HashEntry {
    long first_position;
} HashEntry;

// Función hash mejorada para nombres de canciones
int hash_function(const char *name) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = tolower(*name++))) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash % HASH_SIZE;
}

// Función para eliminar comillas externas y limpiar el campo
void clean_field(char *field) {
    int len = strlen(field);
    
    // Eliminar comillas al inicio y final si existen
    if (len >= 2 && field[0] == '"' && field[len-1] == '"') {
        memmove(field, field + 1, len - 2);
        field[len - 2] = '\0';
        len -= 2;
    }
    
    // Eliminar espacios en blanco al inicio y final
    while (len > 0 && isspace((unsigned char)field[0])) {
        memmove(field, field + 1, len);
        len--;
    }
    
    while (len > 0 && isspace((unsigned char)field[len-1])) {
        field[len-1] = '\0';
        len--;
    }
}

// Función para parsear línea CSV con campos entre comillas
int parse_csv_line(char *line, char *fields[], int max_fields) {
    int field_count = 0;
    char *ptr = line;
    bool in_quotes = false;
    char *field_start = ptr;
    
    while (*ptr && field_count < max_fields) {
        if (*ptr == '"') {
            in_quotes = !in_quotes;
        } else if (*ptr == ',' && !in_quotes) {
            // Fin del campo
            *ptr = '\0';
            clean_field(field_start);
            fields[field_count++] = field_start;
            field_start = ptr + 1;
        }
        ptr++;
    }
    
    // Último campo
    if (field_count < max_fields) {
        clean_field(field_start);
        fields[field_count++] = field_start;
    }
    
    return field_count;
}

// Crear archivo binario inicial
void create_binary_file(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Error creando archivo binario\n");
        return;
    }
    
    HashEntry hash_table[HASH_SIZE];
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_table[i].first_position = -1;
    }
    
    size_t written = fwrite(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (written != HASH_SIZE) {
        printf("Error escribiendo tabla hash\n");
    }
    
    fclose(file);
    printf("Archivo binario creado: %s\n", filename);
}

// Agregar canción al archivo
void add_song(const char *filename, const char *id, const char *name, 
              const char *album, const char *artists, int year, 
              int duration_ms, double danceability, double energy, double tempo) {
    FILE *file = fopen(filename, "r+b");
    if (!file) {
        printf("Error abriendo archivo\n");
        return;
    }
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int hash_index = hash_function(name);
    
    Song new_song;
    strncpy(new_song.id, id, sizeof(new_song.id) - 1);
    new_song.id[sizeof(new_song.id) - 1] = '\0';
    
    strncpy(new_song.name, name, sizeof(new_song.name) - 1);
    new_song.name[sizeof(new_song.name) - 1] = '\0';
    
    strncpy(new_song.album, album, sizeof(new_song.album) - 1);
    new_song.album[sizeof(new_song.album) - 1] = '\0';
    
    strncpy(new_song.artists, artists, sizeof(new_song.artists) - 1);
    new_song.artists[sizeof(new_song.artists) - 1] = '\0';
    
    new_song.year = year;
    new_song.duration_ms = duration_ms;
    new_song.danceability = danceability;
    new_song.energy = energy;
    new_song.tempo = tempo;
    new_song.next = hash_table[hash_index].first_position;
    
    fseek(file, 0, SEEK_END);
    long new_position = ftell(file);
    
    size_t written = fwrite(&new_song, sizeof(Song), 1, file);
    if (written != 1) {
        printf("Error escribiendo canción\n");
        fclose(file);
        return;
    }
    
    hash_table[hash_index].first_position = new_position;
    
    fseek(file, 0, SEEK_SET);
    written = fwrite(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (written != HASH_SIZE) {
        printf("Error actualizando tabla hash\n");
    }
    
    fclose(file);
}

// Función para cargar canciones desde el archivo CSV específico
void load_songs_from_csv(const char *csv_filename, const char *bin_filename) {
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        printf("Error abriendo archivo CSV: %s\n", csv_filename);
        return;
    }
    
    char line[MAX_LINE];
    int count = 0;
    int error_count = 0;
    
    // Leer y saltar cabecera
    if (!fgets(line, sizeof(line), csv_file)) {
        printf("Error leyendo cabecera del CSV\n");
        fclose(csv_file);
        return;
    }
    
    printf("Procesando archivo CSV...\n");
    
    while (fgets(line, sizeof(line), csv_file)) {
        // Eliminar newline al final
        line[strcspn(line, "\n")] = 0;
        
        // Array para almacenar los campos parseados
        char *fields[25]; // Según la estructura proporcionada
        
        int field_count = parse_csv_line(line, fields, 25);
        
        if (field_count >= 24) { // Deberíamos tener al menos 24 campos
            char *id = fields[0];
            char *name = fields[1];
            char *album = fields[2];
            char *artists = fields[4]; // artists está en posición 4
            char *year_str = fields[23];
            char *duration_str = fields[20];
            char *danceability_str = fields[9];
            char *energy_str = fields[10];
            char *tempo_str = fields[19];
            
            // Convertir campos numéricos
            int year = atoi(year_str);
            int duration_ms = atoi(duration_str);
            double danceability = atof(danceability_str);
            double energy = atof(energy_str);
            double tempo = atof(tempo_str);
            
            // Validar datos básicos
            if (strlen(name) > 0 && year > 0) {
                add_song(bin_filename, id, name, album, artists, year, 
                        duration_ms, danceability, energy, tempo);
                count++;
                
                if (count % 10000 == 0) {
                    printf("Procesadas %d canciones...\n", count);
                }
            } else {
                error_count++;
            }
        } else {
            error_count++;
            if (error_count <= 10) { // Mostrar solo los primeros 10 errores
                printf("Error parseando línea (solo %d campos): %s\n", field_count, line);
            }
        }
    }
    
    fclose(csv_file);
    printf("\n=== RESUMEN DE CARGA ===\n");
    printf("Canciones procesadas exitosamente: %d\n", count);
    printf("Líneas con errores: %d\n", error_count);
}

// Función para mostrar estadísticas del hash
void show_hash_stats(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo\n");
        return;
    }
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int collisions = 0;
    int empty_buckets = 0;
    int max_chain = 0;
    int total_songs = 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        if (hash_table[i].first_position == -1) {
            empty_buckets++;
        } else {
            int chain_length = 0;
            long current_pos = hash_table[i].first_position;
            
            while (current_pos != -1) {
                chain_length++;
                total_songs++;
                fseek(file, current_pos, SEEK_SET);
                Song song;
                size_t items_read = fread(&song, sizeof(Song), 1, file);
                if (items_read != 1) {
                    printf("Error leyendo canción en posición %ld\n", current_pos);
                    break;
                }
                current_pos = song.next;
            }
            
            if (chain_length > 1) {
                collisions += (chain_length - 1);
            }
            
            if (chain_length > max_chain) {
                max_chain = chain_length;
            }
        }
    }
    
    printf("\n=== ESTADÍSTICAS DE HASH ===\n");
    printf("Total de canciones: %d\n", total_songs);
    printf("Total de buckets: %d\n", HASH_SIZE);
    printf("Buckets vacíos: %d (%.2f%%)\n", empty_buckets, 
           (empty_buckets * 100.0) / HASH_SIZE);
    printf("Colisiones totales: %d\n", collisions);
    printf("Longitud máxima de cadena: %d\n", max_chain);
    printf("Factor de carga: %.2f%%\n", 
           ((HASH_SIZE - empty_buckets) * 100.0) / HASH_SIZE);
    printf("Promedio de elementos por bucket no vacío: %.2f\n", 
           (float)total_songs / (HASH_SIZE - empty_buckets));
    
    fclose(file);
}

int main() {
    const char *bin_filename = "songs_database.bin";
    const char *csv_filename = "tracks_features.csv";
    
    printf("=== CREADOR DE BASE DE DATOS DE CANCIONES ===\n");
    
    // Verificar si el archivo CSV existe
    FILE *test_csv = fopen(csv_filename, "r");
    if (!test_csv) {
        printf("Error: No se encuentra el archivo %s\n", csv_filename);
        printf("Asegúrese de que el archivo CSV esté en el mismo directorio.\n");
        return 1;
    }
    fclose(test_csv);
    
    // Crear archivo binario
    create_binary_file(bin_filename);
    
    // Cargar canciones desde CSV
    printf("Cargando canciones desde: %s\n", csv_filename);
    load_songs_from_csv(csv_filename, bin_filename);
    
    // Mostrar estadísticas
    show_hash_stats(bin_filename);
    
    printf("\nBase de datos creada exitosamente en: %s\n", bin_filename);
    
    return 0;
}
