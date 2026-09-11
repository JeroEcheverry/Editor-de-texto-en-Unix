/**
 * ====================================================================================
 *  cat_edicion.c  --  Integracion del editor con el shell eafitOS
 * ====================================================================================
 *  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 *  Este archivo es el unico punto de contacto entre el editor y el shell de la
 *  asignatura. Cumple el contrato que impone la tabla de comandos del shell
 *  (int (*handler)(int argc, char **argv)) y traduce esa llamada al bucle propio
 *  del editor.
 *
 *  Se registro en una categoria nueva ("edicion") en vez de meterlo en "datos"
 *  porque el editor no se comporta como los demas comandos del shell: esos reciben
 *  argumentos, ejecutan un par de syscalls, imprimen el resultado y devuelven el
 *  control de inmediato. El editor en cambio toma el control de stdin con su
 *  propio ciclo lectura-evaluacion-impresion, mantiene estado vivo entre comandos
 *  (descriptor abierto, indice de lineas, portapapeles, historial) y representa
 *  una sesion completa de trabajo, no una operacion puntual que se pueda trazar
 *  como las demas.
 *
 *  Tambien se penso en lanzarlo como binario externo con fork(2)+execvp(3), como
 *  hace p_exec, pero eso obligaria a mantener dos ejecutables separados. Se
 *  prefirio compilarlo junto al shell porque el enunciado pide integracion
 *  funcional, no solo poder invocarlo desde afuera.
 * ====================================================================================
 */

#include "shell.h"
#include "editor.h"

#include <stdio.h>

/**
 * e_edit [archivo]  --  Abre el editor de texto integrado.
 *
 * Recibe los argumentos ya divididos por el tokenizador del shell. El archivo es
 * opcional: sin el, el editor arranca sin ningun archivo abierto y el usuario lo
 * abre desde adentro con el comando 'o'.
 *
 * El control regresa al shell cuando el usuario escribe 'q' dentro del editor.
 * Todos los recursos (descriptor, indice, portapapeles y archivos temporales de
 * /tmp) se liberan dentro de editor_ejecutar antes de retornar, de modo que el
 * shell continua sin descriptores ni memoria pendientes.
 *
 * Retorna 0 en exito y 1 si los argumentos son invalidos, siguiendo la convencion
 * de los demas comandos del shell.
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
