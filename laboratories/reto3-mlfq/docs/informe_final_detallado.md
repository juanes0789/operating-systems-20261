# Informe final detallado - Simulador MLFQ en C

## 0. Resumen ejecutivo

Este proyecto implementa un simulador de planificacion de CPU con politica **Multi-Level Feedback Queue (MLFQ)**.
El sistema modela procesos con tiempos de llegada y rafaga, los organiza en tres colas de prioridad, ejecuta por ciclos discretos y calcula metricas de desempeno.

Reglas implementadas:

- `Q0` (alta prioridad), quantum `2`
- `Q1` (media prioridad), quantum `4`
- `Q2` (baja prioridad), quantum `8`
- Seleccion por prioridad estricta (siempre la cola mas alta no vacia)
- Round Robin dentro de cada cola
- Democion por consumo completo de quantum
- Sin democion si termina antes del quantum
- Priority boost global cada `S` ciclos
- Salida en consola y exportacion a CSV

---

## 1. Objetivo del proyecto

Construir un simulador didactico y formalmente organizado para estudiar:

1. Como cambia el comportamiento del scheduler segun prioridades y quantums.
2. Como el priority boost mitiga starvation.
3. Como se miden tiempos de respuesta, espera y retorno.
4. Como documentar y reproducir resultados de manera verificable.

---

## 2. Arquitectura general del codigo

Estructura principal:

- `include/scheduler.h`: definiciones de tipos y API publica.
- `src/main.c`: entrada del programa, parseo CLI y orquestacion.
- `src/scheduler.c`: nucleo del algoritmo MLFQ.
- `src/io.c`: lectura de procesos desde CSV y escritura de resultados.
- `src/metrics.c`: calculo de promedios e impresion tabular.
- `data/*.csv`: escenarios de prueba.
- `docs/*.md`: analisis, validacion teorica y justificacion.

Separar responsabilidades facilita:

- mantenimiento,
- pruebas,
- trazabilidad,
- extension futura del proyecto.

---

## 3. Modelo de datos explicado para todo publico

### 3.1 Estructura `Process`

Representa un proceso del sistema:

- `pid`: nombre del proceso (ejemplo `P1`).
- `arrival_time`: ciclo en que aparece en el sistema.
- `burst_time`: CPU total que necesita.
- `remaining_time`: CPU que aun falta por ejecutar.
- `start_time`: primer ciclo en que realmente se ejecuta.
- `finish_time`: ciclo en que termina.
- `first_response_time`: primer momento de respuesta.
- `current_queue`: cola actual (`0`, `1`, `2`).
- `quantum_used`: cuanto ha usado del quantum actual.

### 3.2 Estructura `ReadyQueue`

Cola circular para Round Robin:

- `items`: arreglo de indices de procesos.
- `head`: posicion de lectura.
- `tail`: posicion de insercion.
- `size`: cantidad de elementos.
- `capacity`: capacidad maxima.

### 3.3 Estructura `SchedulerConfig`

Parametros del scheduler:

- `quantums[3]`: `2,4,8`.
- `boost_period`: cada cuantos ciclos ocurre boost.

### 3.4 Estructura `SimulationSummary`

Promedios globales:

- `average_response`
- `average_turnaround`
- `average_waiting`

---

## 4. Flujo completo de ejecucion

1. `main` lee argumentos de consola (`--input`, `--output`, `--boost`).
2. `load_processes_csv` carga los procesos desde un archivo CSV.
3. `run_mlfq_simulation` ejecuta el reloj ciclo por ciclo hasta completar todos.
4. `compute_summary` calcula promedios.
5. `print_results` muestra resultados en consola.
6. `write_results_csv` exporta tabla final en CSV.

---

## 5. Explicacion tecnica funcion por funcion

## 5.1 `src/main.c`

### `print_usage(const char *program)`

- Muestra formato correcto de uso por terminal.
- Se invoca cuando el usuario pasa argumentos invalidos.

### `main(int argc, char **argv)`

Responsabilidades:

1. Definir rutas por defecto:
   - entrada: `data/processes_example.csv`
   - salida: `out/results.csv`
