# Operating Systems 2026-1

Repositorio de la materia de **Sistemas Operativos (2026-1)**.

## Estructura principal

- `laboratories/`: laboratorios y retos de la materia
- `ticket_system/`: proyecto adicional del repositorio

## Laboratorios

### `laboratories/reto3-mlfq`
Simulador de scheduler **MLFQ (Multi-Level Feedback Queue)** implementado en C.

Incluye:

- 3 colas de prioridad (`Q0`, `Q1`, `Q2`)
- quantums `2`, `4`, `8`
- democion por uso completo de quantum
- priority boost periodico
- metricas por proceso (`Response`, `Turnaround`, `Waiting`)
- exportacion de resultados a CSV
- **reporte HTML automatico** por ejecucion con:
  - Gantt por colas (`Q0`, `Q1`, `Q2`)
  - PID visible en cada bloque
  - linea de vida por proceso
  - eventos del scheduler
  - snapshots del estado de colas

## Ejecucion rapida del laboratorio MLFQ

```bash
cd laboratories/reto3-mlfq
make clean
make
./mlfq_sim --input data/processes_example.csv --output out/results.csv --html-output out/report.html --boost 20
```

## Documentacion publica del laboratorio

Dentro de `laboratories/reto3-mlfq/docs/` se incluyen:

- guia de reproduccion en Ubuntu
- validacion teorica del escenario grande

## Nota

El detalle operativo del laboratorio, su arquitectura, ejecucion y casos de prueba publicos estan documentados en:

- `laboratories/reto3-mlfq/README.md`
