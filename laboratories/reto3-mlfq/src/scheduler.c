#include "scheduler.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int queue_init(ReadyQueue *queue, int capacity) {
    queue->items = (int *)malloc((size_t)capacity * sizeof(int));
    if (!queue->items) {
        return -1;
    }
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->capacity = capacity;
    return 0;
}

static void queue_destroy(ReadyQueue *queue) {
    free(queue->items);
    queue->items = NULL;
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->capacity = 0;
}

static int queue_push(ReadyQueue *queue, int value) {
    if (queue->size == queue->capacity) {
        return -1;
    }
    queue->items[queue->tail] = value;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    return 0;
}

static int queue_pop(ReadyQueue *queue, int *value) {
    if (queue->size == 0) {
        return -1;
    }
    *value = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return 0;
}

static int queue_empty(const ReadyQueue *queue) {
    return queue->size == 0;
}

static void reset_process(Process *p) {
    p->remaining_time = p->burst_time;
    p->start_time = -1;
    p->finish_time = -1;
    p->first_response_time = -1;
    p->current_queue = 0;
    p->quantum_used = 0;
}

void free_simulation_trace(SimulationTrace *trace) {
    size_t i;

    if (!trace) {
        return;
    }

    for (i = 0; i < trace->snapshot_count; ++i) {
        int level;
        free(trace->snapshots[i].running_label);
        trace->snapshots[i].running_label = NULL;
        for (level = 0; level < MLFQ_LEVELS; ++level) {
            free(trace->snapshots[i].queue_labels[level]);
            trace->snapshots[i].queue_labels[level] = NULL;
        }
    }

    free(trace->slices);
    free(trace->events);
    free(trace->snapshots);

    trace->slices = NULL;
    trace->events = NULL;
    trace->snapshots = NULL;
    trace->slice_count = 0;
    trace->slice_capacity = 0;
    trace->event_count = 0;
    trace->event_capacity = 0;
    trace->snapshot_count = 0;
    trace->snapshot_capacity = 0;
    trace->total_time = 0;
}

static char *duplicate_string(const char *text) {
    size_t len;
    char *copy;

    if (!text) {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1);
    if (!copy) {
        return NULL;
    }

    memcpy(copy, text, len + 1);
    return copy;
}

static int ensure_slice_capacity(SimulationTrace *trace) {
    size_t new_capacity;
    CpuSlice *new_items;

    if (!trace) {
        return 0;
    }
    if (trace->slice_count < trace->slice_capacity) {
        return 0;
    }

    new_capacity = trace->slice_capacity == 0 ? 32 : trace->slice_capacity * 2;
    new_items = (CpuSlice *)realloc(trace->slices, new_capacity * sizeof(CpuSlice));
    if (!new_items) {
        return -1;
    }

    trace->slices = new_items;
    trace->slice_capacity = new_capacity;
    return 0;
}

static int ensure_event_capacity(SimulationTrace *trace) {
    size_t new_capacity;
    TraceEvent *new_items;

    if (!trace) {
        return 0;
    }
    if (trace->event_count < trace->event_capacity) {
        return 0;
    }

    new_capacity = trace->event_capacity == 0 ? 64 : trace->event_capacity * 2;
    new_items = (TraceEvent *)realloc(trace->events, new_capacity * sizeof(TraceEvent));
    if (!new_items) {
        return -1;
    }

    trace->events = new_items;
    trace->event_capacity = new_capacity;
    return 0;
}

static int ensure_snapshot_capacity(SimulationTrace *trace) {
    size_t new_capacity;
    QueueSnapshot *new_items;

    if (!trace) {
        return 0;
    }
    if (trace->snapshot_count < trace->snapshot_capacity) {
        return 0;
    }

    new_capacity = trace->snapshot_capacity == 0 ? 32 : trace->snapshot_capacity * 2;
    new_items = (QueueSnapshot *)realloc(trace->snapshots, new_capacity * sizeof(QueueSnapshot));
    if (!new_items) {
        return -1;
    }

    trace->snapshots = new_items;
    trace->snapshot_capacity = new_capacity;
    return 0;
}

