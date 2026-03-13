#include "scheduler.h"

#include <limits.h>
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
    ReadyQueue queues[MLFQ_LEVELS]
) {
    size_t i;
    for (i = 0; i < count; ++i) {
        if (processes[i].arrival_time == current_time) {
            processes[i].current_queue = 0;
            processes[i].quantum_used = 0;
            if (queue_push(&queues[0], (int)i) != 0) {
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
    int *running
) {
    int level;
    int idx;

    if (*running >= 0) {
        processes[*running].current_queue = 0;
        processes[*running].quantum_used = 0;
        if (queue_push(&queues[0], *running) != 0) {
            return -1;
        }
        *running = -1;
    }

    for (level = 1; level < MLFQ_LEVELS; ++level) {
        while (!queue_empty(&queues[level])) {
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

    (void)count;
    return 0;
}

int run_mlfq_simulation(Process *processes, size_t count, const SchedulerConfig *config) {
    ReadyQueue queues[MLFQ_LEVELS];
    int running = -1;
    int completed = 0;
    int time = 0;
    int max_steps;
    int i;

    if (!processes || !config || count == 0) {
        return -1;
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

        if (enqueue_new_arrivals(processes, count, time, queues) != 0) {
            goto fail;
        }

        if (config->boost_period > 0 && time > 0 && (time % config->boost_period == 0)) {
            if (apply_priority_boost(processes, count, queues, &running) != 0) {
                goto fail;
            }
        }

        if (running >= 0 && has_higher_ready_queue(queues, processes[running].current_queue)) {
            if (queue_push(&queues[processes[running].current_queue], running) != 0) {
                goto fail;
            }
            running = -1;
        }

        if (running < 0) {
            running = choose_next_process(queues, processes);
            if (running >= 0 && processes[running].start_time < 0) {
                processes[running].start_time = time;
                processes[running].first_response_time = time;
            }
        }

        if (running < 0) {
            time++;
            continue;
        }

        p = &processes[running];
        p->remaining_time--;
        p->quantum_used++;
        time++;

        if (p->remaining_time == 0) {
            p->finish_time = time;
            running = -1;
            completed++;
            continue;
        }

        if (p->quantum_used >= config->quantums[p->current_queue]) {
            if (p->current_queue < MLFQ_LEVELS - 1) {
                p->current_queue++;
            }
            p->quantum_used = 0;
            if (queue_push(&queues[p->current_queue], running) != 0) {
                goto fail;
            }
            running = -1;
        }
    }

    for (i = 0; i < MLFQ_LEVELS; ++i) {
        queue_destroy(&queues[i]);
    }
    return completed == (int)count ? 0 : -1;

fail:
    for (i = 0; i < MLFQ_LEVELS; ++i) {
        queue_destroy(&queues[i]);
    }
    return -1;
}

