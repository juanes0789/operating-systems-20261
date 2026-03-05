#include <string.h>
#include "../../include/utils/utils.h"

void limpiar_string(char *str) {
    str[strcspn(str, "\n")] = 0;
}

int validar_correo(const char *str) {
    return (strchr(str, '@') != NULL);
}

int es_numerico(const char *str) {
    if (strlen(str) == 0) return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}