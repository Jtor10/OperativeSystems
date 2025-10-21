#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>

#define HASH_SIZE 1000
#define MAX_TITLE 256
#define MAX_ARTIST 256
#define MAX_ALBUM 256
#define MAX_RESULTS 100
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678

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

// Estructura para memoria compartida
typedef struct {
    int search_type;
    char search_term[256];
    int search_year;
    int result_count;
    Song results[MAX_RESULTS];
    int request_ready;  // 0 = esperando, 1 = solicitud lista
    int response_ready; // 0 = procesando, 1 = respuesta lista
    int shutdown;       // 0 = ejecutando, 1 = terminar
} SharedData;

// Variables globales
int shm_id, sem_id;
SharedData *shared_data;
pid_t db_pid = -1;

// Operaciones sobre semáforos
void sem_wait(int sem_id) {
    struct sembuf op = {0, -1, 0};
    semop(sem_id, &op, 1);
}

void sem_signal(int sem_id) {
    struct sembuf op = {0, 1, 0};
    semop(sem_id, &op, 1);
}

// Función para limpiar recursos
void cleanup() {
    if (shared_data) {
        shared_data->shutdown = 1;
        shared_data->request_ready = 1; // Despertar al proceso BD
    }
    
    if (db_pid != -1) {
        kill(db_pid, SIGTERM);
        waitpid(db_pid, NULL, 0);
    }
    
    // Liberar memoria compartida y semáforos
    if (shm_id != -1) {
        shmdt(shared_data);
        shmctl(shm_id, IPC_RMID, NULL);
    }
    if (sem_id != -1) {
        semctl(sem_id, 0, IPC_RMID);
    }
}

// Manejador de señales
void signal_handler(int sig) {
    (void)sig;
    printf("\nFinalizando programa...\n");
    cleanup();
    exit(0);
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

// Función para formatear duración
void format_duration(int duration_ms, char *buffer, size_t buffer_size) {
    int total_seconds = duration_ms / 1000;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    snprintf(buffer, buffer_size, "%d:%02d", minutes, seconds);
}

// Función para mostrar resultados
void display_results() {
    if (shared_data->result_count == 0) {
        printf("NA - No se encontraron resultados\n");
        return;
    }
    
    printf("\n=== RESULTADOS ENCONTRADOS: %d ===\n", shared_data->result_count);
    
    for (int i = 0; i < shared_data->result_count && i < 10; i++) {
        Song *song = &shared_data->results[i];
        char duration_str[20];
        format_duration(song->duration_ms, duration_str, sizeof(duration_str));
        
        if (shared_data->result_count == 1) {
            printf("\n★ Canción encontrada ★\n");
            printf("ID: %s\n", song->id);
            printf("Nombre: %s\n", song->name);
            printf("Artista(s): %s\n", song->artists);
            printf("Álbum: %s\n", song->album);
            printf("Año: %d\n", song->year);
            printf("Duración: %s\n", duration_str);
            printf("Bailabilidad: %.3f\n", song->danceability);
            printf("Energía: %.3f\n", song->energy);
            printf("Tempo: %.1f BPM\n", song->tempo);
            printf("---\n");
        } else {
            printf("\n%d. %s - %s\n", i + 1, song->name, song->artists);
            printf("   Álbum: %s | Año: %d | Duración: %s\n", 
                   song->album, song->year, duration_str);
            if (i == 0) {
                printf("   Bailabilidad: %.3f | Energía: %.3f | Tempo: %.1f BPM\n",
                       song->danceability, song->energy, song->tempo);
            }
        }
    }
    
    if (shared_data->result_count > 10) {
        printf("\n... y %d resultados más\n", shared_data->result_count - 10);
    }
}

// Función segura para leer entrada
void safe_fgets(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
    } else {
        buffer[strcspn(buffer, "\n")] = 0;
    }
}

// Función segura para leer números
int safe_scanf_int(const char *format, int *value) {
    int result = scanf(format, value);
    while (getchar() != '\n');
    return result;
}

