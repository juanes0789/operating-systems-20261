#ifndef TICKET_H
#define TICKET_H

typedef struct {
    long radicado;
    int identificacion;
    char *correo;
    char *tipo_reclamacion;
} Ticket;

// Funciones principales
Ticket* crear_ticket(int id, const char *correo, const char *tipo);
int guardar_ticket(Ticket *t);
void liberar_ticket(Ticket *t);

#endif