#include "scheduler.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

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

static void write_html_escaped(FILE *f, const char *text) {
    const unsigned char *p = (const unsigned char *)text;

    if (!text) {
        return;
    }

    while (*p) {
        switch (*p) {
            case '&':
                fputs("&amp;", f);
                break;
            case '<':
                fputs("&lt;", f);
                break;
            case '>':
                fputs("&gt;", f);
                break;
            case '"':
                fputs("&quot;", f);
                break;
            default:
                fputc((int)*p, f);
                break;
        }
        ++p;
    }
}

static const char *event_type_label(TraceEventType type) {
    switch (type) {
        case TRACE_EVENT_ARRIVAL:
            return "ARRIVAL";
        case TRACE_EVENT_DISPATCH:
            return "DISPATCH";
        case TRACE_EVENT_PREEMPT:
            return "PREEMPT";
        case TRACE_EVENT_DEMOTION:
            return "DEMOTION";
        case TRACE_EVENT_BOOST:
            return "BOOST";
        case TRACE_EVENT_FINISH:
            return "FINISH";
        default:
            return "UNKNOWN";
    }
}

static const char *event_type_class(TraceEventType type) {
    switch (type) {
        case TRACE_EVENT_ARRIVAL:
            return "event-arrival";
        case TRACE_EVENT_DISPATCH:
            return "event-dispatch";
        case TRACE_EVENT_PREEMPT:
            return "event-preempt";
        case TRACE_EVENT_DEMOTION:
            return "event-demotion";
        case TRACE_EVENT_BOOST:
            return "event-boost";
        case TRACE_EVENT_FINISH:
            return "event-finish";
        default:
            return "event-generic";
    }
}

static const char *color_for_process(int process_index) {
    static const char *palette[] = {
        "#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c", "#0891b2",
        "#db2777", "#65a30d", "#7c3aed", "#0f766e", "#b91c1c", "#1d4ed8"
    };
    size_t palette_size = sizeof(palette) / sizeof(palette[0]);

    if (process_index < 0) {
        return "#6b7280";
    }
    return palette[(size_t)process_index % palette_size];
}

static int count_context_switches(const SimulationTrace *trace) {
    int last_process = -2;
    int switches = 0;
    size_t i;

    if (!trace) {
        return 0;
    }

    for (i = 0; i < trace->slice_count; ++i) {
        int current = trace->slices[i].process_index;
        if (current < 0) {
            continue;
        }
        if (last_process >= 0 && last_process != current) {
            switches++;
        }
        last_process = current;
    }

    return switches;
}

static int count_idle_time(const SimulationTrace *trace) {
    int total = 0;
    size_t i;

    if (!trace) {
        return 0;
    }

    for (i = 0; i < trace->slice_count; ++i) {
        if (trace->slices[i].process_index < 0) {
            total += trace->slices[i].end_time - trace->slices[i].start_time;
        }
    }

    return total;
}

