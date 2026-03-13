#include "scheduler.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LINE_BUF_SIZE 256

static void trim(char *s) {
    size_t len;
    size_t start = 0;

    if (!s) {
        return;
    }

    len = strlen(s);
    while (start < len && isspace((unsigned char)s[start])) {
        start++;
    }
    if (start > 0) {
        memmove(s, s + start, len - start + 1);
        len = strlen(s);
    }

    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static int ensure_parent_dir(const char *path) {
    char buffer[512];
    char *slash;

    if (!path) {
        return -1;
    }

    if (strlen(path) >= sizeof(buffer)) {
        return -1;
    }

    strcpy(buffer, path);
    slash = strrchr(buffer, '/');
    if (!slash) {
        return 0;
    }
    *slash = '\0';

    if (buffer[0] == '\0') {
        return 0;
    }

    if (mkdir(buffer, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

int load_processes_csv(const char *path, Process **processes, size_t *count) {
    FILE *f;
    char line[LINE_BUF_SIZE];
    size_t capacity = 16;
    size_t used = 0;
    Process *arr;

    if (!path || !processes || !count) {
        return -1;
    }

    f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    arr = (Process *)malloc(capacity * sizeof(Process));
    if (!arr) {
        fclose(f);
        return -1;
    }

    if (!fgets(line, sizeof(line), f)) {
        free(arr);
        fclose(f);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        char *pid;
        char *arrival;
        char *burst;
        char *saveptr = NULL;
        Process p;

        trim(line);
        if (line[0] == '\0') {
            continue;
        }

        pid = strtok_r(line, ",", &saveptr);
        arrival = strtok_r(NULL, ",", &saveptr);
        burst = strtok_r(NULL, ",", &saveptr);

        if (!pid || !arrival || !burst) {
            free(arr);
            fclose(f);
            return -1;
        }

        trim(pid);
        trim(arrival);
        trim(burst);

        if (strlen(pid) >= PID_MAX_LEN) {
            free(arr);
            fclose(f);
            return -1;
        }

        memset(&p, 0, sizeof(Process));
        strcpy(p.pid, pid);
        p.arrival_time = (int)strtol(arrival, NULL, 10);
        p.burst_time = (int)strtol(burst, NULL, 10);

        if (p.arrival_time < 0 || p.burst_time <= 0) {
            free(arr);
            fclose(f);
            return -1;
        }

        if (used == capacity) {
            Process *new_arr;
            capacity *= 2;
            new_arr = (Process *)realloc(arr, capacity * sizeof(Process));
            if (!new_arr) {
                free(arr);
                fclose(f);
                return -1;
            }
            arr = new_arr;
        }

        arr[used++] = p;
    }

    fclose(f);
    *processes = arr;
    *count = used;
    return used > 0 ? 0 : -1;
}

int write_results_csv(const char *path, const Process *processes, size_t count) {
    FILE *f;
    size_t i;

    if (!path || !processes || count == 0) {
        return -1;
    }

    if (ensure_parent_dir(path) != 0) {
        return -1;
    }

    f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    fprintf(f, "PID,Arrival,Burst,Start,Finish,Response,Turnaround,Waiting\n");
    for (i = 0; i < count; ++i) {
        int response = processes[i].first_response_time - processes[i].arrival_time;
        int turnaround = processes[i].finish_time - processes[i].arrival_time;
        int waiting = turnaround - processes[i].burst_time;

        fprintf(
            f,
            "%s,%d,%d,%d,%d,%d,%d,%d\n",
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

    fclose(f);
    return 0;
}

