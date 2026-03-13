# Guion extenso de sustentacion: caso de prueba MLFQ contado como historia

## Como usar este documento

Este texto esta pensado para lectura en publico. Tiene doble objetivo:

1. Que cualquier persona entienda la idea general del scheduler.
2. Que un jurado tecnico vea que los conceptos estan correctamente aplicados.

Puedes leerlo completo (12-15 minutos) o recortar por secciones.

---

## 1) Apertura (lectura sugerida)

Buenas tardes.

Hoy no les voy a mostrar primero una tabla de numeros ni un bloque de codigo. Hoy les voy a contar una historia.

Imaginen una ciudad en hora pico, con una central de emergencias que recibe casos de distinta urgencia. Algunos se resuelven rapido; otros requieren mucho tiempo de atencion. La central tiene personal limitado, pero debe tomar decisiones justas y eficientes en cada minuto.

Eso, exactamente, es lo que hace un scheduler en un sistema operativo: decide quien usa la CPU, cuando, por cuanto tiempo y bajo que prioridad.

Nuestro proyecto implementa esa toma de decisiones usando la politica **Multi-Level Feedback Queue (MLFQ)**, una estrategia que combina prioridad, rotacion y mecanismos de equidad para evitar que procesos largos queden olvidados.

El valor de esta sustentacion no es solo decir que el programa funciona, sino mostrar **por que** funciona, **como** se comporta bajo carga y **que evidencia** respalda cada afirmacion.

---

## 2) El escenario real contado como contexto

Pensemos que cada proceso es una solicitud de servicio en la central:

- `P1` y `P8` son casos complejos y largos.
- `P11` es un caso muy corto que puede cerrarse rapido.
- Los demas son casos intermedios, llegando en momentos diferentes.

En el simulador, estas solicitudes llegan con una hora de llegada (`arrival_time`) y una carga de trabajo (`burst_time`).

El caso de prueba grande esta definido en `data/processes_stress_mlfq.csv` con 12 procesos:

- `P1,0,18`
- `P2,0,5`
- `P3,1,9`
- `P4,2,4`
- `P5,3,12`
- `P6,5,7`
- `P7,6,3`
- `P8,8,15`
- `P9,10,6`
- `P10,12,10`
- `P11,14,2`
- `P12,16,11`

No es un escenario trivial: mezcla procesos cortos, medianos y largos, con llegadas escalonadas. Es ideal para forzar decisiones reales del scheduler.

---

## 3) Reglas del juego (con conceptos tecnicos correctos)

Nuestra central trabaja con tres niveles de prioridad:

- `Q0`: prioridad alta, quantum `2`
- `Q1`: prioridad media, quantum `4`
- `Q2`: prioridad baja, quantum `8`

Reglas de planificacion:

1. **Prioridad estricta entre colas**: siempre se ejecuta la cola de mayor prioridad no vacia.
2. **Round Robin por cola**: dentro de cada nivel se rota en orden FIFO.
3. **Democion**: si un proceso consume todo su quantum sin terminar, baja un nivel.
4. **No democion**: si termina antes de agotar quantum, finaliza donde esta.
5. **Priority boost periodico**: cada 20 ciclos (`--boost 20`) todos los procesos pendientes vuelven a `Q0`.

Este quinto punto es clave: evita que procesos largos se queden indefinidamente relegados en `Q2` (starvation).

---

## 4) La historia por actos (narrativa + tecnica)

## Acto I: Llegadas y primer filtro rapido

La simulacion arranca en `t=0`. Llegan `P1` y `P2`.

Como toda llegada entra por `Q0`, ambos reciben una primera atencion corta de 2 ciclos como maximo. Esto tiene una logica clara: dar respuesta inicial rapida a todo lo nuevo.

A medida que avanzan los ciclos, entran mas procesos (`P3`, `P4`, `P5`, ...). Cada uno entra a `Q0`, recibe su turno corto, y si no termina, baja a `Q1`.

En lenguaje operativo:

