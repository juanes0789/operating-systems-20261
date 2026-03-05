#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ticket/ticket.h"
#include "../include/utils/utils.h"

int main() {
    int id;
    char correo[100], tipo[100], buffer[100];

    printf("--- SISTEMA DE TICKETS ---\n");
    // 1. Validación de Identificación (Bucle)
    while (1) {
        printf("Ingrese Identificacion (solo numeros): ");
        if (fgets(buffer, sizeof(buffer), stdin)) {
            limpiar_string(buffer);
            if (strlen(buffer) > 0 && es_numerico(buffer)) {
                id = atoi(buffer); // Convertir cadena a entero
                break; 
            }
        }
        printf(" Error: La identificacion debe ser numerica y no estar vacia.\n\n");
    }

   while (1) {
        printf("Ingrese Correo Electronico: ");
        if (fgets(correo, sizeof(correo), stdin)) {
            limpiar_string(correo);
            if (strlen(correo) > 0 && validar_correo(correo)) {
                break; // Correo válido, salimos del bucle
            }
        }
        printf(" Error: El correo debe contener un '@' y no estar vacio.\n\n");
    }


    printf("Tipo de reclamacion: ");
    fgets(tipo, 100, stdin);
    limpiar_string(tipo);

    Ticket *mi_ticket = crear_ticket(id, correo, tipo);
    if (mi_ticket && guardar_ticket(mi_ticket)) {
        printf("\n Registro exitoso. Radicado: %ld\n", mi_ticket->radicado);
    } else {
        printf("\n Error al procesar el ticket.\n");
    }

    liberar_ticket(mi_ticket);
    return 0;
}