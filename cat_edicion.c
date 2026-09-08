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
 *  DECISION DE DISENO: POR QUE UNA CATEGORIA NUEVA
 *
 *  El shell agrupa sus comandos en cuatro categorias: datos, memoria, monitoreo y
 *  utilidades. Los catorce comandos existentes comparten una misma naturaleza: son
 *  demostraciones de una sola operacion. Reciben sus argumentos, ejecutan dos o
 *  tres llamadas al sistema, imprimen el resultado y retornan. No conservan ningun
 *  estado entre invocaciones y no le quitan el control al bucle del shell.
 *
 *  El editor rompe ese patron en tres puntos:
 *
 *    1. Toma el control de la entrada estandar y ejecuta su propio ciclo
 *       lectura-evaluacion-impresion anidado dentro del ciclo del shell.
 *    2. Mantiene estado vivo entre comandos: un descriptor abierto, el indice de
 *       lineas, el portapapeles y el historial de deshacer.
 *    3. Sus llamadas al sistema no corresponden a una operacion unica que se pueda
 *       trazar e imprimir, sino a una sesion completa de trabajo.
 *
 *  Clasificarlo en "datos" seria correcto por el tema (trabaja con archivos) pero
 *  incorrecto por la naturaleza del comando, y haria que la ayuda del shell
 *  presentara como equivalentes dos cosas que se comportan de forma distinta. Por
 *  eso se registra en una categoria nueva, "edicion", reservada a aplicaciones
 *  interactivas que ceden y devuelven el control del shell.
 *
 *  ALTERNATIVA CONSIDERADA
 *
 *  El shell tambien puede lanzar binarios externos con fork(2) y execvp(3) a traves
 *  de p_exec. Ejecutar el editor por esa via aislaria su memoria en un proceso
 *  aparte, pero obligaria a mantener dos binarios separados y a que el shell no
 *  tuviera ningun conocimiento del editor. Se prefirio compilarlo dentro porque el
 *  enunciado pide integracion funcional y porque el editor forma parte del mismo
 *  proyecto, no es una herramienta externa del sistema.
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
