/**
 * ====================================================================================
 *  comandos.c  --  Capa de comandos
 * ====================================================================================
 *  Traduce lo que escribe el usuario en llamadas a la capa de disco.
 *
 *  Responsabilidades de esta capa:
 *   - Validar los argumentos antes de acceder al archivo.
 *   - Convertir la numeracion que ve el usuario (linea 1, 2, 3...) a los indices
 *     del arreglo interno (0, 1, 2...).
 *   - Informar al usuario del resultado de cada operacion.
 *
 *  Esta capa no ejecuta llamadas al sistema sobre el archivo de texto: delega
 *  todas esas operaciones en archivo.c.
 * ====================================================================================
 */

#include "editor.h"

#include <unistd.h>   /* write          */
#include <stdio.h>    /* printf         */
#include <stdlib.h>   /* strtol         */
#include <string.h>   /* strlen         */
#include <errno.h>    /* errno          */

/* ==================================================================================
 * Utilidades internas
 * ================================================================================== */

/**
 * Convierte una cadena en un numero de linea valido.
 *
 * Se usa strtol y no atoi porque atoi no reporta errores: atoi("hola") devuelve 0
 * sin indicar que la conversion fallo, y ese 0 se colaria como numero de linea.
 *
 * Retorna: el numero convertido (siempre >= 1), o -1 si la cadena no representa
 *          un entero positivo valido.
 */
static long parsear_numero(const char *txt)
{
    if (txt == NULL || *txt == '\0') return -1;

    char *fin = NULL;
    errno = 0;
    long v = strtol(txt, &fin, 10);

    if (errno != 0)   return -1;   /* Desbordamiento del rango de long */
    if (fin == txt)   return -1;   /* No habia ningun digito           */

    while (*fin == ' ' || *fin == '\t') fin++;

    if (*fin != '\0') return -1;   /* Quedaban caracteres sobrantes    */
    if (v < 1)        return -1;   /* Las lineas se numeran desde 1    */

    return v;
}

/**
 * Verifica que haya un archivo abierto antes de operar sobre el.
 * Retorna 0 si lo hay, -1 si no (e imprime el mensaje correspondiente).
 */
static int exigir_archivo(const Editor *ed)
{
    if (!ed_esta_abierto(ed)) {
        printf("Error: no hay ningun archivo abierto. Usa: o <archivo>\n");
        return -1;
    }
    return 0;
}

/* ==================================================================================
 * o <archivo>  --  Abrir o crear un archivo
 * ================================================================================== */
int cmd_o(Editor *ed, const char *arg)
{
    if (arg == NULL || *arg == '\0') {
        printf("Uso: o <archivo>\n");
        return -1;
    }

    if (ed_abrir(ed, arg) == -1) {
        return -1;                     /* ed_abrir ya reporto el error con perror */
    }

    printf("Archivo '%s' abierto (fd=%d, %ld bytes, %zu lineas).\n",
           ed->ruta, ed->fd, (long)ed->tam, ed->n_lineas);
    return 0;
}

/* ==================================================================================
 * p [n]  --  Imprimir la linea n, o el archivo completo si no se indica n
 * ================================================================================== */
int cmd_p(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    /* Caso 1: 'p' sin argumento imprime todo el archivo con numeracion. */
    if (arg == NULL || *arg == '\0') {
        if (ed->n_lineas == 0) {
            printf("(archivo vacio)\n");
            return 0;
        }

        for (size_t i = 0; i < ed->n_lineas; i++) {
            printf("%4zu | ", i + 1);
            if (ed_imprimir_linea(ed, i) == -1) return -1;
        }
        return 0;
    }

    /* Caso 2: 'p n' imprime una sola linea. */
    long n_linea = parsear_numero(arg);
    if (n_linea < 0) {
        printf("Uso: p [n]   (n debe ser un entero mayor o igual a 1)\n");
        return -1;
    }
    if ((size_t)n_linea > ed->n_lineas) {
        printf("Error: la linea %ld no existe (el archivo tiene %zu lineas).\n",
               n_linea, ed->n_lineas);
        return -1;
    }

    return ed_imprimir_linea(ed, (size_t)n_linea - 1);
}

/* ==================================================================================
 * a <texto>  --  Anadir el texto como una nueva linea al final
 * ================================================================================== */
int cmd_a(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    if (arg == NULL) arg = "";        /* 'a' sin texto anade una linea en blanco */

    if (ed_anexar(ed, arg) == -1) return -1;

    printf("Linea anadida. El archivo tiene ahora %zu lineas (%ld bytes).\n",
           ed->n_lineas, (long)ed->tam);
    return 0;
}

/* ==================================================================================
 * d <n>  --  Borrar la linea n
 * ================================================================================== */
int cmd_d(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    long n_linea = parsear_numero(arg);
    if (n_linea < 0) {
        printf("Uso: d <n>   (n debe ser un entero mayor o igual a 1)\n");
        return -1;
    }
    if (ed->n_lineas == 0) {
        printf("Error: el archivo esta vacio, no hay nada que borrar.\n");
        return -1;
    }
    if ((size_t)n_linea > ed->n_lineas) {
        printf("Error: la linea %ld no existe (el archivo tiene %zu lineas).\n",
               n_linea, ed->n_lineas);
        return -1;
    }

    if (ed_borrar_linea(ed, (size_t)n_linea - 1) == -1) return -1;

    printf("Linea %ld borrada. Quedan %zu lineas (%ld bytes).\n",
           n_linea, ed->n_lineas, (long)ed->tam);
    return 0;
}
