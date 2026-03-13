# Escenario grande de prueba (MLFQ)

Este documento define un escenario de estres para el simulador y los resultados esperados teoricamente bajo estas reglas:

- Colas: `Q0`, `Q1`, `Q2`
- Quantums: `Q0=2`, `Q1=4`, `Q2=8`
- Seleccion por mayor prioridad no vacia
- Round Robin dentro de cada cola
- Democion al agotar quantum
- Sin democion si el proceso termina antes
- Priority boost global cada `20` ciclos
- Desempate por orden de llegada a la cola (determinista por orden del CSV)

## Entrada del escenario

Archivo: `data/processes_stress_mlfq.csv`

```csv
PID,Arrival,Burst
P1,0,18
P2,0,5
P3,1,9
P4,2,4
P5,3,12
P6,5,7
P7,6,3
P8,8,15
P9,10,6
P10,12,10
P11,14,2
P12,16,11
```

## Lectura teorica por fases

- `t=0..20`: entran procesos progresivamente; en `Q0` todos reciben rafagas de 2 ticks y los que no terminan se demueven a `Q1`.
- `t=20`: se aplica boost. Todos los pendientes vuelven a `Q0`. `P11` (corto) termina de inmediato en `t=22`.
- `t=20..40`: nueva ronda de `Q0` favorece respuesta de procesos tardios y redistribuye trabajo largo a `Q1`.
- `t=40`: segundo boost. Procesos largos que ya habian bajado recuperan prioridad temporal.
- `t=60`, `t=80`, `t=100`: boosts sucesivos evitan starvation y fuerzan nuevas rondas en `Q0`.
- Finalizacion total en `t=102`, igual a la suma de bursts (CPU siempre ocupada).

## Resultados esperados teoricamente

Estos valores son los esperados para este modelo MLFQ determinista:

| PID | Arrival | Burst | Start | Finish | Response | Turnaround | Waiting |
|---|---:|---:|---:|---:|---:|---:|---:|
| P1 | 0 | 18 | 0 | 101 | 0 | 101 | 83 |
| P2 | 0 | 5 | 2 | 49 | 2 | 49 | 44 |
| P3 | 1 | 9 | 4 | 83 | 3 | 82 | 73 |
| P4 | 2 | 4 | 6 | 32 | 4 | 30 | 26 |
| P5 | 3 | 12 | 8 | 94 | 5 | 91 | 79 |
| P6 | 5 | 7 | 10 | 72 | 5 | 67 | 60 |
| P7 | 6 | 3 | 12 | 37 | 6 | 31 | 28 |
| P8 | 8 | 15 | 14 | 102 | 6 | 94 | 79 |
| P9 | 10 | 6 | 16 | 63 | 6 | 53 | 47 |
| P10 | 12 | 10 | 18 | 75 | 6 | 63 | 53 |
| P11 | 14 | 2 | 20 | 22 | 6 | 8 | 6 |
| P12 | 16 | 11 | 22 | 88 | 6 | 72 | 61 |

Promedios teoricos del escenario:

- Response promedio: `4.58`
- Turnaround promedio: `61.75`
- Waiting promedio: `53.25`

## Criterio de validacion

La validacion pasa si `out/results_stress.csv` coincide exactamente con la tabla anterior para:

- `Start`, `Finish`
- `Response`, `Turnaround`, `Waiting`

Comando de reproduccion:

```bash
cd "/Users/juanestebanmosquera/Documents/UdeA/2026-1/SO/Reto3"
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress.csv --boost 20
```

