#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define HASH_SIZE 1000
#define MAX_TITLE 256
#define MAX_ARTIST 256
#define MAX_ALBUM 256

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

// Función hash (debe ser la misma que en el creador)
int hash_function(const char *name) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = tolower(*name++))) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % HASH_SIZE;
}

// Función para formatear duración de ms a minutos:segundos
void format_duration(int duration_ms, char *buffer, size_t buffer_size) {
    int total_seconds = duration_ms / 1000;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    snprintf(buffer, buffer_size, "%d:%02d", minutes, seconds);
}

// Función para buscar por nombre exacto
void search_by_exact_name(const char *filename, const char *name) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo: %s\n", filename);
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
    long current_pos = hash_table[hash_index].first_position;
    int found = 0;
    
    printf("\n=== BUSCANDO: '%s' (Hash: %d) ===\n", name, hash_index);
    
    while (current_pos != -1) {
        fseek(file, current_pos, SEEK_SET);
        Song song;
        size_t items_read = fread(&song, sizeof(Song), 1, file);
        if (items_read != 1) {
            printf("Error leyendo canción en posición %ld\n", current_pos);
            break;
        }
        
        if (strcasecmp(song.name, name) == 0) {
            char duration_str[20];
            format_duration(song.duration_ms, duration_str, sizeof(duration_str));
            
            printf("\n★ Canción encontrada ★\n");
            printf("ID: %s\n", song.id);
            printf("Nombre: %s\n", song.name);
            printf("Artista(s): %s\n", song.artists);
            printf("Álbum: %s\n", song.album);
            printf("Año: %d\n", song.year);
            printf("Duración: %s\n", duration_str);
            printf("Bailabilidad: %.3f\n", song.danceability);
            printf("Energía: %.3f\n", song.energy);
            printf("Tempo: %.1f BPM\n", song.tempo);
            printf("---\n");
            found++;
        }
        
        current_pos = song.next;
    }
    
    if (!found) {
        printf("No se encontró la canción: %s\n", name);
    } else {
        printf("Total encontradas: %d\n", found);
    }
    
    fclose(file);
}

// Función para buscar por palabra en el nombre
void search_by_name_word(const char *filename, const char *word) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo: %s\n", filename);
        return;
    }
    
    printf("\n=== BUSCANDO PALABRA: '%s' ===\n", word);
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int found = 0;
    char lower_word[MAX_TITLE];
    
    // Convertir palabra de búsqueda a minúsculas
    strncpy(lower_word, word, sizeof(lower_word) - 1);
    lower_word[sizeof(lower_word) - 1] = '\0';
    for (int i = 0; lower_word[i]; i++) {
        lower_word[i] = tolower(lower_word[i]);
    }
    
    for (int i = 0; i < HASH_SIZE; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            size_t items_read = fread(&song, sizeof(Song), 1, file);
            if (items_read != 1) {
                printf("Error leyendo canción en posición %ld\n", current_pos);
                break;
            }
            
            // Convertir nombre de canción a minúsculas para comparación
            char lower_name[MAX_TITLE];
            strncpy(lower_name, song.name, sizeof(lower_name) - 1);
            lower_name[sizeof(lower_name) - 1] = '\0';
            for (int j = 0; lower_name[j]; j++) {
                lower_name[j] = tolower(lower_name[j]);
            }
            
            if (strstr(lower_name, lower_word) != NULL) {
                char duration_str[20];
                format_duration(song.duration_ms, duration_str, sizeof(duration_str));
                
                printf("\n%d. %s - %s\n", ++found, song.name, song.artists);
                printf("   Álbum: %s | Año: %d | Duración: %s\n", 
                       song.album, song.year, duration_str);
                printf("   Bailabilidad: %.3f | Energía: %.3f | Tempo: %.1f BPM\n",
                       song.danceability, song.energy, song.tempo);
            }
            
            current_pos = song.next;
        }
    }
    
    if (!found) {
        printf("No se encontraron canciones con la palabra: %s\n", word);
    } else {
        printf("\nTotal encontradas: %d\n", found);
    }
    
    fclose(file);
}

// Función para buscar por artista
void search_by_artist(const char *filename, const char *artist) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo: %s\n", filename);
        return;
    }
    
    printf("\n=== BUSCANDO ARTISTA: '%s' ===\n", artist);
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int found = 0;
    char lower_artist[MAX_ARTIST];
    
    // Convertir artista de búsqueda a minúsculas
    strncpy(lower_artist, artist, sizeof(lower_artist) - 1);
    lower_artist[sizeof(lower_artist) - 1] = '\0';
    for (int i = 0; lower_artist[i]; i++) {
        lower_artist[i] = tolower(lower_artist[i]);
    }
    
    for (int i = 0; i < HASH_SIZE; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            size_t items_read = fread(&song, sizeof(Song), 1, file);
            if (items_read != 1) {
                printf("Error leyendo canción en posición %ld\n", current_pos);
                break;
            }
            
            // Convertir artistas a minúsculas para comparación
            char lower_song_artists[MAX_ARTIST];
            strncpy(lower_song_artists, song.artists, sizeof(lower_song_artists) - 1);
            lower_song_artists[sizeof(lower_song_artists) - 1] = '\0';
            for (int j = 0; lower_song_artists[j]; j++) {
                lower_song_artists[j] = tolower(lower_song_artists[j]);
            }
            
            if (strstr(lower_song_artists, lower_artist) != NULL) {
                char duration_str[20];
                format_duration(song.duration_ms, duration_str, sizeof(duration_str));
                
                printf("\n%d. %s - %s\n", ++found, song.name, song.artists);
                printf("   Álbum: %s | Año: %d | Duración: %s\n", 
                       song.album, song.year, duration_str);
            }
            
            current_pos = song.next;
        }
    }
    
    if (!found) {
        printf("No se encontraron canciones del artista: %s\n", artist);
    } else {
        printf("\nTotal encontradas: %d\n", found);
    }
    
    fclose(file);
}

