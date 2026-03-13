#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>

#define MLFQ_LEVELS 3
#define PID_MAX_LEN 32

typedef struct {
    char pid[PID_MAX_LEN];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int finish_time;
    int first_response_time;
    int current_queue;
    int quantum_used;
} Process;

typedef struct {
    int *items;
    int head;
    int tail;
    int size;
    int capacity;
} ReadyQueue;

typedef struct {
    int quantums[MLFQ_LEVELS];
    int boost_period;
} SchedulerConfig;

typedef struct {
    Process *processes;
    size_t process_count;
    SchedulerConfig config;
} SimulationInput;

typedef struct {
    double average_response;
    double average_turnaround;
    double average_waiting;
} SimulationSummary;

int load_processes_csv(const char *path, Process **processes, size_t *count);
int run_mlfq_simulation(Process *processes, size_t count, const SchedulerConfig *config);
void compute_summary(const Process *processes, size_t count, SimulationSummary *summary);
int write_results_csv(const char *path, const Process *processes, size_t count);
void print_results(const Process *processes, size_t count, const SimulationSummary *summary);

#endif