static void write_process_table(FILE *f, const Process *processes, size_t count) {
    size_t i;

    fputs("<table><thead><tr><th>PID</th><th>Arrival</th><th>Burst</th><th>Start</th><th>Finish</th><th>Response</th><th>Turnaround</th><th>Waiting</th></tr></thead><tbody>", f);
    for (i = 0; i < count; ++i) {
        int response = processes[i].first_response_time - processes[i].arrival_time;
        int turnaround = processes[i].finish_time - processes[i].arrival_time;
        int waiting = turnaround - processes[i].burst_time;

        fprintf(
            f,
            "<tr><td><strong>%s</strong></td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td></tr>",
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
    fputs("</tbody></table>", f);
}

static void write_boost_markers(FILE *f, const SchedulerConfig *config, int total_time) {
    int time;

    if (!config || config->boost_period <= 0 || total_time <= 0) {
        return;
    }

    for (time = config->boost_period; time < total_time; time += config->boost_period) {
        double left = ((double)time / (double)total_time) * 100.0;
        fprintf(
            f,
            "<div class=\"boost-line\" style=\"left: %.4f%%;\" title=\"Boost en t=%d\"></div>",
            left,
            time
        );
    }
}

static void write_cpu_gantt(FILE *f, const Process *processes, const SimulationTrace *trace, int total_time) {
    int queue_level;
    size_t i;

    fputs("<div class=\"gantt-grid\">", f);
    for (queue_level = 0; queue_level < MLFQ_LEVELS; ++queue_level) {
        fprintf(
            f,
            "<div class=\"gantt-row\"><div class=\"gantt-axis-label\">Q%d</div><div class=\"gantt-track\">",
            queue_level
        );
        for (i = 0; i < trace->event_count; ++i) {
            const TraceEvent *event = &trace->events[i];
            if (event->type == TRACE_EVENT_BOOST && total_time > 0) {
                double left = ((double)event->time / (double)total_time) * 100.0;
                fprintf(
                    f,
                    "<div class=\"boost-line\" style=\"left: %.4f%%;\" title=\"Boost en t=%d\"></div>",
                    left,
                    event->time
                );
            }
        }
        for (i = 0; i < trace->slice_count; ++i) {
            const CpuSlice *slice = &trace->slices[i];
            double left;
            double width;
            const char *color;
            const char *label;

            if (slice->process_index < 0 || slice->queue_level != queue_level) {
                continue;
            }

            left = ((double)slice->start_time / (double)total_time) * 100.0;
            width = ((double)(slice->end_time - slice->start_time) / (double)total_time) * 100.0;
            color = color_for_process(slice->process_index);
            label = processes[slice->process_index].pid;

            fprintf(
                f,
                "<div class=\"cpu-slice active\" style=\"left: %.4f%%; width: %.4f%%; background: %s;\" title=\"%s | t=%d-%d | Q%d\">",
                left,
                width,
                color,
                label,
                slice->start_time,
                slice->end_time,
                slice->queue_level
            );
            write_html_escaped(f, label);
            fputs("</div>", f);
        }
        fputs("</div></div>", f);
    }
    fputs("</div>", f);
}

static void write_process_timelines(
    FILE *f,
    const Process *processes,
    size_t count,
    const SimulationTrace *trace,
    const SchedulerConfig *config,
    int total_time
) {
    size_t i;
    size_t j;

    fputs("<div class=\"process-lifelines\">", f);
    for (i = 0; i < count; ++i) {
        fprintf(
            f,
            "<div class=\"process-row\"><div class=\"process-label\">%s</div><div class=\"process-track\">",
            processes[i].pid
        );
        write_boost_markers(f, config, total_time);
        fprintf(
            f,
            "<div class=\"arrival-marker\" style=\"left: %.4f%%;\" title=\"Arrival %d\"></div>",
            ((double)processes[i].arrival_time / (double)total_time) * 100.0,
            processes[i].arrival_time
        );
        fprintf(
            f,
            "<div class=\"finish-marker\" style=\"left: %.4f%%;\" title=\"Finish %d\"></div>",
            ((double)processes[i].finish_time / (double)total_time) * 100.0,
            processes[i].finish_time
        );
        for (j = 0; j < trace->slice_count; ++j) {
            const CpuSlice *slice = &trace->slices[j];
            if (slice->process_index != (int)i) {
                continue;
            }
            fprintf(
                f,
                "<div class=\"process-slice\" style=\"left: %.4f%%; width: %.4f%%; background: %s;\" title=\"%s | t=%d-%d | Q%d\">Q%d</div>",
                ((double)slice->start_time / (double)total_time) * 100.0,
                ((double)(slice->end_time - slice->start_time) / (double)total_time) * 100.0,
                color_for_process((int)i),
                processes[i].pid,
                slice->start_time,
                slice->end_time,
                slice->queue_level,
                slice->queue_level
            );
        }
        fputs("</div></div>", f);
    }
    fputs("</div>", f);
}

static void write_event_table(FILE *f, const Process *processes, const SimulationTrace *trace) {
    size_t i;

    fputs("<table><thead><tr><th>Tiempo</th><th>Evento</th><th>Proceso</th><th>Detalle</th></tr></thead><tbody>", f);
    for (i = 0; i < trace->event_count; ++i) {
        const TraceEvent *event = &trace->events[i];
        fputs("<tr>", f);
        fprintf(f, "<td>%d</td>", event->time);
        fprintf(
            f,
            "<td><span class=\"pill %s\">%s</span></td>",
            event_type_class(event->type),
            event_type_label(event->type)
        );
        fputs("<td>", f);
        if (event->process_index >= 0) {
            write_html_escaped(f, processes[event->process_index].pid);
        } else {
            fputs("-", f);
        }
        fputs("</td><td>", f);
        write_html_escaped(f, event->description);
        fputs("</td></tr>", f);
    }
    fputs("</tbody></table>", f);
}

static void write_snapshot_table(FILE *f, const SimulationTrace *trace) {
    size_t i;

    fputs("<table><thead><tr><th>Tiempo</th><th>CPU</th><th>Q0</th><th>Q1</th><th>Q2</th></tr></thead><tbody>", f);
    for (i = 0; i < trace->snapshot_count; ++i) {
        const QueueSnapshot *snapshot = &trace->snapshots[i];
        fprintf(f, "<tr><td>%d</td><td>", snapshot->time);
        write_html_escaped(f, snapshot->running_label);
        fputs("</td><td>", f);
        write_html_escaped(f, snapshot->queue_labels[0]);
        fputs("</td><td>", f);
        write_html_escaped(f, snapshot->queue_labels[1]);
        fputs("</td><td>", f);
        write_html_escaped(f, snapshot->queue_labels[2]);
        fputs("</td></tr>", f);
    }
    fputs("</tbody></table>", f);
}

int write_html_report(
    const char *path,
    const Process *processes,
    size_t count,
    const SchedulerConfig *config,
    const SimulationSummary *summary,
    const SimulationTrace *trace
) {
    FILE *f;
    int total_time;
    int context_switches;
    int idle_time;

    if (!path || !processes || count == 0 || !config || !summary || !trace) {
        return -1;
    }

    if (ensure_parent_dir(path) != 0) {
        return -1;
    }

    total_time = trace->total_time > 0 ? trace->total_time : 1;
    context_switches = count_context_switches(trace);
    idle_time = count_idle_time(trace);

    f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    fputs(
        "<!DOCTYPE html><html lang=\"es\"><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>Reporte MLFQ</title><style>"
        "body{font-family:Arial,sans-serif;background:#f5f7fb;color:#172033;margin:0;padding:24px;}"
        "h1,h2{margin:0 0 12px;}h1{font-size:2rem;}h2{margin-top:32px;font-size:1.25rem;}"
        ".lead{color:#475569;max-width:1000px;line-height:1.5;}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;margin:18px 0 28px;}"
        ".card{background:#fff;border:1px solid #dbe3f0;border-radius:14px;padding:16px;box-shadow:0 6px 18px rgba(15,23,42,.05);}"
        ".metric{font-size:1.5rem;font-weight:700;margin-top:8px;}"
        "table{width:100%;border-collapse:collapse;background:#fff;border-radius:14px;overflow:hidden;box-shadow:0 6px 18px rgba(15,23,42,.05);}"
        "th,td{padding:10px 12px;border-bottom:1px solid #e5e7eb;text-align:left;font-size:.95rem;vertical-align:top;}"
        "th{background:#eef4ff;}"
        ".timeline-shell{background:#fff;border:1px solid #dbe3f0;border-radius:14px;padding:16px;box-shadow:0 6px 18px rgba(15,23,42,.05);}"
        ".timeline-scale{display:flex;justify-content:space-between;font-size:.9rem;color:#475569;margin-bottom:10px;}"
        ".timeline-track,.process-track,.gantt-track{position:relative;height:58px;background:linear-gradient(90deg,#f8fafc,#eef2ff);border:1px solid #dbe3f0;border-radius:12px;overflow:hidden;}"
        ".gantt-grid{display:flex;flex-direction:column;gap:10px;}"
        ".gantt-row{display:grid;grid-template-columns:64px 1fr;gap:12px;align-items:center;}"
        ".gantt-axis-label{font-weight:700;color:#1e293b;text-align:right;padding-right:4px;}"
        ".cpu-slice,.process-slice{position:absolute;top:10px;height:38px;border-radius:10px;color:#fff;font-size:.78rem;font-weight:700;display:flex;align-items:center;justify-content:center;box-shadow:0 3px 10px rgba(15,23,42,.2);overflow:hidden;white-space:nowrap;}"
        ".cpu-slice.idle{background:#94a3b8;color:#111827;}"
        ".process-lifelines{display:flex;flex-direction:column;gap:12px;}"
        ".process-row{display:grid;grid-template-columns:80px 1fr;gap:12px;align-items:center;}"
        ".process-label{font-weight:700;color:#1e293b;}"
        ".arrival-marker,.finish-marker,.boost-line{position:absolute;top:0;bottom:0;width:3px;}"
        ".arrival-marker{background:#22c55e;}"
        ".finish-marker{background:#ef4444;}"
        ".boost-line{background:rgba(168,85,247,.9);width:2px;z-index:1;}"
        ".pill{display:inline-block;padding:4px 8px;border-radius:999px;font-size:.78rem;font-weight:700;}"
        ".event-arrival{background:#dcfce7;color:#166534;}.event-dispatch{background:#dbeafe;color:#1d4ed8;}"
        ".event-preempt{background:#fef3c7;color:#92400e;}.event-demotion{background:#ffe4e6;color:#be123c;}"
        ".event-boost{background:#f3e8ff;color:#7e22ce;}.event-finish{background:#d1fae5;color:#065f46;}"
        "code{background:#e2e8f0;padding:2px 6px;border-radius:6px;}"
        "</style></head><body>",
        f
    );

    fputs("<h1>Reporte HTML de la simulacion MLFQ</h1>", f);
    fputs(
        "<p class=\"lead\">Este reporte resume una ejecucion completa del scheduler Multi-Level Feedback Queue. Incluye configuracion, metricas, diagrama temporal de CPU, lineas de vida por proceso, eventos relevantes y snapshots del estado de las colas para reconstruir la historia completa de la simulacion.</p>",
        f
    );

    fputs("<div class=\"grid\">", f);
    fprintf(f, "<div class=\"card\"><div>Quantum Q0</div><div class=\"metric\">%d</div></div>", config->quantums[0]);
    fprintf(f, "<div class=\"card\"><div>Quantum Q1</div><div class=\"metric\">%d</div></div>", config->quantums[1]);
    fprintf(f, "<div class=\"card\"><div>Quantum Q2</div><div class=\"metric\">%d</div></div>", config->quantums[2]);
    fprintf(f, "<div class=\"card\"><div>Boost periodico</div><div class=\"metric\">%d</div></div>", config->boost_period);
    fprintf(f, "<div class=\"card\"><div>Tiempo total</div><div class=\"metric\">%d</div></div>", trace->total_time);
    fprintf(f, "<div class=\"card\"><div>Cambios de contexto</div><div class=\"metric\">%d</div></div>", context_switches);
    fprintf(f, "<div class=\"card\"><div>Tiempo ocioso CPU</div><div class=\"metric\">%d</div></div>", idle_time);
    fprintf(f, "<div class=\"card\"><div>Response promedio</div><div class=\"metric\">%.2f</div></div>", summary->average_response);
    fprintf(f, "<div class=\"card\"><div>Turnaround promedio</div><div class=\"metric\">%.2f</div></div>", summary->average_turnaround);
    fprintf(f, "<div class=\"card\"><div>Waiting promedio</div><div class=\"metric\">%.2f</div></div>", summary->average_waiting);
    fprintf(f, "<div class=\"card\"><div>Slices de CPU</div><div class=\"metric\">%zu</div></div>", trace->slice_count);
    fprintf(f, "<div class=\"card\"><div>Eventos registrados</div><div class=\"metric\">%zu</div></div>", trace->event_count);
    fputs("</div>", f);

    fputs("<h2>Metricas por proceso</h2>", f);
    write_process_table(f, processes, count);

    fputs("<h2>Gantt de CPU</h2>", f);
    fputs("<p class=\"lead\">Cada bloque representa un tramo continuo de ejecucion en CPU. En el eje Y aparecen las colas de prioridad `Q0`, `Q1` y `Q2`. La etiqueta visible de cada caja es el PID, y el tooltip indica intervalo exacto y cola. </p>", f);
    fputs("<div class=\"timeline-shell\"><div class=\"timeline-scale\">", f);
    fprintf(f, "<span>0</span><span>%d ciclos</span>", total_time);
    fputs("</div>", f);
    write_cpu_gantt(f, processes, trace, total_time);
    fputs("</div>", f);

    fputs("<h2>Linea de vida por proceso</h2>", f);
    fputs("<p class=\"lead\">La barra verde vertical marca la llegada, la roja marca la finalizacion y los bloques coloreados muestran cada vez que el proceso obtuvo CPU. Asi se puede observar su espera, sus interrupciones y su progreso entre colas.</p>", f);
    write_process_timelines(f, processes, count, trace, config, total_time);

    fputs("<h2>Eventos clave</h2>", f);
    fputs("<p class=\"lead\">Esta tabla resume las decisiones del scheduler: llegadas, despachos a CPU, interrupciones por prioridad, demociones por quantum, boosts periodicos y finalizaciones.</p>", f);
    write_event_table(f, processes, trace);

    fputs("<h2>Estado de colas por ciclo</h2>", f);
    fputs("<p class=\"lead\">Cada snapshot se toma al inicio del ciclo, despues de procesar llegadas, boost y posible seleccion de CPU. Permite reconstruir exactamente como estaban Q0, Q1 y Q2 antes de ejecutar el tick correspondiente.</p>", f);
    write_snapshot_table(f, trace);

    fputs("<h2>Como leer el reporte</h2><ul>", f);
    fputs("<li><strong>Q0</strong> concentra procesos nuevos o promovidos por boost y usa quantum corto para dar respuesta rapida.</li>", f);
    fputs("<li><strong>Q1</strong> y <strong>Q2</strong> absorben trabajo mas largo mediante demociones progresivas.</li>", f);
    fputs("<li>Las lineas moradas indican <strong>priority boost</strong>: todos los procesos pendientes vuelven a Q0.</li>", f);
    fputs("<li>Si un proceso aparece varias veces en su linea de vida, significa que fue replanificado varias veces por Round Robin, preemption o boost.</li>", f);
    fputs("</ul>", f);

    fputs("</body></html>", f);
    fclose(f);
    return 0;
}




