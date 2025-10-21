# Proyecto Sistemas Operativos - Base de Datos de Música

## 📋 Descripción del Proyecto

Sistema de gestión y búsqueda de canciones implementado en C que utiliza procesos no emparentados y comunicación interprocesos para realizar búsquedas eficientes en una base de datos de características musicales de Spotify.

## 👥 Integrantes

- **David Nicolas Urrego Botero**
- **Santiago Zamora Garzón**  
- **Jepherson Brian Torres Cruz**

## 📊 Dataset

**Nombre:** Spotify Tracks Features  
**Archivo:** `tracks_features.csv`

### Estructura de Datos
| Campo | Descripción | Tipo |
|-------|-------------|------|
| `id` | Identificador único de la canción | String |
| `name` | Nombre de la canción | String |
| `album` | Álbum al que pertenece | String |
| `artists` | Artista(s) de la canción | String |
| `year` | Año de lanzamiento | Entero |
| `duration_ms` | Duración en milisegundos | Entero |
| `danceability` | Bailabilidad (0.0 - 1.0) | Float |
| `energy` | Energía (0.0 - 1.0) | Float |
| `tempo` | Tempo en BPM | Float |

## 🔍 Criterios de Búsqueda Implementados

1. **Búsqueda por nombre exacto** - Usando tabla hash para acceso rápido
2. **Búsqueda por palabra en el nombre** - Búsqueda parcial en el título
3. **Búsqueda por artista** - Encuentra canciones por artista
4. **Búsqueda por año** - Canciones de un año específico
5. **Búsqueda por rango de bailabilidad** - Rango entre 0.0 y 1.0
6. **Búsqueda por rango de energía** - Rango entre 0.0 y 1.0

### Rangos de Valores Válidos
- **Año:** 1900 - 2024
- **Bailabilidad:** 0.0 - 1.0
- **Energía:** 0.0 - 1.0

## 🏗️ Arquitectura del Sistema

### Estructura Técnica
- **Procesos:** Dos procesos no emparentados
- **Comunicación:** Tuberías nombradas (FIFOs)
- **Indexación:** Tabla hash de 1000 posiciones
- **Almacenamiento:** Archivo binario indexado
- **Memoria:** Gestión dinámica con `malloc()`/`free()`

### Flujo del Sistema

Usuario → Proceso 1 (Interfaz) → Tuberías → Proceso 2 (Búsqueda) → Resultados


### Características Cumplidas

    Dos procesos no emparentados

    Comunicación por tuberías nombradas

    Tabla hash para búsqueda eficiente

    Gestión de memoria dinámica

    Datos almacenados en disco (no en memoria)

    Tiempo de búsqueda < 2 segundos

    Uso de memoria < 10MB


### Dependencias

    Compilador: GCC

    Sistema Operativo: Linux

    Herramientas: Make

### Notas Técnicas

    El sistema utiliza memoria compartida para la comunicación entre procesos

    La tabla hash garantiza búsquedas en tiempo constante O(1) para nombres exactos

    Los archivos binarios indexados permiten acceso rápido a los datos

    Gestión automática de memoria para prevenir leaks
