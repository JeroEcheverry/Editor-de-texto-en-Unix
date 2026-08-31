/**
 * ====================================================================================
 *  comandos.c  --  CAPA DE COMANDOS
 * ====================================================================================
 *  Traduce lo que el usuario escribe en llamadas a la capa de disco.
 *
 *  Reglas de esta capa:
 *   - Valida SIEMPRE los argumentos antes de tocar el disco.
 *   - Convierte la numeración humana (línea 1, 2, 3...) a índices del arreglo (0, 1, 2...).
 *   - Imprime en pantalla usando write() sobre el File Descriptor 1 (STDOUT),
 *     como pide el enunciado.
 * ====================================================================================
 */

#include "editor.h"

#include <unistd.h>   /* write */
#include <stdio.h>    /* printf, fflush */
#include <stdlib.h>   /* strtol */
#include <string.h>   /* strlen */
#include <errno.h>
#include <limits.h>

/* ==================================================================================
 * Utilidades internas
 * ================================================================================== */

/**
 * Imprime un bloque de bytes en STDOUT usando write() (FD 1).
 *
 * ¡CUIDADO CON LA MEZCLA! printf() escribe en un búfer de la biblioteca estándar
 * que se vacía cuando le da la gana. write() va directo al kernel. Si mezclas
 * ambos sin cuidado, la salida sale DESORDENADA en pantalla.
 * Solución: fflush(stdout) antes de cada write() para vaciar el búfer de printf.
 */
static void imprimir_bytes(const char *buf, size_t n)
{
    fflush(stdout);
    escribir_todo(1, buf, n);   /* FD 1 = STDOUT, siempre abierto por el shell */
}

/**
 * Convierte texto a número de línea y valida que sea usable.
 * Devuelve el número (>=1) o -1 si el texto no es un entero positivo válido.
 *
 * Usamos strtol y no atoi porque atoi NO reporta errores: atoi("hola") da 0
 * silenciosamente, y ese 0 se te cuela como número de línea válido.
 */
static long parsear_numero(const char *txt)
{
    if (txt == NULL || *txt == '\0') return -1;

    char *fin = NULL;
    errno = 0;
    long v = strtol(txt, &fin, 10);

    if (errno != 0)      return -1;   /* Desbordamiento */
    if (fin == txt)      return -1;   /* No había ni un dígito */
    while (*fin == ' ' || *fin == '\t') fin++;
    if (*fin != '\0')    return -1;   /* Sobraba basura después del número */
    if (v < 1)           return -1;   /* Las líneas se cuentan desde 1 */

    return v;
}

/* Mensaje único para "no has abierto nada todavía". */
static int exigir_archivo(const Editor *ed)
{
    if (!ed_esta_abierto(ed)) {
        printf("Error: no hay ningún archivo abierto. Usa: o <archivo>\n");
        return -1;
    }
    return 0;
}

/* ==================================================================================
 * o [archivo]  --  Abrir o crear
 * ================================================================================== */
int cmd_o(Editor *ed, const char *arg)
{
    if (arg == NULL || *arg == '\0') {
        printf("Uso: o <archivo>\n");
        return -1;
    }

    if (ed_abrir(ed, arg) == -1) {
        return -1;                     /* ed_abrir ya llamó a perror() */
    }

    printf("Archivo '%s' abierto (fd=%d, %ld bytes, %zu líneas).\n",
           ed->ruta, ed->fd, (long)ed->tam, ed->n_lineas);
    return 0;
}

/* ==================================================================================
 * p [n]  --  Imprimir línea n, o todo el archivo si no hay argumento
 * ================================================================================== */
int cmd_p(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    char buf[ED_MAX_LINEA];

    /* --- Caso 1: 'p' sin argumento -> volcar el archivo completo --- */
    if (arg == NULL || *arg == '\0') {
        if (ed->n_lineas == 0) {
            printf("(archivo vacío)\n");
            return 0;
        }

        for (size_t i = 0; i < ed->n_lineas; i++) {
            ssize_t n = ed_leer_linea(ed, i, buf, sizeof(buf));
            if (n < 0) return -1;

            printf("%4zu | ", i + 1);      /* Numeración humana: empieza en 1 */
            imprimir_bytes(buf, (size_t)n);

            /* Si la última línea no traía '\n', lo ponemos para no romper el prompt. */
            if (n == 0 || buf[n - 1] != '\n') imprimir_bytes("\n", 1);
        }
        return 0;
    }

    /* --- Caso 2: 'p n' -> una sola línea --- */
    long n_linea = parsear_numero(arg);
    if (n_linea < 0) {
        printf("Uso: p [n]   (n debe ser un entero >= 1)\n");
        return -1;
    }
    if ((size_t)n_linea > ed->n_lineas) {
        printf("Error: la línea %ld no existe (el archivo tiene %zu líneas).\n",
               n_linea, ed->n_lineas);
        return -1;
    }

    ssize_t n = ed_leer_linea(ed, (size_t)n_linea - 1, buf, sizeof(buf));
    if (n < 0) return -1;

    imprimir_bytes(buf, (size_t)n);
    if (n == 0 || buf[n - 1] != '\n') imprimir_bytes("\n", 1);
    return 0;
}

/* ==================================================================================
 * a [texto]  --  Anexar una línea al final
 * ================================================================================== */
int cmd_a(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    if (arg == NULL) arg = "";        /* 'a' solo => añade una línea en blanco */

    if (ed_anexar(ed, arg) == -1) return -1;

    printf("Línea añadida. El archivo tiene ahora %zu líneas (%ld bytes).\n",
           ed->n_lineas, (long)ed->tam);
    return 0;
}

/* ==================================================================================
 * d [n]  --  Borrar la línea n
 * ================================================================================== */
int cmd_d(Editor *ed, const char *arg)
{
    if (exigir_archivo(ed) == -1) return -1;

    long n_linea = parsear_numero(arg);
    if (n_linea < 0) {
        printf("Uso: d <n>   (n debe ser un entero >= 1)\n");
        return -1;
    }
    if (ed->n_lineas == 0) {
        printf("Error: el archivo está vacío, no hay nada que borrar.\n");
        return -1;
    }
    if ((size_t)n_linea > ed->n_lineas) {
        printf("Error: la línea %ld no existe (el archivo tiene %zu líneas).\n",
               n_linea, ed->n_lineas);
        return -1;
    }

    if (ed_borrar_linea(ed, (size_t)n_linea - 1) == -1) return -1;

    printf("Línea %ld borrada. Quedan %zu líneas (%ld bytes).\n",
           n_linea, ed->n_lineas, (long)ed->tam);
    return 0;
}