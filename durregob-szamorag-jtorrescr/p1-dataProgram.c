#include "database.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>

// Variables globales para comunicación
int pipe_fd = -1;
pid_t db_pid = -1;

// Función para limpiar recursos
void cleanup() {
    if (pipe_fd != -1) {
        close(pipe_fd);
    }
    unlink(PIPE_NAME);
    
    if (db_pid != -1) {
        kill(db_pid, SIGTERM);
        waitpid(db_pid, NULL, 0);
    }
}

// Manejador de señales
void signal_handler(int sig) {
    (void)sig;
    printf("\nFinalizando programa...\n");
    cleanup();
    exit(0);
}

// Función para crear base de datos desde CSV (del creador original)
void create_database_from_csv() {
    const char *csv_filename = "tracks_features.csv";
    const char *bin_filename = "songs_database.bin";
    
    printf("=== CREANDO BASE DE DATOS COMPLETA ===\n");
    
    // Verificar si existe el CSV
    FILE *csv_file = fopen(csv_filename, "r");
    if (!csv_file) {
        printf("Error: No se encuentra el archivo '%s'\n", csv_filename);
        printf("Coloque el archivo CSV en el mismo directorio y vuelva a ejecutar.\n");
        return;
    }
    fclose(csv_file);
    
    // Crear archivo binario básico primero
    printf("Creando estructura de base de datos...\n");
    
    FILE *bin_file = fopen(bin_filename, "wb");
    if (!bin_file) {
        printf("Error creando archivo binario\n");
        return;
    }
    
    HashEntry hash_table[HASH_SIZE];
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_table[i].first_position = -1;
    }
    
    fwrite(hash_table, sizeof(HashEntry), HASH_SIZE, bin_file);
    fclose(bin_file);
    
    printf("Base de datos básica creada. Cargando canciones desde CSV...\n");
    printf("Esto puede tomar varios minutos...\n");
    
    // Usar la función de carga del database.c
    load_songs_from_csv(csv_filename, bin_filename);
    
    printf("=== BASE DE DATOS CREADA EXITOSAMENTE ===\n");
}

// Función para esperar a que el pipe esté disponible
int wait_for_pipe(const char *pipe_name, int max_attempts) {
    int attempts = 0;
    while (attempts < max_attempts) {
        int fd = open(pipe_name, O_WRONLY | O_NONBLOCK);
        if (fd != -1) {
            close(fd);
            return 0;
        }
        if (errno != ENOENT) {
            return 0;
        }
        sleep(1);
        attempts++;
        printf("Esperando a que la base de datos esté lista... (%d/%d)\n", attempts, max_attempts);
    }
    return -1;
}

// Función para enviar solicitud y recibir respuesta
int send_search_request(SearchRequest *req, SearchResponse *resp) {
    if (pipe_fd == -1) {
        printf("Error: No hay conexión con la base de datos\n");
        return -1;
    }
    
    ssize_t bytes_written = write(pipe_fd, req, sizeof(SearchRequest));
    if (bytes_written != sizeof(SearchRequest)) {
        printf("Error enviando solicitud\n");
        return -1;
    }
    
    ssize_t bytes_read = read(pipe_fd, resp, sizeof(SearchResponse));
    if (bytes_read != sizeof(SearchResponse)) {
        printf("Error recibiendo respuesta\n");
        return -1;
    }
    
    return 0;
}

// Función para formatear duración
void format_duration(int duration_ms, char *buffer, size_t buffer_size) {
    int total_seconds = duration_ms / 1000;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    snprintf(buffer, buffer_size, "%d:%02d", minutes, seconds);
}

