#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program) {
    fprintf(
        stderr,
        "Uso: %s [--input archivo.csv] [--output results.csv] [--html-output reporte.html] [--boost N]\n",
        program
    );
}

int main(int argc, char **argv) {
    const char *input_path = "data/processes_example.csv";
    const char *output_path = "out/results.csv";
    const char *html_output_path = "out/report.html";
    int boost_period = 20;
    int i;
    Process *processes = NULL;
    size_t count = 0;
    SchedulerConfig config = {{2, 4, 8}, 20};
    SimulationSummary summary;
    SimulationTrace trace = {0};

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            input_path = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--html-output") == 0 && i + 1 < argc) {
            html_output_path = argv[++i];
        } else if (strcmp(argv[i], "--boost") == 0 && i + 1 < argc) {
            boost_period = atoi(argv[++i]);
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (boost_period < 0) {
        fprintf(stderr, "Error: --boost debe ser >= 0 (0 deshabilita boost).\n");
        return 1;
    }

    config.boost_period = boost_period;

    if (load_processes_csv(input_path, &processes, &count) != 0) {
        fprintf(stderr, "Error leyendo archivo de procesos: %s\n", input_path);
        return 1;
    }

    if (run_mlfq_simulation(processes, count, &config, &trace) != 0) {
        fprintf(stderr, "Error durante la simulacion MLFQ.\n");
        free(processes);
        free_simulation_trace(&trace);
        return 1;
    }

    compute_summary(processes, count, &summary);
    print_results(processes, count, &summary);

    if (write_results_csv(output_path, processes, count) != 0) {
        fprintf(stderr, "Error escribiendo resultados CSV: %s\n", output_path);
        free(processes);
        free_simulation_trace(&trace);
        return 1;
    }

    if (write_html_report(html_output_path, processes, count, &config, &summary, &trace) != 0) {
        fprintf(stderr, "Error escribiendo reporte HTML: %s\n", html_output_path);
        free(processes);
        free_simulation_trace(&trace);
        return 1;
    }

    printf("\nResultados exportados en: %s\n", output_path);
    printf("Reporte HTML exportado en: %s\n", html_output_path);

    free(processes);
    free_simulation_trace(&trace);
    return 0;
}

