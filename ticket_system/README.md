# Sistema de Registro de Tickets de Reclamación

Juan Esteban Mosquera Perea - 1046528349

Maria Paula Mosquera Alvarez - 1022002020


## Estructura del Proyecto
- `src/`: Contiene la lógica de implementación (.c).
- `include/`: Definiciones de estructuras y prototipos de funciones (.h).
- `assets/`: Directorio donde se almacenan los tickets generados.
- `Makefile`: Script de automatización de compilación.

## Conceptos Técnicos Aplicados

### 1. Manejo de Memoria Dinámica
Se utiliza la función `malloc` para reservar memoria en el *heap* para la estructura `Ticket`. Esto permite que la vida útil del objeto no esté limitada al scope de la función creadora. Al finalizar, se emplea `free` para liberar la memoria y evitar *memory leaks*.

### 2. Uso de Punteros y Paso por Referencia
Para optimizar el rendimiento, las estructuras se pasan a las funciones mediante punteros (`Ticket*`). Esto evita copiar toda la estructura en el *stack* cada vez que se llama a una función de validación o guardado.

### 3. Generación de Radicado
El número de radicado se genera utilizando una conversión explícita (`cast`) de la función `time(NULL)` de la librería `<time.h>`, garantizando un identificador único basado en el timestamp de la época Unix.

### 4. Makefile y Modularización
El proyecto utiliza un `Makefile` con flags de seguridad (`-Wall -Wextra`). La modularización separa la lógica de negocio (`ticket.c`) de las funciones de soporte (`utils.c`), facilitando el mantenimiento.

## Instrucciones
- **Compilar:** `make`
- **Ejecutar:** `make run`
- **Limpiar:** `make clean`
