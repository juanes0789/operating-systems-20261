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

typedef enum {
    TRACE_EVENT_ARRIVAL,
    TRACE_EVENT_DISPATCH,
    TRACE_EVENT_PREEMPT,
    TRACE_EVENT_DEMOTION,
    TRACE_EVENT_BOOST,
    TRACE_EVENT_FINISH
} TraceEventType;

typedef struct {
    int start_time;
    int end_time;
    int process_index;
    int queue_level;
} CpuSlice;

typedef struct {
    int time;
    TraceEventType type;
    int process_index;
    int from_queue;
    int to_queue;
    char description[128];
} TraceEvent;

typedef struct {
    int time;
    int running_index;
    char *running_label;
    char *queue_labels[MLFQ_LEVELS];
} QueueSnapshot;

typedef struct {
    CpuSlice *slices;
    size_t slice_count;
    size_t slice_capacity;
    TraceEvent *events;
    size_t event_count;
    size_t event_capacity;
    QueueSnapshot *snapshots;
    size_t snapshot_count;
    size_t snapshot_capacity;
    int total_time;
} SimulationTrace;

int load_processes_csv(const char *path, Process **processes, size_t *count);
int run_mlfq_simulation(
    Process *processes,
    size_t count,
    const SchedulerConfig *config,
    SimulationTrace *trace
);
void compute_summary(const Process *processes, size_t count, SimulationSummary *summary);
int write_results_csv(const char *path, const Process *processes, size_t count);
int write_html_report(
    const char *path,
    const Process *processes,
    size_t count,
    const SchedulerConfig *config,
    const SimulationSummary *summary,
    const SimulationTrace *trace
);
void print_results(const Process *processes, size_t count, const SimulationSummary *summary);
void free_simulation_trace(SimulationTrace *trace);

#endif