2. Leer argumentos de linea de comandos.
3. Validar `--boost >= 0`.
4. Cargar procesos del CSV.
5. Ejecutar simulacion MLFQ.
6. Calcular metricas promedio.
7. Imprimir y exportar resultados.
8. Liberar memoria (`free`).

Si ocurre error en cualquier paso, retorna codigo `1`.
Si todo sale bien, retorna `0`.

---

## 5.2 `src/io.c`

### `trim(char *s)`

- Limpia espacios al inicio y al final de una cadena.
- Evita errores al leer campos CSV con espacios extra.

### `ensure_parent_dir(const char *path)`

- Verifica/crea el directorio padre de la ruta de salida.
- Ejemplo: si salida es `out/results.csv`, asegura `out/`.

### `load_processes_csv(const char *path, Process **processes, size_t *count)`

Proceso interno:

1. Abre el archivo.
2. Reserva memoria dinamica inicial para procesos.
3. Ignora cabecera.
4. Lee linea por linea.
5. Separa columnas `pid,arrival,burst`.
6. Valida datos (arrival >= 0, burst > 0).
7. Redimensiona arreglo con `realloc` si se llena.
8. Devuelve arreglo final y cantidad.

Retorna `0` si todo va bien, `-1` si hay error.

### `write_results_csv(const char *path, const Process *processes, size_t count)`

- Crea/abre archivo de salida.
- Escribe cabecera:
  `PID,Arrival,Burst,Start,Finish,Response,Turnaround,Waiting`
- Recorre procesos y escribe metricas por fila.
- Cierra archivo y retorna estado.

---

## 5.3 `src/metrics.c`

### `response_time(const Process *p)`

Calcula:

`first_response_time - arrival_time`

### `turnaround_time(const Process *p)`

Calcula:

`finish_time - arrival_time`

### `waiting_time(const Process *p)`

Calcula:

`turnaround - burst_time`

### `compute_summary(const Process *processes, size_t count, SimulationSummary *summary)`

- Suma las metricas de todos los procesos.
- Divide por cantidad para obtener promedios.
- Maneja punteros nulos y lista vacia de forma segura.

### `print_results(const Process *processes, size_t count, const SimulationSummary *summary)`

- Imprime tabla completa de procesos.
- Al final imprime promedios globales.

---

## 5.4 `src/scheduler.c` (nucleo MLFQ)

### Bloque de colas (`queue_*`)

#### `queue_init`

- Reserva memoria para la cola circular y reinicia punteros.

#### `queue_destroy`

- Libera memoria y limpia campos.

#### `queue_push`

- Inserta un proceso al final de la cola.
- Falla si la cola esta llena.

#### `queue_pop`

- Extrae el proceso al frente de la cola.
- Falla si la cola esta vacia.

#### `queue_empty`

- Retorna si la cola no tiene elementos.

### Bloque de control de procesos

#### `reset_process`

- Inicializa estado previo a simular:
  `remaining_time`, `start_time=-1`, `finish_time=-1`, etc.

#### `has_higher_ready_queue`

- Verifica si existe una cola de mayor prioridad con procesos listos.
- Sirve para decidir preempcion.

#### `choose_next_process`

- Busca de `Q0` a `Q2`.
- Elige el primero disponible segun prioridad.
- Mantiene Round Robin al usar pop FIFO.

#### `enqueue_new_arrivals`

- En cada tick agrega a `Q0` los procesos cuyo `arrival_time == tiempo_actual`.

### Bloque de boost

#### `apply_priority_boost`

- Si hay proceso corriendo, lo reinserta en `Q0`.
- Mueve pendientes de colas inferiores hacia `Q0`.
- Reinicia `quantum_used` para que todos reingresen con oportunidad justa.

### Funcion principal

#### `run_mlfq_simulation(Process *processes, size_t count, const SchedulerConfig *config)`

Es la simulacion completa por ciclos:

1. Valida entradas.
2. Inicializa procesos y colas.
3. Repite hasta completar todos los procesos:
   - encolar llegadas,
   - aplicar boost si corresponde,
   - preemptar si hay cola superior activa,
   - seleccionar proceso si CPU esta libre,
   - ejecutar 1 tick,
   - si termina: registrar `finish_time`,
   - si agota quantum: demover y reencolar.