static int append_slice(
    SimulationTrace *trace,
    int start_time,
    int end_time,
    int process_index,
    int queue_level
) {
    CpuSlice *last;

    if (!trace) {
        return 0;
    }

    if (trace->slice_count > 0) {
        last = &trace->slices[trace->slice_count - 1];
        if (
            last->process_index == process_index &&
            last->queue_level == queue_level &&
            last->end_time == start_time
        ) {
            last->end_time = end_time;
            return 0;
        }
    }

    if (ensure_slice_capacity(trace) != 0) {
        return -1;
    }

    trace->slices[trace->slice_count].start_time = start_time;
    trace->slices[trace->slice_count].end_time = end_time;
    trace->slices[trace->slice_count].process_index = process_index;
    trace->slices[trace->slice_count].queue_level = queue_level;
    trace->slice_count++;
    return 0;
}

static int append_event(
    SimulationTrace *trace,
    int time,
    TraceEventType type,
    int process_index,
    int from_queue,
    int to_queue,
    const char *description
) {
    TraceEvent *event;

    if (!trace) {
        return 0;
    }

    if (ensure_event_capacity(trace) != 0) {
        return -1;
    }

    event = &trace->events[trace->event_count++];
    event->time = time;
    event->type = type;
    event->process_index = process_index;
    event->from_queue = from_queue;
    event->to_queue = to_queue;
    if (description) {
        snprintf(event->description, sizeof(event->description), "%s", description);
    } else {
        event->description[0] = '\0';
    }
    return 0;
}

static int build_queue_label(
    const ReadyQueue *queue,
    const Process *processes,
    char **label
) {
    size_t buffer_size = 1024;
    size_t length = 0;
    int i;
    char *buffer;

    if (!label) {
        return -1;
    }

    buffer = (char *)malloc(buffer_size);
    if (!buffer) {
        return -1;
    }
    buffer[0] = '\0';

    if (!queue || queue->size == 0) {
        snprintf(buffer, buffer_size, "vacia");
        *label = buffer;
        return 0;
    }

    for (i = 0; i < queue->size; ++i) {
        int idx = queue->items[(queue->head + i) % queue->capacity];
        int written = snprintf(
            buffer + length,
            buffer_size - length,
            "%s%s(rem=%d)",
            i == 0 ? "" : ", ",
            processes[idx].pid,
            processes[idx].remaining_time
        );

        if (written < 0) {
            free(buffer);
            return -1;
        }

        if ((size_t)written >= buffer_size - length) {
            char *new_buffer;
            buffer_size *= 2;
            new_buffer = (char *)realloc(buffer, buffer_size);
            if (!new_buffer) {
                free(buffer);
                return -1;
            }
            buffer = new_buffer;
            i--;
            continue;
        }

        length += (size_t)written;
    }

    *label = buffer;
    return 0;
}

static int append_snapshot(
    SimulationTrace *trace,
    int time,
    int running,
    const Process *processes,
    const ReadyQueue queues[MLFQ_LEVELS]
) {
    QueueSnapshot *snapshot;
    int level;
    char running_buffer[128];

    if (!trace) {
        return 0;
    }

    if (ensure_snapshot_capacity(trace) != 0) {
        return -1;
    }

    snapshot = &trace->snapshots[trace->snapshot_count];
    snapshot->time = time;
    snapshot->running_index = running;
    snapshot->running_label = NULL;
    for (level = 0; level < MLFQ_LEVELS; ++level) {
        snapshot->queue_labels[level] = NULL;
    }

    if (running >= 0) {
        snprintf(
            running_buffer,
            sizeof(running_buffer),
            "%s (Q%d, rem=%d, q_used=%d)",
            processes[running].pid,
            processes[running].current_queue,
            processes[running].remaining_time,
            processes[running].quantum_used
        );
    } else {
        snprintf(running_buffer, sizeof(running_buffer), "IDLE");
    }

    snapshot->running_label = duplicate_string(running_buffer);
    if (!snapshot->running_label) {
        return -1;
    }

    for (level = 0; level < MLFQ_LEVELS; ++level) {
        if (build_queue_label(&queues[level], processes, &snapshot->queue_labels[level]) != 0) {
            int cleanup_level;
            free(snapshot->running_label);
            snapshot->running_label = NULL;
            for (cleanup_level = 0; cleanup_level < level; ++cleanup_level) {
                free(snapshot->queue_labels[cleanup_level]);
                snapshot->queue_labels[cleanup_level] = NULL;
            }
            return -1;
        }
    }

    trace->snapshot_count++;
    return 0;
}

