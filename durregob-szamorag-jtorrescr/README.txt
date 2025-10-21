PROYECTO SISTEMAS OPERATIVOS - BASE DE DATOS DE MÚSICA
=====================================================

Integrantes:
- David Nicolas Urrego Botero
- Santiago Zamora Garzón
- Jepherson Brian Torres Cruz

DATASET: Spotify Tracks Features
Archivo: tracks_features.csv

DESCRIPCIÓN DE CAMPOS:
- id: Identificador único de la canción
- name: Nombre de la canción
- album: Álbum al que pertenece
- artists: Artista(s) de la canción
- year: Año de lanzamiento
- duration_ms: Duración en milisegundos
- danceability: Bailabilidad (0.0 - 1.0)
- energy: Energía (0.0 - 1.0)
- tempo: Tempo en BPM

CRITERIOS DE BÚSQUEDA IMPLEMENTADOS:
1. Búsqueda por nombre exacto (usando tabla hash)
2. Búsqueda por palabra en el nombre
3. Búsqueda por artista
4. Búsqueda por año
5. Búsqueda por rango de bailabilidad
6. Búsqueda por rango de energía

RANGOS DE VALORES VÁLIDOS:
- Año: 1900-2024
- Bailabilidad: 0.0 - 1.0
- Energía: 0.0 - 1.0

ESTRUCTURA TÉCNICA:
- Dos procesos no emparentados
- Comunicación mediante tuberías nombradas
- Tabla hash de 1000 posiciones
- Archivo binario indexado
- Memoria dinámica con malloc/free

COMPILACIÓN Y USO:
1. make
2. ./p1-dataProgram

EJEMPLOS DE USO:
- Buscar "Bohemian Rhapsody" (nombre exacto)
- Buscar "love" (palabra en nombre)
- Buscar "The Beatles" (artista)
- Buscar 1985 (año)
- Buscar bailabilidad 0.7-0.9 (rango)

CARACTERÍSTICAS CUMPLIDAS:
✓ Dos procesos no emparentados
✓ Comunicación por tuberías nombradas
✓ Tabla hash para búsqueda eficiente
✓ Memoria dinámica
✓ Datos en disco (no en memoria)
✓ Búsqueda en menos de 2 segundos
✓ Uso de memoria < 10MB
