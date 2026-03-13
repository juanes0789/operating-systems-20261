# Simulador MLFQ en C

Este proyecto implementa un scheduler **Multi-Level Feedback Queue (MLFQ)** por ciclos de reloj con las reglas solicitadas:

- `Q0` (alta): quantum = 2
- `Q1` (media): quantum = 4
- `Q2` (baja): quantum = 8
- Seleccion siempre por cola de mayor prioridad no vacia
- Round Robin dentro de cada cola
- Democion al agotar quantum
- Sin democion si termina antes
- Priority boost cada `S` ciclos (configurable)
- Calculo de metricas por proceso y exportacion CSV
- Generacion automatica de reporte HTML por ejecucion

## Estructura

- `include/scheduler.h`: tipos, traza y prototipos
- `src/scheduler.c`: nucleo de simulacion MLFQ y captura de eventos
- `src/io.c`: carga de procesos y escritura de resultados CSV
- `src/metrics.c`: metricas y salida tabular por consola
- `src/report.c`: generacion del reporte HTML con timeline, eventos y snapshots
- `src/main.c`: runner/CLI
- `data/processes_example.csv`: escenario sugerido
- `data/processes_stress_mlfq.csv`: escenario grande de validacion
- `out/results.csv`: salida CSV generada
- `out/report.html`: reporte HTML generado por defecto
- `docs/analysis.md`: analisis conceptual
- `docs/mlfq_theoretical_validation.md`: resultados teoricos esperados del escenario grande
- `docs/project_justification.md`: sustento tecnico del proyecto

## Formato de entrada

CSV con cabecera:

```csv
PID,Arrival,Burst
P1,0,8
P2,1,4
```

## Build y ejecucion

```bash
cd "/Users/juanestebanmosquera/Documents/UdeA/2026-1/SO/Reto3"
make
./mlfq_sim --input data/processes_example.csv --output out/results.csv --html-output out/report.html --boost 20
```

Salidas generadas:

- `out/results.csv`
- `out/report.html`

## Metricas calculadas

Por proceso:

- `Response = first_response_time - arrival_time`
- `Turnaround = finish_time - arrival_time`
- `Waiting = turnaround - burst_time`

## Reporte HTML

Cada ejecucion puede generar un reporte HTML autocontenido con:

- resumen de configuracion y metricas,
- Gantt de CPU,
- linea de vida por proceso,
- tabla de eventos (arrival, dispatch, preemption, demotion, boost, finish),
- snapshots del estado de `Q0`, `Q1` y `Q2` por ciclo.

Parametro opcional disponible:

- `--html-output ruta/reporte.html`

## Escenario grande de prueba

Para validacion fuerte del scheduler (12 procesos, llegadas escalonadas y cargas mixtas):

```bash
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress.csv --html-output out/report_stress.html --boost 20
```

Referencias:

- `docs/mlfq_theoretical_validation.md`
- `docs/project_justification.md`
- `docs/guia_reproduccion_ubuntu.md`

## Notas de comportamiento

- El simulador es **preemptivo entre colas**: si llega trabajo a una cola de mayor prioridad, puede interrumpir a uno en cola inferior en el siguiente tick.
- El `priority boost` reinicia la prioridad de procesos activos y en espera a `Q0`, y reinicia su contador de quantum del nivel.
- `--boost 0` deshabilita boost.