static int has_higher_ready_queue(const ReadyQueue queues[MLFQ_LEVELS], int queue_level) {
    int level;
    for (level = 0; level < queue_level; ++level) {
        if (!queue_empty(&queues[level])) {
            return 1;
        }
    }
    return 0;
}

static int choose_next_process(ReadyQueue queues[MLFQ_LEVELS], Process *processes) {
    int level;
    int index;

    for (level = 0; level < MLFQ_LEVELS; ++level) {
        if (!queue_empty(&queues[level])) {
            if (queue_pop(&queues[level], &index) != 0) {
                return -1;
            }
            processes[index].current_queue = level;
            return index;
        }
    }
    return -1;
}

static int enqueue_new_arrivals(
    Process *processes,
    size_t count,
    int current_time,
    ReadyQueue queues[MLFQ_LEVELS],
    SimulationTrace *trace
) {
    size_t i;
    for (i = 0; i < count; ++i) {
        if (processes[i].arrival_time == current_time) {
            char description[128];

            processes[i].current_queue = 0;
            processes[i].quantum_used = 0;
            if (queue_push(&queues[0], (int)i) != 0) {
                return -1;
            }
            snprintf(
                description,
                sizeof(description),
                "%s llega al sistema y entra a Q0 con burst=%d",
                processes[i].pid,
                processes[i].burst_time
            );
            if (
                append_event(
                    trace,
                    current_time,
                    TRACE_EVENT_ARRIVAL,
                    (int)i,
                    -1,
                    0,
                    description
                ) != 0
            ) {
                return -1;
            }
        }
    }
    return 0;
}

static int apply_priority_boost(
    Process *processes,
    size_t count,
    ReadyQueue queues[MLFQ_LEVELS],
    int *running,
    int current_time,
    SimulationTrace *trace
) {
    int level;
    int idx;
    int affected = 0;
    char description[128];

    if (*running >= 0) {
        affected++;
        processes[*running].current_queue = 0;
        processes[*running].quantum_used = 0;
        if (queue_push(&queues[0], *running) != 0) {
            return -1;
        }
        *running = -1;
    }

    for (level = 1; level < MLFQ_LEVELS; ++level) {
        while (!queue_empty(&queues[level])) {
            affected++;
            if (queue_pop(&queues[level], &idx) != 0) {
                return -1;
            }
            processes[idx].current_queue = 0;
            processes[idx].quantum_used = 0;
            if (queue_push(&queues[0], idx) != 0) {
                return -1;
            }
        }
    }

    for (level = 0; level < MLFQ_LEVELS; ++level) {
        int qsize = queues[level].size;
        int moved = 0;
        while (moved < qsize) {
            if (queue_pop(&queues[level], &idx) != 0) {
                return -1;
            }
            processes[idx].quantum_used = 0;
            if (level > 0) {
                processes[idx].current_queue = 0;
                if (queue_push(&queues[0], idx) != 0) {
                    return -1;
                }
            } else {
                if (queue_push(&queues[0], idx) != 0) {
                    return -1;
                }
            }
            moved++;
        }
        if (level > 0) {
            queues[level].head = 0;
            queues[level].tail = 0;
            queues[level].size = 0;
        }
    }

    snprintf(
        description,
        sizeof(description),
        "Priority boost aplicado en t=%d; %d procesos pendientes vuelven a Q0",
        current_time,
        affected
    );
    if (append_event(trace, current_time, TRACE_EVENT_BOOST, -1, -1, 0, description) != 0) {
        return -1;
    }

    (void)count;
    return 0;
}