- procesos cortos tienden a salir temprano,
- procesos largos son "detectados" y enviados a niveles de mayor quantum,
- la cola alta no se monopoliza por trabajos extensos.

## Acto II: La presion crece, aparece el riesgo de injusticia

Ya no hay pocos procesos: hay cola acumulada, distintas longitudes de trabajo y competencia real por CPU.

Sin un mecanismo de compensacion, los procesos que caen a `Q2` podrian tardar demasiado en volver a recibir CPU si siguen llegando nuevos en `Q0`.

Ese problema es **starvation**.

## Acto III: Entra la politica de equidad (boost)

En `t=20` ocurre el primer **priority boost**.

Narrativamente: la central detiene un momento la inercia del sistema y reevalua todo. Tecnica y formalmente: los pendientes regresan a `Q0` y su contador de quantum se reinicia.

Esto se repite en `t=40`, `t=60`, `t=80` y `t=100`.

Consecuencia:

- nadie queda olvidado permanentemente en cola baja,
- se mejora equidad global,
- se conserva capacidad de reaccion frente a procesos nuevos.

## Acto IV: Cierre del sistema

La simulacion finaliza en `t=102`, que coincide con la suma total de CPU requerida. Eso confirma que:

- no hubo perdida de trabajo,
- todos los procesos terminaron,
- el scheduler aplico reglas consistentes de inicio a fin.

---

## 5) Traduccion directa de la narrativa al codigo

La historia no es metafora suelta: corresponde linea por linea al comportamiento implementado.

- Llegadas: `enqueue_new_arrivals` en `src/scheduler.c`.
- Seleccion por prioridad: `choose_next_process` recorre de `Q0` a `Q2`.
- Round Robin: colas circulares `ReadyQueue` con `queue_push` y `queue_pop`.
- Democion por quantum: bloque que compara `quantum_used` con `config->quantums[current_queue]`.
- Boost periodico: `apply_priority_boost` cuando `time % boost_period == 0`.
- Registro de tiempos: `start_time`, `first_response_time`, `finish_time`.

Es decir: cada escena de la historia tiene una implementacion concreta verificable.

---

## 6) Resultados esperados del caso de prueba grande

Resultados teoricos (documentados en `docs/mlfq_theoretical_validation.md`):

- Response promedio: `4.58`
- Turnaround promedio: `61.75`
- Waiting promedio: `53.25`

Interpretacion para publico general:

- **Response**: cuanto tarda en recibir primera atencion.
- **Turnaround**: tiempo total en el sistema desde que llega hasta que termina.
- **Waiting**: tiempo acumulado de espera sin CPU.

Interpretacion tecnica:

- Respuesta inicial baja para muchos procesos, por efecto de `Q0` con quantum pequeno.
- Turnaround y waiting altos en procesos largos, esperable bajo alta competencia y carga total grande.
- Equidad sostenida por boosts periodicos, reduciendo riesgo de starvation.

---

## 7) Lectura sugerida de procesos representativos

### Caso `P11` (burst 2): proceso corto

`P11` llega tarde (`arrival=14`) pero requiere poco CPU (`burst=2`).

Que muestra:

- Se beneficia de la politica de prioridad alta y turnos cortos.
- Obtiene finalizacion rapida una vez entra en servicio.
- Representa el comportamiento esperado para trabajos interactivos/cortos.

### Caso `P1` (burst 18): proceso largo

`P1` llega primero y necesita mucho CPU.

Que muestra:

- Recibe CPU desde el inicio, pero no monopoliza `Q0`.
- Al consumir quantum, es demovido progresivamente.
- Con boosts vuelve a competir en `Q0`, evitando abandono permanente.

### Caso `P8` (burst 15): largo y tardio

`P8` no llega al inicio (`arrival=8`) y aun asi completa correctamente.

Que muestra:

- El sistema soporta llegadas tardias con carga alta.
- MLFQ mantiene orden y finalizacion total aun en escenarios exigentes.

---

