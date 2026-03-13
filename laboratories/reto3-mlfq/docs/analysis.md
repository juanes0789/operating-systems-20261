# Analisis de parametros MLFQ

## 1) Que ocurre si el boost es muy frecuente?

- Muchos procesos regresan continuamente a `Q0`.
- El sistema se comporta mas parecido a Round Robin de alta prioridad.
- Se reduce el beneficio de tener colas bajas para trabajos largos.
- Mejora la equidad, pero puede aumentar el overhead de cambios de contexto.

## 2) Que ocurre si no existe boost?

- Los procesos CPU-bound pueden quedarse mucho tiempo en `Q2`.
- Si siguen llegando procesos interactivos/cortos a `Q0`, los de baja prioridad pueden esperar demasiado.
- Aumenta riesgo de starvation para procesos largos.

## 3) Como afecta un quantum pequeno en la cola de mayor prioridad?

- Mejora la latencia inicial y el response time de procesos nuevos/interactivos.
- Incrementa cantidad de expulsiones/demociones y cambios de contexto.
- Si es demasiado pequeno, puede degradar throughput por overhead.

## 4) Puede haber starvation?

- Si no hay boost, si: procesos en colas bajas pueden casi no ejecutarse.
- Con boost periodico, el starvation se mitiga porque todos vuelven a `Q0` cada `S` ciclos.
- Un `S` muy grande deja periodos largos de espera; uno moderado balancea mejor equidad y eficiencia.