int run_mlfq_simulation(
    Process *processes,
    size_t count,
    const SchedulerConfig *config,
    SimulationTrace *trace
) {
    ReadyQueue queues[MLFQ_LEVELS];
    int running = -1;
    int completed = 0;
    int time = 0;
    int max_steps;
    int i;

    if (!processes || !config || count == 0) {
        return -1;
    }

    if (trace) {
        free_simulation_trace(trace);
    }

    max_steps = 0;
    for (i = 0; i < (int)count; ++i) {
        reset_process(&processes[i]);
        if (processes[i].burst_time <= 0 || processes[i].arrival_time < 0) {
            return -1;
        }
        if (max_steps > INT_MAX - processes[i].burst_time) {
            return -1;
        }
        max_steps += processes[i].burst_time;
    }

    for (i = 0; i < MLFQ_LEVELS; ++i) {
        if (queue_init(&queues[i], (int)count * 8) != 0) {
            while (i-- > 0) {
                queue_destroy(&queues[i]);
            }
            return -1;
        }
    }

    while (completed < (int)count && time <= max_steps * 10) {
        Process *p;

        if (enqueue_new_arrivals(processes, count, time, queues, trace) != 0) {
            goto fail;
        }

        if (config->boost_period > 0 && time > 0 && (time % config->boost_period == 0)) {
            if (apply_priority_boost(processes, count, queues, &running, time, trace) != 0) {
                goto fail;
            }
        }

        if (running >= 0 && has_higher_ready_queue(queues, processes[running].current_queue)) {
            char description[128];

            snprintf(
                description,
                sizeof(description),
                "%s es interrumpido porque hay trabajo listo en una cola de mayor prioridad",
                processes[running].pid
            );
            if (
                append_event(
                    trace,
                    time,
                    TRACE_EVENT_PREEMPT,
                    running,
                    processes[running].current_queue,
                    processes[running].current_queue,
                    description
                ) != 0
            ) {
                goto fail;
            }
            if (queue_push(&queues[processes[running].current_queue], running) != 0) {
                goto fail;
            }
            running = -1;
        }

        if (running < 0) {
            running = choose_next_process(queues, processes);
            if (running >= 0) {
                char description[128];

                if (processes[running].start_time < 0) {
                    processes[running].start_time = time;
                    processes[running].first_response_time = time;
                }
                snprintf(
                    description,
                    sizeof(description),
                    "%s entra a CPU desde Q%d",
                    processes[running].pid,
                    processes[running].current_queue
                );
                if (
                    append_event(
                        trace,
                        time,
                        TRACE_EVENT_DISPATCH,
                        running,
                        processes[running].current_queue,
                        processes[running].current_queue,
                        description
                    ) != 0
                ) {
                    goto fail;
                }
            }
        }

        if (append_snapshot(trace, time, running, processes, queues) != 0) {
            goto fail;
        }

        if (running < 0) {
            if (append_slice(trace, time, time + 1, -1, -1) != 0) {
                goto fail;
            }
            time++;
            continue;
        }

        p = &processes[running];
        p->remaining_time--;
        p->quantum_used++;
        if (append_slice(trace, time, time + 1, running, p->current_queue) != 0) {
            goto fail;
        }
        time++;

        if (p->remaining_time == 0) {
            char description[128];

            p->finish_time = time;
            snprintf(
                description,
                sizeof(description),
                "%s finaliza en t=%d",
                p->pid,
                time
            );
            if (append_event(trace, time, TRACE_EVENT_FINISH, running, p->current_queue, -1, description) != 0) {
                goto fail;
            }
            running = -1;
            completed++;
            continue;
        }

        if (p->quantum_used >= config->quantums[p->current_queue]) {
            int from_queue = p->current_queue;
            int to_queue = p->current_queue;
            char description[128];

            if (p->current_queue < MLFQ_LEVELS - 1) {
                to_queue = p->current_queue + 1;
                p->current_queue = to_queue;
            }
            p->quantum_used = 0;
            snprintf(
                description,
                sizeof(description),
                "%s agota su quantum y pasa de Q%d a Q%d",
                p->pid,
                from_queue,
                to_queue
            );
            if (append_event(trace, time, TRACE_EVENT_DEMOTION, running, from_queue, to_queue, description) != 0) {
                goto fail;
            }
            if (queue_push(&queues[p->current_queue], running) != 0) {
                goto fail;
            }
            running = -1;
        }
    }

    for (i = 0; i < MLFQ_LEVELS; ++i) {
        queue_destroy(&queues[i]);
    }
    if (trace) {
        trace->total_time = time;
        if (append_snapshot(trace, time, -1, processes, queues) != 0) {
            free_simulation_trace(trace);
            return -1;
        }
    }
    return completed == (int)count ? 0 : -1;

fail:
    for (i = 0; i < MLFQ_LEVELS; ++i) {
        queue_destroy(&queues[i]);
    }
    if (trace) {
        free_simulation_trace(trace);
    }
    return -1;
}

