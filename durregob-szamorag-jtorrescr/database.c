#include "database.h"

// Función auxiliar para copiar string de forma segura - versión simplificada
void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest_size > 0) {
        // Copiar hasta dest_size-1 caracteres y agregar null terminator
        size_t i;
        for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }
}

// Función hash
int hash_function(const char *name) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = tolower(*name++))) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % HASH_SIZE;
}

// Limpiar campo
void clean_field(char *field) {
    int len = strlen(field);
    
    if (len >= 2 && field[0] == '"' && field[len-1] == '"') {
        memmove(field, field + 1, len - 2);
        field[len - 2] = '\0';
        len -= 2;
    }
    
    while (len > 0 && isspace((unsigned char)field[0])) {
        memmove(field, field + 1, len);
        len--;
    }
    
    while (len > 0 && isspace((unsigned char)field[len-1])) {
        field[len-1] = '\0';
        len--;
    }
}

// Parsear línea CSV
int parse_csv_line(char *line, char *fields[], int max_fields) {
    int field_count = 0;
    char *ptr = line;
    bool in_quotes = false;
    char *field_start = ptr;
    
    while (*ptr && field_count < max_fields) {
        if (*ptr == '"') {
            in_quotes = !in_quotes;
        } else if (*ptr == ',' && !in_quotes) {
            *ptr = '\0';
            clean_field(field_start);
            fields[field_count++] = field_start;
            field_start = ptr + 1;
        }
        ptr++;
    }
    
    if (field_count < max_fields) {
        clean_field(field_start);
        fields[field_count++] = field_start;
    }
    
    return field_count;
}

// Cargar canciones desde CSV (versión simplificada para el proyecto)
void load_songs_from_csv(const char *csv_filename, const char *bin_filename) {
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        printf("Error: No se puede abrir el archivo CSV %s\n", csv_filename);
        return;
    }
    
    char line[MAX_LINE];
    int count = 0;
    
    // Leer cabecera
    if (fgets(line, sizeof(line), csv_file) == NULL) {
        fclose(csv_file);
        return;
    }
    
    printf("Procesando CSV...\n");
    
    // Abrir archivo binario para lectura/escritura
    FILE *bin_file = fopen(bin_filename, "r+b");
    if (!bin_file) {
        printf("Error: No se puede abrir la base de datos\n");
        fclose(csv_file);
        return;
    }
    
    // Leer tabla hash existente
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, bin_file) != HASH_SIZE) {
        printf("Error leyendo tabla hash\n");
        fclose(csv_file);
        fclose(bin_file);
        return;
    }
    
    // Procesar cada línea del CSV
    while (fgets(line, sizeof(line), csv_file) != NULL && count < 50000) { // Límite para prueba
        line[strcspn(line, "\n")] = 0;
        char *fields[25];
        
        int field_count = parse_csv_line(line, fields, 25);
        
        if (field_count >= 24) {
            char *id = fields[0];
            char *name = fields[1];
            char *album = fields[2];
            char *artists = fields[4];
            char *year_str = fields[23];
            char *duration_str = fields[20];
            char *danceability_str = fields[9];
            char *energy_str = fields[10];
            char *tempo_str = fields[19];
            
            int year = atoi(year_str);
            int duration_ms = atoi(duration_str);
            double danceability = atof(danceability_str);
            double energy = atof(energy_str);
            double tempo = atof(tempo_str);
            
            if (strlen(name) > 0 && year > 0) {
                int hash_index = hash_function(name);
                
                Song new_song;
                safe_strcpy(new_song.id, id, sizeof(new_song.id));
                safe_strcpy(new_song.name, name, sizeof(new_song.name));
                safe_strcpy(new_song.album, album, sizeof(new_song.album));
                safe_strcpy(new_song.artists, artists, sizeof(new_song.artists));
                new_song.year = year;
                new_song.duration_ms = duration_ms;
                new_song.danceability = danceability;
                new_song.energy = energy;
                new_song.tempo = tempo;
                new_song.next = hash_table[hash_index].first_position;
                
                // Escribir canción al final del archivo
                fseek(bin_file, 0, SEEK_END);
                long new_position = ftell(bin_file);
                
                if (fwrite(&new_song, sizeof(Song), 1, bin_file) == 1) {
                    // Actualizar tabla hash
                    hash_table[hash_index].first_position = new_position;
                    count++;
                }
            }
        }
        
        if (count % 10000 == 0) {
            printf("Procesadas %d canciones...\n", count);
        }
    }
    
    // Escribir tabla hash actualizada
    fseek(bin_file, 0, SEEK_SET);
    fwrite(hash_table, sizeof(HashEntry), HASH_SIZE, bin_file);
    
    fclose(bin_file);
    fclose(csv_file);
    
    printf("Carga completada: %d canciones procesadas\n", count);
}

