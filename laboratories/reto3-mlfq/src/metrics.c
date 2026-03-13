#include "scheduler.h"

#include <stdio.h>

static int response_time(const Process *p) {
    return p->first_response_time - p->arrival_time;
}

static int turnaround_time(const Process *p) {
    return p->finish_time - p->arrival_time;
}

static int waiting_time(const Process *p) {
    return turnaround_time(p) - p->burst_time;
}

void compute_summary(const Process *processes, size_t count, SimulationSummary *summary) {
    size_t i;
    double total_response = 0.0;
    double total_turnaround = 0.0;
    double total_waiting = 0.0;

    if (!summary) {
        return;
    }

    summary->average_response = 0.0;
    summary->average_turnaround = 0.0;
    summary->average_waiting = 0.0;

    if (!processes || count == 0) {
        return;
    }

    for (i = 0; i < count; ++i) {
        total_response += (double)response_time(&processes[i]);
        total_turnaround += (double)turnaround_time(&processes[i]);
        total_waiting += (double)waiting_time(&processes[i]);
    }

    summary->average_response = total_response / (double)count;
    summary->average_turnaround = total_turnaround / (double)count;
    summary->average_waiting = total_waiting / (double)count;
}

void print_results(const Process *processes, size_t count, const SimulationSummary *summary) {
    size_t i;

    if (!processes || count == 0) {
        return;
    }

    printf("PID\tArrival\tBurst\tStart\tFinish\tResponse\tTurnaround\tWaiting\n");
    for (i = 0; i < count; ++i) {
        int response = processes[i].first_response_time - processes[i].arrival_time;
        int turnaround = processes[i].finish_time - processes[i].arrival_time;
        int waiting = turnaround - processes[i].burst_time;

        printf(
            "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
            processes[i].pid,
            processes[i].arrival_time,
            processes[i].burst_time,
            processes[i].start_time,
            processes[i].finish_time,
            response,
            turnaround,
            waiting
        );
    }

    if (summary) {
        printf(
            "\nPromedios -> Response: %.2f | Turnaround: %.2f | Waiting: %.2f\n",
            summary->average_response,
            summary->average_turnaround,
            summary->average_waiting
        );
    }
}

