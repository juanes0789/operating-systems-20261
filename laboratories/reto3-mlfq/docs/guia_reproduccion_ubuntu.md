# Guia de reproduccion en Ubuntu (copiar y pegar)

Esta guia esta pensada para que cualquier persona clone el repo y ejecute el simulador MLFQ en Ubuntu sin configuraciones manuales extra.

## 1) Instalar dependencias del sistema

```bash
sudo apt update
sudo apt install -y build-essential make git
```

Verificacion rapida:

```bash
gcc --version
make --version
git --version
```

## 2) Clonar el repositorio

Reemplaza `<URL_DEL_REPO>` por la URL real de tu repositorio.

```bash
git clone <URL_DEL_REPO>
cd Reto3
```

## 3) Compilar el proyecto

```bash
make clean
make
```

Si todo sale bien, se genera el ejecutable `mlfq_sim` en la raiz del proyecto.

## 4) Ejecutar el caso base (ejemplo del enunciado)

```bash
./mlfq_sim --input data/processes_example.csv --output out/results.csv --boost 20
```

Archivo generado:

- `out/results.csv`

## 5) Ejecutar el caso grande de estres

```bash
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress.csv --boost 20
```

Archivo generado:

- `out/results_stress.csv`

## 6) Validar salida esperada del caso grande

La referencia teorica esta en:

- `docs/mlfq_theoretical_validation.md`

Puedes inspeccionar el resultado generado con:

```bash
cat out/results_stress.csv
```

## 7) Experimentos recomendados (analisis)

### Sin boost (posible starvation)

```bash
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress_boost0.csv --boost 0
```

### Boost frecuente

```bash
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress_boost5.csv --boost 5
```

### Boost moderado (referencia)

```bash
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress_boost20.csv --boost 20
```

## 8) Comandos minimos para README del repo

Si quieres una version ultra-corta para la portada del repo, usa esto:

```bash
sudo apt update
sudo apt install -y build-essential make git
git clone <URL_DEL_REPO>
cd Reto3
make clean
make
./mlfq_sim --input data/processes_example.csv --output out/results.csv --boost 20
./mlfq_sim --input data/processes_stress_mlfq.csv --output out/results_stress.csv --boost 20
```

## 9) Limpieza del proyecto

```bash
make clean
```