// Función auxiliar para leer tabla hash
static int read_hash_table(FILE *file, HashEntry *hash_table) {
    size_t read_items = fread(hash_table, sizeof(HashEntry), HASH_SIZE, file);
    return (read_items == HASH_SIZE) ? 0 : -1;
}

// Función auxiliar para leer una canción
static int read_song(FILE *file, long position, Song *song) {
    if (position == -1) return -1;
    fseek(file, position, SEEK_SET);
    size_t items_read = fread(song, sizeof(Song), 1, file);
    return (items_read == 1) ? 0 : -1;
}

// Búsqueda por nombre exacto
int search_by_exact_name(const char *filename, const char *name, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int hash_index = hash_function(name);
    long current_pos = hash_table[hash_index].first_position;
    int found = 0;
    
    while (current_pos != -1 && found < max_results) {
        Song song;
        if (read_song(file, current_pos, &song) != 0) {
            break;
        }
        
        if (strcasecmp(song.name, name) == 0) {
            results[found++] = song;
        }
        
        current_pos = song.next;
    }
    
    fclose(file);
    return found;
}

// Búsqueda por palabra en nombre
int search_by_name_word(const char *filename, const char *word, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    char lower_word[MAX_TITLE];
    safe_strcpy(lower_word, word, sizeof(lower_word));
    for (int i = 0; lower_word[i]; i++) {
        lower_word[i] = tolower(lower_word[i]);
    }
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            Song song;
            if (read_song(file, current_pos, &song) != 0) {
                break;
            }
            
            char lower_name[MAX_TITLE];
            safe_strcpy(lower_name, song.name, sizeof(lower_name));
            for (int j = 0; lower_name[j]; j++) {
                lower_name[j] = tolower(lower_name[j]);
            }
            
            if (strstr(lower_name, lower_word) != NULL) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}

// Búsqueda por artista
int search_by_artist(const char *filename, const char *artist, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    char lower_artist[MAX_ARTIST];
    safe_strcpy(lower_artist, artist, sizeof(lower_artist));
    for (int i = 0; lower_artist[i]; i++) {
        lower_artist[i] = tolower(lower_artist[i]);
    }
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            Song song;
            if (read_song(file, current_pos, &song) != 0) {
                break;
            }
            
            char lower_song_artists[MAX_ARTIST];
            safe_strcpy(lower_song_artists, song.artists, sizeof(lower_song_artists));
            for (int j = 0; lower_song_artists[j]; j++) {
                lower_song_artists[j] = tolower(lower_song_artists[j]);
            }
            
            if (strstr(lower_song_artists, lower_artist) != NULL) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}

// Búsqueda por año
int search_by_year(const char *filename, int year, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            Song song;
            if (read_song(file, current_pos, &song) != 0) {
                break;
            }
            
            if (song.year == year) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}

// Búsqueda por rango de bailabilidad
int search_by_danceability_range(const char *filename, double min_val, double max_val, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            Song song;
            if (read_song(file, current_pos, &song) != 0) {
                break;
            }
            
            if (song.danceability >= min_val && song.danceability <= max_val) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}

// Búsqueda por rango de energía
int search_by_energy_range(const char *filename, double min_val, double max_val, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0;
    }
    
    HashEntry hash_table[HASH_SIZE];
    if (read_hash_table(file, hash_table) != 0) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            Song song;
            if (read_song(file, current_pos, &song) != 0) {
                break;
            }
            
            if (song.energy >= min_val && song.energy <= max_val) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}