// Función para buscar por nombre exacto
int search_by_exact_name(const char *filename, const char *name, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, file) != HASH_SIZE) {
        fclose(file);
        return 0;
    }
    
    int hash_index = hash_function(name);
    long current_pos = hash_table[hash_index].first_position;
    int found = 0;
    
    while (current_pos != -1 && found < max_results) {
        fseek(file, current_pos, SEEK_SET);
        Song song;
        if (fread(&song, sizeof(Song), 1, file) != 1) break;
        
        if (strcasecmp(song.name, name) == 0) {
            results[found++] = song;
        }
        
        current_pos = song.next;
    }
    
    fclose(file);
    return found;
}

// Función para buscar por palabra en el nombre
int search_by_name_word(const char *filename, const char *word, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, file) != HASH_SIZE) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    char lower_word[MAX_TITLE];
    strncpy(lower_word, word, sizeof(lower_word) - 1);
    lower_word[sizeof(lower_word) - 1] = '\0';
    for (int i = 0; lower_word[i]; i++) {
        lower_word[i] = tolower(lower_word[i]);
    }
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            if (fread(&song, sizeof(Song), 1, file) != 1) break;
            
            char lower_name[MAX_TITLE];
            strncpy(lower_name, song.name, sizeof(lower_name) - 1);
            lower_name[sizeof(lower_name) - 1] = '\0';
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

// Función para buscar por artista
int search_by_artist(const char *filename, const char *artist, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, file) != HASH_SIZE) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    char lower_artist[MAX_ARTIST];
    strncpy(lower_artist, artist, sizeof(lower_artist) - 1);
    lower_artist[sizeof(lower_artist) - 1] = '\0';
    for (int i = 0; lower_artist[i]; i++) {
        lower_artist[i] = tolower(lower_artist[i]);
    }
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            if (fread(&song, sizeof(Song), 1, file) != 1) break;
            
            char lower_song_artists[MAX_ARTIST];
            strncpy(lower_song_artists, song.artists, sizeof(lower_song_artists) - 1);
            lower_song_artists[sizeof(lower_song_artists) - 1] = '\0';
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

// Función para buscar por año
int search_by_year(const char *filename, int year, Song *results, int max_results) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, file) != HASH_SIZE) {
        fclose(file);
        return 0;
    }
    
    int found = 0;
    
    for (int i = 0; i < HASH_SIZE && found < max_results; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1 && found < max_results) {
            fseek(file, current_pos, SEEK_SET);
            Song song;
            if (fread(&song, sizeof(Song), 1, file) != 1) break;
            
            if (song.year == year) {
                results[found++] = song;
            }
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return found;
}

// Función para mostrar estadísticas
int get_database_stats(const char *filename, int *total_songs, int *min_year, int *max_year) {
    FILE *file = fopen(filename, "rb");
    if (!file) return -1;
    
    HashEntry hash_table[HASH_SIZE];
    if (fread(hash_table, sizeof(HashEntry), HASH_SIZE, file) != HASH_SIZE) {
        fclose(file);
        return -1;
    }
    
    *total_songs = 0;
    *min_year = 3000;
    *max_year = 0;
    
    for (int i = 0; i < HASH_SIZE; i++) {
        long current_pos = hash_table[i].first_position;
        
        while (current_pos != -1) {
            (*total_songs)++;
            fseek(file, current_pos, SEEK_SET);
            Song song;
            if (fread(&song, sizeof(Song), 1, file) != 1) break;
            
            if (song.year < *min_year) *min_year = song.year;
            if (song.year > *max_year) *max_year = song.year;
            
            current_pos = song.next;
        }
    }
    
    fclose(file);
    return 0;
}

