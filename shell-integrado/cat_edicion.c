/*
 * cat_edicion.c -- integracion del editor con el shell eafitOS.
 * Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 * Registra el comando 'e_edit' del shell y lo conecta con el bucle propio
 * del editor (repl.c). Se compilo junto al shell en vez de como programa
 * externo por simplicidad, ya que igual comparten el mismo repositorio.
 */

#include "shell.h"
#include "editor.h"

#include <stdio.h>

/*
 * e_edit [archivo]  --  abre el editor de texto integrado.
 * El archivo es opcional; sin el, el editor arranca sin nada abierto y se
 * puede abrir despues con el comando 'o'. Se vuelve al shell con 'q'.
 */
int cmd_e_edit(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, COLOR_ERROR "Uso: e_edit [archivo]\n" COLOR_RESET);
        return 1;
    }

    const char *ruta = (argc == 2) ? argv[1] : NULL;

    printf(COLOR_INFO "Cediendo el control al editor de texto.\n" COLOR_RESET);
    printf(COLOR_INFO "Escribe 'h' para la ayuda del editor y 'q' para volver a eafitOS.\n\n" COLOR_RESET);

    editor_ejecutar(ruta);

    printf(COLOR_INFO "\nEditor cerrado. De vuelta en eafitOS.\n" COLOR_RESET);
    return 0;
}