4. Libera memoria.
5. Retorna `0` si todos terminaron, `-1` en caso contrario.

---

## 6. Explicacion de "cada palabra" importante del lenguaje C en este proyecto

No es practico explicar literalmente cada token del archivo, pero si todas las palabras clave y elementos que sostienen la logica.

### Palabras clave de C usadas

- `typedef`: crea un alias de tipo para usar nombres mas legibles.
- `struct`: define una estructura de datos.
- `static`: limita visibilidad de funciones al archivo actual.
- `const`: indica que un dato no debe modificarse.
- `int`, `size_t`, `double`, `char`: tipos de datos.
- `if`, `else`: decisiones condicionales.
- `for`, `while`: ciclos de repeticion.
- `return`: devuelve un valor desde una funcion.
- `continue`: salta a la siguiente iteracion del ciclo.
- `goto`: salta a etiqueta de error para limpieza centralizada.

### Operadores y simbolos relevantes

- `->`: acceso a campo de estructura por puntero.
- `.`: acceso a campo de estructura por valor.
- `==`, `!=`, `<`, `<=`, `>=`: comparaciones.
- `&&`, `||`: operadores logicos.
- `%`: modulo (usado para boost periodico y cola circular).
- `++`, `--`: incremento/decremento.
- `(type)`: cast de tipo.

### Funciones de libreria mas importantes

- `malloc`, `realloc`, `free`: gestion dinamica de memoria.
- `fopen`, `fgets`, `fprintf`, `fclose`: E/S de archivos.
- `strtok_r`: separacion segura de campos CSV.
- `strtol`, `atoi`: conversion de texto a entero.
- `strlen`, `strcpy`, `memset`, `memmove`: utilidades de cadena/memoria.
- `mkdir`: creacion de directorios.

---

## 7. Relacion entre teoria y resultados

El proyecto conecta directamente con teoria de planificacion:

- Procesos cortos suelen obtener mejor respuesta inicial.
- Procesos largos descienden de cola, reduciendo interferencia a interactivos.
- El boost periodico evita que procesos en colas bajas queden olvidados.

Escenario grande recomendado:

- Entrada: `data/processes_stress_mlfq.csv`
- Resultado esperado: `docs/mlfq_theoretical_validation.md`
- Salida simulada: `out/results_stress.csv`

La coincidencia entre esperado y ejecutado sustenta la correccion del simulador.

---

## 8. Fortalezas del trabajo realizado

1. Cumplimiento completo de especificaciones funcionales.
2. Arquitectura modular y mantenible.
3. Simulacion determinista y reproducible.
4. Metricas claras y exportables.
5. Documentacion de analisis y justificacion academica.

---

## 9. Limitaciones y mejoras futuras

- Agregar pruebas unitarias automatizadas por modulo.
- Exportar timeline detallado tipo Gantt por tick.
- Permitir definir quantums y politica desde archivo de configuracion.
- Anadir comparacion automatica entre resultados teoricos y simulados.

---

## 10. Conclusiones

El proyecto entrega un simulador MLFQ funcional, estructurado y defendible tecnicamente.
No solo ejecuta la politica pedida, tambien produce evidencia cuantitativa (metricas y CSV), permite analisis academico de decisiones de scheduling y ofrece una base solida para evolucion a un laboratorio mas avanzado de sistemas operativos.

En terminos de sustentacion, el trabajo demuestra:

- dominio del modelo MLFQ,
- correcta traduccion de reglas teoricas a implementacion en C,
- uso responsable de memoria y estructuras,
- capacidad de validacion experimental reproducible.

---

## 11. Anexo de sustentacion oral

Para presentacion leida ante publico (version narrativa extensa del caso de prueba), revisar:

- `docs/guion_sustentacion_caso_prueba_extenso.md`

Este anexo traduce la ejecucion del escenario de estres a una historia de contexto real,
manteniendo los conceptos tecnicos correctos de MLFQ y un bloque de preguntas/respuestas
sugeridas para defensa ante jurado.