// Mostrar resultados
void display_results(SearchResponse *resp) {
    if (resp->result_count == 0) {
        printf("NA - No se encontraron resultados\n");
        return;
    }
    
    printf("\n=== RESULTADOS ENCONTRADOS: %d ===\n", resp->result_count);
    
    for (int i = 0; i < resp->result_count && i < 10; i++) {
        Song *song = &resp->results[i];
        char duration_str[20];
        format_duration(song->duration_ms, duration_str, sizeof(duration_str));
        
        printf("\n%d. %s\n", i + 1, song->name);
        printf("   Artista: %s\n", song->artists);
        printf("   Álbum: %s | Año: %d\n", song->album, song->year);
        printf("   Duración: %s | Bailabilidad: %.3f | Energía: %.3f\n", 
               duration_str, song->danceability, song->energy);
    }
    
    if (resp->result_count > 10) {
        printf("\n... y %d resultados más\n", resp->result_count - 10);
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

int safe_scanf_double(const char *format, double *value) {
    int result = scanf(format, value);
    while (getchar() != '\n');
    return result;
}

// Función segura para leer caracter
int safe_scanf_char(char *value) {
    int result = scanf(" %c", value);
    while (getchar() != '\n');
    return result;
}

// Proceso de interfaz de usuario
void user_interface_process() {
    printf("=== SISTEMA DE BÚSQUEDA DE CANCIONES ===\n");
    printf("Base de datos de música - Proceso de Interfaz\n\n");
    
    int option;
    SearchRequest req;
    SearchResponse resp;
    
    do {
        printf("\n=== MENÚ PRINCIPAL ===\n");
        printf("1. Buscar por nombre exacto\n");
        printf("2. Buscar por palabra en el nombre\n");
        printf("3. Buscar por artista\n");
        printf("4. Buscar por año\n");
        printf("5. Buscar por rango de bailabilidad (0.0 - 1.0)\n");
        printf("6. Buscar por rango de energía (0.0 - 1.0)\n");
        printf("7. Crear/Regenerar base de datos desde CSV\n");
        printf("8. Salir\n");
        printf("Seleccione una opción: ");
        
        if (safe_scanf_int("%d", &option) != 1) {
            printf("Entrada inválida\n");
            continue;
        }
        
        memset(&req, 0, sizeof(SearchRequest));
        clock_t start, end;
        double cpu_time_used;
        
        switch (option) {
            case 1:
                printf("Ingrese el nombre exacto de la canción: ");
                safe_fgets(req.search_term, sizeof(req.search_term));
                req.search_type = 1;
                
                start = clock();
                if (send_search_request(&req, &resp) == 0) {
                    end = clock();
                    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                    display_results(&resp);
                    printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                }
                break;
                
            case 2:
                printf("Ingrese palabra a buscar en nombres: ");
                safe_fgets(req.search_term, sizeof(req.search_term));
                req.search_type = 2;
                
                start = clock();
                if (send_search_request(&req, &resp) == 0) {
                    end = clock();
                    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                    display_results(&resp);
                    printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                }
                break;
                
            case 3:
                printf("Ingrese nombre del artista: ");
                safe_fgets(req.search_term, sizeof(req.search_term));
                req.search_type = 3;
                
                start = clock();
                if (send_search_request(&req, &resp) == 0) {
                    end = clock();
                    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                    display_results(&resp);
                    printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                }
                break;
                
            case 4:
                printf("Ingrese año a buscar: ");
                if (safe_scanf_int("%d", &req.search_year) == 1) {
                    req.search_type = 4;
                    
                    start = clock();
                    if (send_search_request(&req, &resp) == 0) {
                        end = clock();
                        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                        display_results(&resp);
                        printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                    }
                } else {
                    printf("Año inválido\n");
                }
                break;
                
            case 5:
                printf("Ingrese valor mínimo de bailabilidad (0.0-1.0): ");
                if (safe_scanf_double("%lf", &req.min_value) == 1) {
                    printf("Ingrese valor máximo de bailabilidad (0.0-1.0): ");
                    if (safe_scanf_double("%lf", &req.max_value) == 1) {
                        req.search_type = 5;
                        
                        start = clock();
                        if (send_search_request(&req, &resp) == 0) {
                            end = clock();
                            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                            display_results(&resp);
                            printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                        }
                    } else {
                        printf("Valor máximo inválido\n");
                    }
                } else {
                    printf("Valor mínimo inválido\n");
                }
                break;
                
            case 6:
                printf("Ingrese valor mínimo de energía (0.0-1.0): ");
                if (safe_scanf_double("%lf", &req.min_value) == 1) {
                    printf("Ingrese valor máximo de energía (0.0-1.0): ");
                    if (safe_scanf_double("%lf", &req.max_value) == 1) {
                        req.search_type = 6;
                        
                        start = clock();
                        if (send_search_request(&req, &resp) == 0) {
                            end = clock();
                            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                            display_results(&resp);
                            printf("\nTiempo de búsqueda: %.3f segundos\n", cpu_time_used);
                        }
                    } else {
                        printf("Valor máximo inválido\n");
                    }
                } else {
                    printf("Valor mínimo inválido\n");
                }
                break;
                
            case 7:
                printf("\n¿Está seguro de que desea crear/regenerar la base de datos? (s/n): ");
                char confirm;
                if (safe_scanf_char(&confirm) == 1 && (confirm == 's' || confirm == 'S')) {
                    create_database_from_csv();
                }
                break;
                
            case 8:
                printf("Saliendo del sistema...\n");
                break;
                
            default:
                printf("Opción no válida\n");
        }
        
    } while (option != 8);
}

// Proceso de base de datos
void database_process() {
    printf("Proceso de Base de Datos iniciado (PID: %d)\n", getpid());
    
    const char *bin_filename = "songs_database.bin";
    
    // Verificar si existe la base de datos
    FILE *test_file = fopen(bin_filename, "rb");
    if (!test_file) {
        printf("Base de datos no encontrada: %s\n", bin_filename);
        printf("Ejecute la opción 7 del menú para crear la base de datos desde CSV.\n");
        
        // Crear una base de datos vacía para que el sistema funcione
        printf("Creando base de datos vacía temporal...\n");
        FILE *bin_file = fopen(bin_filename, "wb");
        if (bin_file) {
            HashEntry hash_table[HASH_SIZE];
            for (int i = 0; i < HASH_SIZE; i++) {
                hash_table[i].first_position = -1;
            }
            fwrite(hash_table, sizeof(HashEntry), HASH_SIZE, bin_file);
            fclose(bin_file);
            printf("Base de datos vacía creada. Use la opción 7 para cargar datos reales.\n");
        }
    } else {
        fclose(test_file);
        printf("Base de datos cargada: %s\n", bin_filename);
    }
    
    // Crear pipe nombrado
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        perror("Error creando pipe");
        exit(1);
    }
    
    int fd = open(PIPE_NAME, O_RDWR);
    if (fd == -1) {
        perror("Error abriendo pipe");
        unlink(PIPE_NAME);
        exit(1);
    }
    
    printf("Pipe creado. Esperando solicitudes de búsqueda...\n");
    
    // Bucle principal del proceso de base de datos
    while (1) {
        SearchRequest req;
        SearchResponse resp;
        
        ssize_t bytes_read = read(fd, &req, sizeof(SearchRequest));
        if (bytes_read == sizeof(SearchRequest)) {
            memset(&resp, 0, sizeof(SearchResponse));
            
            // Procesar según el tipo de búsqueda
            switch (req.search_type) {
                case 1: // Nombre exacto
                    resp.result_count = search_by_exact_name(bin_filename, req.search_term, resp.results, MAX_RESULTS);
                    break;
                case 2: // Palabra en nombre
                    resp.result_count = search_by_name_word(bin_filename, req.search_term, resp.results, MAX_RESULTS);
                    break;
                case 3: // Artista
                    resp.result_count = search_by_artist(bin_filename, req.search_term, resp.results, MAX_RESULTS);
                    break;
                case 4: // Año
                    resp.result_count = search_by_year(bin_filename, req.search_year, resp.results, MAX_RESULTS);
                    break;
                case 5: // Rango bailabilidad
                    resp.result_count = search_by_danceability_range(bin_filename, req.min_value, req.max_value, resp.results, MAX_RESULTS);
                    break;
                case 6: // Rango energía
                    resp.result_count = search_by_energy_range(bin_filename, req.min_value, req.max_value, resp.results, MAX_RESULTS);
                    break;
                default:
                    resp.result_count = 0;
            }
            
            ssize_t bytes_written = write(fd, &resp, sizeof(SearchResponse));
            if (bytes_written != sizeof(SearchResponse)) {
                printf("Error enviando respuesta\n");
            }
        } else if (bytes_read == 0) {
            break;
        } else if (bytes_read == -1) {
            perror("Error leyendo del pipe");
            break;
        }
    }
    
    close(fd);
    unlink(PIPE_NAME);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (argc == 2 && strcmp(argv[1], "--db") == 0) {
        database_process();
        return 0;
    }
    
    // Si se pasa --create, crear la base de datos y salir
    if (argc == 2 && strcmp(argv[1], "--create") == 0) {
        create_database_from_csv();
        return 0;
    }
    
    db_pid = fork();
    if (db_pid == 0) {
        database_process();
        exit(0);
    } else if (db_pid > 0) {
        printf("Iniciando interfaz de usuario...\n");
        
        if (wait_for_pipe(PIPE_NAME, 5) == -1) {
            printf("Error: No se pudo conectar con la base de datos\n");
            cleanup();
            return 1;
        }
        
        pipe_fd = open(PIPE_NAME, O_RDWR);
        if (pipe_fd == -1) {
            perror("Error conectando al pipe");
            cleanup();
            return 1;
        }
        
        printf("Conectado a la base de datos (PID: %d)\n", db_pid);
        printf("Sistema listo para búsquedas.\n");
        user_interface_process();
        
        cleanup();
    } else {
        perror("Error creando proceso");
        return 1;
    }
    
    return 0;
}
