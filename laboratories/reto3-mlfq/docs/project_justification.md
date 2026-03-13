# Justificacion detallada del proyecto

## 1. Cumplimiento de requerimientos funcionales

El proyecto implementa de forma directa los requisitos del enunciado:

- Politica MLFQ con 3 niveles y quantums distintos (`2`, `4`, `8`).
- Prioridad estricta entre colas: siempre se ejecuta la cola de mayor prioridad no vacia.
- Round Robin en cada nivel mediante colas FIFO.
- Democion cuando un proceso consume todo su quantum del nivel actual.
- Sin democion cuando el proceso termina antes de agotar quantum.
- Priority boost periodico configurable (`--boost S`), incluyendo opcion `S=0` para deshabilitar.
- Simulacion discreta por ciclos de reloj (tick a tick).
- Metricas obligatorias por proceso y promedios globales.
- Exportacion de resultados en formato CSV para trazabilidad y analisis externo.

## 2. Decisiones de diseno y por que son correctas

### 2.1 Modelo de proceso

Cada proceso mantiene exactamente los atributos solicitados:

- `pid`, `arrival_time`, `burst_time`, `remaining_time`
- `start_time`, `finish_time`, `first_response_time`
- `current_queue`

Adicionalmente se usa `quantum_used` para controlar democion por consumo completo de quantum, lo cual evita ambiguedades en cada tick.

### 2.2 Estructura de colas

Se implementan colas circulares (`ReadyQueue`) por nivel. Ventajas:

- Operaciones `push/pop` en tiempo constante `O(1)`.
- Orden FIFO estable para Round Robin.
- Comportamiento determinista y reproducible.

### 2.3 Motor de simulacion por ticks

La simulacion avanza de uno en uno (`time++`) y en cada tick hace:

1. Encolar nuevos arribos.
2. Aplicar boost si corresponde al ciclo actual.
3. Resolver preempcion entre niveles (si aparece cola de mayor prioridad).
4. Seleccionar siguiente proceso por prioridad.
5. Ejecutar 1 tick y actualizar estado.
6. Finalizar o demover segun quantum/restante.

Este orden respeta la semantica MLFQ clasica y garantiza consistencia temporal.

## 3. Validez de metricas y formulas

Las formulas implementadas son las estandar en scheduling:

- `Response = first_response_time - arrival_time`
- `Turnaround = finish_time - arrival_time`
- `Waiting = turnaround - burst_time`

Estas metricas cubren las tres perspectivas clave:

- Latencia inicial (response).
- Tiempo total en el sistema (turnaround).
- Tiempo de espera acumulado sin ejecutar CPU (waiting).

## 4. Evidencia de funcionamiento exitoso

Se incluyen dos niveles de evidencia:

- Escenario base en `data/processes_example.csv` (el sugerido en el enunciado).
- Escenario grande en `data/processes_stress_mlfq.csv` con 12 procesos heterogeneos.

Los resultados esperados teoricamente del escenario grande estan documentados en `docs/mlfq_theoretical_validation.md` y se comparan con `out/results_stress.csv` para validacion objetiva.

## 5. Analisis tecnico y academico

El proyecto no solo ejecuta, tambien permite argumentar comportamiento del scheduler:

- Efecto del boost frecuente vs. nulo.
- Impacto de quantums pequenos en latencia y overhead.
- Riesgo de starvation y mitigacion con boosts periodicos.

Ese analisis esta consolidado en `docs/analysis.md`.

## 6. Calidad de ingenieria del entregable

La organizacion del repositorio esta separada por responsabilidades:

- Nucleo de simulacion (`src/scheduler.c`).
- Entrada/salida (`src/io.c`).
- Metricas y reporte (`src/metrics.c`).
- Orquestacion CLI (`src/main.c`).
- Contratos de tipos/prototipos (`include/scheduler.h`).

Beneficios:

- Facil mantenimiento y depuracion.
- Reproducibilidad de experimentos con diferentes CSV.
- Base clara para extender politicas o agregar pruebas automaticas.

## 7. Limitaciones conocidas y alcance

Dentro del alcance solicitado, la solucion es correcta y suficiente. Sin embargo, como evolucion natural futura:

- Agregar pruebas unitarias y de regresion automatizadas.
- Exportar timeline completo (Gantt) para auditoria visual de ticks.
- Parametrizar quantums por CLI/archivo para experimentacion avanzada.

Estas mejoras no afectan el cumplimiento actual; solo amplian la robustez del proyecto.