// Función para buscar por año
void search_by_year(const char *filename, int year) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo: %s\n", filename);
        return;
    }
    
    printf("\n=== BUSCANDO AÑO: %d ===\n", year);
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int found = 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            size_t items_read = fread(&song, sizeof(Song), 1, file);
            if (items_read != 1) {
                printf("Error leyendo canción en posición %ld\n", current_pos);
                break;
            }
            
            if (song.year == year) {
                char duration_str[20];
                format_duration(song.duration_ms, duration_str, sizeof(duration_str));
                
                printf("\n%d. %s - %s\n", ++found, song.name, song.artists);
                printf("   Álbum: %s | Duración: %s\n", song.album, duration_str);
            }
            
            current_pos = song.next;
        }
    }
    
    if (!found) {
        printf("No se encontraron canciones del año: %d\n", year);
    } else {
        printf("\nTotal encontradas: %d\n", found);
    }
    
    fclose(file);
}

// Mostrar estadísticas rápidas
void show_stats(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error abriendo archivo: %s\n", filename);
        return;
    }
    
    HashEntry hash_table[HASH_SIZE];
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    if (read_items != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(file);
        return;
    }
    
    int total_songs = 0;
    int min_year = 3000, max_year = 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1) {
            total_songs++;
            fseek(file, current_pos, SEEK_SET);
            Song song;
            size_t items_read = fread(&song, sizeof(Song), 1, file);
            if (items_read != 1) {
                printf("Error leyendo canción en posición %ld\n", current_pos);
                break;
            }
            
            if (song.year < min_year) min_year = song.year;
            if (song.year > max_year) max_year = song.year;
            
            current_pos = song.next;
        }
    }
    
    printf("\n=== ESTADÍSTICAS DE LA BASE DE DATOS ===\n");
    printf("Total de canciones: %d\n", total_songs);
    printf("Rango de años: %d - %d\n", min_year, max_year);
    printf("Tamaño de la tabla hash: %d\n", HASH_SIZE);
    
    fclose(file);
}

// Menú interactivo
void show_menu() {
    printf("\n=== BUSCADOR DE CANCIONES ===\n");
    printf("1. Buscar por nombre exacto\n");
    printf("2. Buscar por palabra en el nombre\n");
    printf("3. Buscar por artista\n");
    printf("4. Buscar por año\n");
    printf("5. Mostrar estadísticas\n");
    printf("6. Salir\n");
    printf("Seleccione una opción: ");
}

int main() {
    const char *filename = "songs_database.bin";
    int option;
    char search_term[256];
    int search_year;
    
    FILE *test_file = fopen(filename, "rb");
    if (!test_file) {
        printf("Error: No se encuentra el archivo %s\n", filename);
        printf("Ejecute primero el programa creador de la base de datos.\n");
        return 1;
    }
    fclose(test_file);
    
    printf("Buscador de Canciones - Base de datos cargada\n");
    
    do {
        show_menu();
        if (scanf("%d", &option) != 1) {
            printf("Entrada inválida\n");
            while (getchar() != '\n'); // Limpiar buffer
            continue;
        }
        getchar(); // Limpiar newline
        
        switch (option) {
            case 1:
                printf("Ingrese el nombre exacto de la canción: ");
                fgets(search_term, sizeof(search_term), stdin);
                search_term[strcspn(search_term, "\n")] = 0;
                if (strlen(search_term) > 0) {
                    search_by_exact_name(filename, search_term);
                }
                break;
                
            case 2:
                printf("Ingrese palabra a buscar en nombres: ");
                fgets(search_term, sizeof(search_term), stdin);
                search_term[strcspn(search_term, "\n")] = 0;
                if (strlen(search_term) > 0) {
                    search_by_name_word(filename, search_term);
                }
                break;
                
            case 3:
                printf("Ingrese nombre del artista: ");
                fgets(search_term, sizeof(search_term), stdin);
                search_term[strcspn(search_term, "\n")] = 0;
                if (strlen(search_term) > 0) {
                    search_by_artist(filename, search_term);
                }
                break;
                
            case 4:
                printf("Ingrese año a buscar: ");
                if (scanf("%d", &search_year) == 1) {
                    search_by_year(filename, search_year);
                } else {
                    printf("Año inválido\n");
                }
                getchar(); // Limpiar buffer
                break;
                
            case 5:
                show_stats(filename);
                break;
                
            case 6:
                printf("Saliendo...\n");
                break;
                
            default:
                printf("Opción no válida\n");
        }
        
    } while (option != 6);
    
    return 0;
}