## 8) Mensaje de sustentacion tecnica (bloque listo para leer)

Lo mas importante de este trabajo es que no se queda en una simulacion visual ni en una intuicion teorica.

Tenemos:

1. Reglas formales de MLFQ implementadas en C.
2. Escenario base y escenario grande de estres.
3. Resultados exportables a CSV para verificacion externa.
4. Metricas estandar de scheduling calculadas por proceso y promedio.
5. Coherencia entre narrativa conceptual, algoritmo y evidencia numerica.

Por eso, la conclusion no es "parece que funciona", sino: **funciona, es reproducible, y su comportamiento es explicable tecnicamente**.

---

## 9) Cierre oral (version extensa)

Si tuviera que resumir este proyecto en una sola frase, diria lo siguiente:

> Construimos una politica de decisiones en tiempo discreto que balancea rapidez de respuesta, uso eficiente de CPU y justicia entre procesos, y demostramos ese balance con evidencia reproducible.

MLFQ no promete que todos terminen igual de rapido; promete algo mas realista: tratar diferente a cargas diferentes sin perder control del sistema. Y eso es precisamente lo que observamos en el caso de prueba grande.

Los procesos cortos reciben atencion temprana. Los largos siguen avanzando sin bloquear al resto. Y el boost periodico evita que la prioridad se convierta en exclusion.

En terminos de ingenieria, el proyecto queda sustentado en tres pilares:

- Implementacion correcta.
- Medicion cuantitativa.
- Documentacion defendible.

En terminos academicos, demuestra comprension de planificacion de CPU, modelado por eventos discretos y evaluacion de rendimiento.

Y en terminos practicos, deja una base lista para evolucionar: pruebas automatizadas, trazas tipo Gantt, y comparativas entre politicas.

Muchas gracias.

---

## 10) Preguntas del jurado y respuestas sugeridas

### Pregunta 1: "Por que usar boost cada 20 ciclos y no otro valor?"

Respuesta sugerida:

Se eligio 20 como punto de equilibrio para el escenario: suficiente para que las colas reflejen prioridad y, al mismo tiempo, evitar starvation prolongado. El valor es parametrico (`--boost N`), por lo tanto se puede estudiar sensibilidad del sistema variando N.

### Pregunta 2: "Que pasa si boost es 0?"

Respuesta sugerida:

Se deshabilita el boost. El scheduler sigue siendo funcional, pero aumenta riesgo de starvation para procesos en colas bajas si siguen llegando procesos de alta prioridad.

### Pregunta 3: "Como garantizan reproducibilidad?"

Respuesta sugerida:

Con entrada CSV fija, reglas deterministas y exportacion de resultados en `out/results*.csv`. El mismo input produce la misma salida bajo la misma configuracion.

### Pregunta 4: "Por que Round Robin en cada cola?"

Respuesta sugerida:

Porque distribuye CPU de forma justa entre procesos de la misma prioridad y evita monopolios internos dentro de cada nivel.

### Pregunta 5: "Cual es la principal limitacion actual?"

Respuesta sugerida:

Falta una suite automatica de pruebas de regresion y una traza visual Gantt por tick. Es una mejora natural sobre una base que ya cumple funcionalmente.

---

## 11) Version corta (2-3 minutos) para cierre rapido

Implementamos un simulador MLFQ en C con tres colas (`Q0/Q1/Q2`), quantums `2/4/8`, democion por consumo completo de quantum y boost periodico cada 20 ciclos.

Probamos un escenario grande de 12 procesos con llegadas y rafagas heterogeneas. El sistema completo finaliza en `t=102`, calcula metricas por proceso y exporta resultados a CSV.

Los promedios observados y teoricos coinciden con el comportamiento esperado: respuesta inicial buena para procesos cortos, degradacion controlada para procesos largos y mitigacion de starvation gracias al boost.

En resumen: el proyecto cumple la especificacion, es reproducible, esta bien estructurado y su comportamiento es tecnicamente justificable.

