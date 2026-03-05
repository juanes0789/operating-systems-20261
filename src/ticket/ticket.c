#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../include/ticket/ticket.h"

Ticket* crear_ticket(int id, const char *correo, const char *tipo) {
    Ticket *nuevo = (Ticket*) malloc(sizeof(Ticket));
    if (!nuevo) return NULL;

    nuevo->identificacion = id;
    nuevo->radicado = (long)time(NULL); // Generación única por tiempo
    
    // Reservamos memoria para los strings
    nuevo->correo = strdup(correo);
    nuevo->tipo_reclamacion = strdup(tipo);

    return nuevo;
}

int guardar_ticket(Ticket *t) {
    char ruta[150];
    sprintf(ruta, "assets/ticket_%ld.txt", t->radicado);

    FILE *archivo = fopen(ruta, "w");
    if (!archivo) return 0;

    fprintf(archivo, "Radicado: %ld\nID: %d\nCorreo: %s\nTipo: %s\n", 
            t->radicado, t->identificacion, t->correo, t->tipo_reclamacion);
    
    fclose(archivo);
    return 1;
}

void liberar_ticket(Ticket *t) {
    if (t) {
        free(t->correo);
        free(t->tipo_reclamacion);
        free(t);
    }
}