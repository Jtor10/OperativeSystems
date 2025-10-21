#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <strings.h>

#define HASH_SIZE 1000
#define MAX_TITLE 256
#define MAX_ARTIST 256
#define MAX_ALBUM 256
#define MAX_LINE 1024
#define PIPE_NAME "/tmp/music_db_pipe"
#define MAX_RESULTS 1000

typedef struct {
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

typedef struct {
    long first_position;
} HashEntry;

typedef struct {
    int search_type;
    char search_term[256];
    int search_year;
    double min_value;
    double max_value;
} SearchRequest;

typedef struct {
    int result_count;
    Song results[MAX_RESULTS];
} SearchResponse;

// Funciones de búsqueda
int hash_function(const char *name);
int search_by_exact_name(const char *filename, const char *name, Song *results, int max_results);
int search_by_name_word(const char *filename, const char *word, Song *results, int max_results);
int search_by_artist(const char *filename, const char *artist, Song *results, int max_results);
int search_by_year(const char *filename, int year, Song *results, int max_results);
int search_by_danceability_range(const char *filename, double min_val, double max_val, Song *results, int max_results);
int search_by_energy_range(const char *filename, double min_val, double max_val, Song *results, int max_results);

// Función de carga desde CSV (del creador original)
void load_songs_from_csv(const char *csv_filename, const char *bin_filename);
void safe_strcpy(char *dest, const char *src, size_t dest_size);

#endif