// Función para enviar solicitud y esperar respuesta
int send_search_request(int search_type, const char *search_term, int search_year) {
    // Preparar solicitud
    sem_wait(sem_id);
    
    shared_data->search_type = search_type;
    shared_data->search_year = search_year;
    if (search_term) {
        strncpy(shared_data->search_term, search_term, sizeof(shared_data->search_term) - 1);
        shared_data->search_term[sizeof(shared_data->search_term) - 1] = '\0';
    } else {
        shared_data->search_term[0] = '\0';
    }
    shared_data->response_ready = 0; // Respuesta no lista
    shared_data->request_ready = 1;  // Solicitud lista
    
    sem_signal(sem_id);
    
    // Esperar respuesta (polling eficiente)
    clock_t start = clock();
    
    while (shared_data->response_ready == 0) {
        if ((clock() - start) / CLOCKS_PER_SEC > 10) { // Timeout de 10 segundos
            printf("Error: Timeout en la búsqueda\n");
            return -1;
        }
        usleep(1000); // Solo 1ms de espera entre verificaciones
    }
    
    return 0;
}

// Proceso de interfaz de usuario
void user_interface_process() {
    printf("=== SISTEMA DE BÚSQUEDA DE CANCIONES ===\n");
    printf("Base de datos de música - Proceso de Interfaz\n\n");
    
    int option;
    char search_term[256];
    int search_year;
    
    do {
        printf("\n=== MENÚ PRINCIPAL ===\n");
        printf("1. Buscar por nombre exacto\n");
        printf("2. Buscar por palabra en el nombre\n");
        printf("3. Buscar por artista\n");
        printf("4. Buscar por año\n");
        printf("5. Mostrar estadísticas\n");
        printf("6. Salir\n");
        printf("Seleccione una opción: ");
        
        if (safe_scanf_int("%d", &option) != 1) {
            printf("Entrada inválida\n");
            continue;
        }
        
        clock_t start, end;
        double cpu_time_used;
        
        switch (option) {
            case 1:
                printf("Ingrese el nombre exacto de la canción: ");
                safe_fgets(search_term, sizeof(search_term));
                if (strlen(search_term) > 0) {
                    start = clock();
                    if (send_search_request(1, search_term, 0) == 0) {
                        end = clock();
                        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                        display_results();
                        printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                    }
                }
                break;
                
            case 2:
                printf("Ingrese palabra a buscar en nombres: ");
                safe_fgets(search_term, sizeof(search_term));
                if (strlen(search_term) > 0) {
                    start = clock();
                    if (send_search_request(2, search_term, 0) == 0) {
                        end = clock();
                        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                        display_results();
                        printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                    }
                }
                break;
                
            case 3:
                printf("Ingrese nombre del artista: ");
                safe_fgets(search_term, sizeof(search_term));
                if (strlen(search_term) > 0) {
                    start = clock();
                    if (send_search_request(3, search_term, 0) == 0) {
                        end = clock();
                        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                        display_results();
                        printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                    }
                }
                break;
                
            case 4:
                printf("Ingrese año a buscar: ");
                if (safe_scanf_int("%d", &search_year) == 1) {
                    start = clock();
                    if (send_search_request(4, NULL, search_year) == 0) {
                        end = clock();
                        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                        display_results();
                        printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                    }
                } else {
                    printf("Año inválido\n");
                }
                break;
                
            case 5:
                start = clock();
                if (send_search_request(5, NULL, 0) == 0) {
                    end = clock();
                    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                    printf("\n=== ESTADÍSTICAS DE LA BASE DE DATOS ===\n");
                    printf("Total de canciones: %d\n", shared_data->result_count);
                    printf("Rango de años: %d - %d\n", 
                           shared_data->results[0].year, shared_data->results[0].duration_ms);
                    printf("Tamaño de la tabla hash: %d\n", HASH_SIZE);
                    printf("Tiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                }
                break;
                
            case 6:
                printf("Saliendo...\n");
                break;
                
            default:
                printf("Opción no válida\n");
        }
        
    } while (option != 6);
}

// Proceso de base de datos
void database_process() {
    printf("Proceso de Base de Datos iniciado (PID: %d)\n", getpid());
    
    const char *bin_filename = "songs_database.bin";
    
    // Verificar si existe la base de datos
    FILE *test_file = fopen(bin_filename, "rb");
    if (!test_file) {
        printf("ERROR: No se encuentra la base de datos '%s'\n", bin_filename);
        printf("Ejecute primero el programa creador de la base de datos.\n");
        sem_wait(sem_id);
        shared_data->shutdown = 1;
        sem_signal(sem_id);
        exit(1);
    }
    fclose(test_file);
    
    printf("Base de datos cargada: %s\n", bin_filename);
    printf("Esperando solicitudes de búsqueda...\n");
    
    // Bucle principal del proceso de base de datos
    while (1) {
        // Verificar si hay solicitud sin bloquear
        sem_wait(sem_id);
        
        if (shared_data->shutdown) {
            sem_signal(sem_id);
            break;
        }
        
        if (shared_data->request_ready) {
            // Procesar solicitud
            int search_type = shared_data->search_type;
            char search_term[256];
            int search_year = shared_data->search_year;
            strcpy(search_term, shared_data->search_term);
            
            shared_data->request_ready = 0; // Solicitud en procesamiento
            sem_signal(sem_id);
            
            // Realizar búsqueda (fuera del semáforo para no bloquear)
            int result_count = 0;
            switch (search_type) {
                case 1: // Nombre exacto
                    result_count = search_by_exact_name(bin_filename, search_term, 
                                                       shared_data->results, MAX_RESULTS);
                    break;
                case 2: // Palabra en nombre
                    result_count = search_by_name_word(bin_filename, search_term, 
                                                      shared_data->results, MAX_RESULTS);
                    break;
                case 3: // Artista
                    result_count = search_by_artist(bin_filename, search_term, 
                                                   shared_data->results, MAX_RESULTS);
                    break;
                case 4: // Año
                    result_count = search_by_year(bin_filename, search_year, 
                                                 shared_data->results, MAX_RESULTS);
                    break;
                case 5: // Estadísticas
                    {
                        int total_songs, min_year, max_year;
                        if (get_database_stats(bin_filename, &total_songs, &min_year, &max_year) == 0) {
                            result_count = total_songs;
                            shared_data->results[0].year = min_year;
                            shared_data->results[0].duration_ms = max_year;
                        }
                    }
                    break;
            }
            
            // Guardar resultados
            sem_wait(sem_id);
            shared_data->result_count = result_count;
            shared_data->response_ready = 1; // Respuesta lista
            sem_signal(sem_id);
            
        } else {
            sem_signal(sem_id);
            usleep(1000); // Espera mínima cuando no hay trabajo
        }
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Crear memoria compartida
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error creando memoria compartida");
        return 1;
    }
    
    // Adjuntar memoria compartida
    shared_data = (SharedData*)shmat(shm_id, NULL, 0);
    if (shared_data == (void*)-1) {
        perror("Error adjuntando memoria compartida");
        return 1;
    }
    
    // Inicializar memoria compartida
    memset(shared_data, 0, sizeof(SharedData));
    shared_data->request_ready = 0;
    shared_data->response_ready = 0;
    shared_data->shutdown = 0;
    
    // Crear semáforo
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Error creando semáforo");
        cleanup();
        return 1;
    }
    
    // Inicializar semáforo a 1
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("Error inicializando semáforo");
        cleanup();
        return 1;
    }
    
    // Crear proceso de base de datos
    db_pid = fork();
    if (db_pid == 0) {
        // Proceso hijo - base de datos
        database_process();
        exit(0);
    } else if (db_pid > 0) {
        // Proceso padre - interfaz de usuario
        printf("Iniciando interfaz de usuario...\n");
        sleep(1); // Esperar a que la BD se inicialice
        
        user_interface_process();
        
        cleanup();
    } else {
        perror("Error creando proceso");
        cleanup();
        return 1;
    }
    
    return 0;
}
