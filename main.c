/**
 * ====================================================================================
 *  main.c  --  Bucle interactivo (REPL) y despacho de comandos
 * ====================================================================================
 *  Read - Eval - Print - Loop:
 *    1. Read  : fgets() lee una línea del teclado (STDIN).
 *    2. Eval  : el primer carácter es el comando; el resto, su argumento.
 *    3. Print : el manejador imprime lo que corresponda.
 *    4. Loop  : repetir hasta 'q' o Ctrl+D.
 *
 *  El enunciado permite printf/fgets SOLO para esto: leer comandos e imprimir
 *  en consola. El archivo de texto jamás se toca con la biblioteca estándar.
 * ====================================================================================
 */

#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENTRADA 8192

/* ==================================================================================
 * TABLA DE COMANDOS
 * ==================================================================================
 * Mismo patrón que el shell del profe: una tabla de datos + punteros a función.
 * Añadir un comando nuevo = añadir una fila. El bucle principal NO se toca.
 * Esto se llama despacho por tabla, y es la alternativa limpia a un if/else gigante.
 */

typedef int (*Manejador)(Editor *ed, const char *arg);

typedef struct {
    char        clave;        /* La letra que escribe el usuario */
    const char *uso;          /* Sintaxis, para el menú de ayuda  */
    const char *descripcion;  /* Qué hace                         */
    Manejador   fn;           /* Función que lo implementa        */
} ComandoEd;

static const ComandoEd tabla[] = {
    { 'o', "o <archivo>", "Abre un archivo (lo crea si no existe).", cmd_o },
    { 'p', "p [n]",       "Imprime la línea n, o todo el archivo.",  cmd_p },
    { 'a', "a <texto>",   "Añade el texto como línea al final.",     cmd_a },
    { 'd', "d <n>",       "Borra la línea n.",                       cmd_d },
};

static const int n_comandos = (int)(sizeof(tabla) / sizeof(tabla[0]));

/* ==================================================================================
 * Ayuda
 * ================================================================================== */
static void mostrar_ayuda(void)
{
    printf("\nComandos disponibles:\n");
    for (int i = 0; i < n_comandos; i++) {
        printf("  %-14s %s\n", tabla[i].uso, tabla[i].descripcion);
    }
    printf("  %-14s %s\n", "h", "Muestra esta ayuda.");
    printf("  %-14s %s\n", "q", "Cierra el archivo y sale.");
    printf("\n");
}

/* ==================================================================================
 * Separar el comando de su argumento
 * ==================================================================================
 * A diferencia del shell del profe (que trocea en argv[]), aquí NO tokenizamos:
 * el comando es un solo carácter y el resto de la línea es el argumento crudo.
 *
 * ¿Por qué? Porque 'a hola mundo con espacios' debe conservar los espacios tal
 * cual. Trocear y volver a pegar sería trabajo extra y una fuente de bugs.
 * Es exactamente el comportamiento del editor 'ed' de Unix.
 *
 * Devuelve: puntero al argumento (dentro de la misma cadena), nunca NULL.
 */
static char *separar_argumento(char *linea)
{
    char *p = linea + 1;                     /* Saltamos el carácter del comando */
    while (*p == ' ' || *p == '\t') p++;     /* Saltamos espacios intermedios    */
    return p;
}

/* Quita el '\n' final que fgets deja pegado a la cadena. */
static void quitar_salto(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/* ==================================================================================
 * MAIN
 * ==================================================================================
 * Acepta un argumento opcional: ./editor archivo.txt abre el archivo de una.
 * Eso es lo que le permitirá al shell invocarlo con un parámetro.
 */
int main(int argc, char **argv)
{
    Editor ed;
    ed_init(&ed);                 /* Estado limpio ANTES de cualquier otra cosa */

    char entrada[MAX_ENTRADA];

    printf("=========================================\n");
    printf("  Editor de texto CLI  -  SO2026B (EAFIT)\n");
    printf("  Escribe 'h' para ver la ayuda.\n");
    printf("=========================================\n");

    /* Si nos pasaron un archivo por línea de comandos, lo abrimos de entrada. */
    if (argc > 1) {
        cmd_o(&ed, argv[1]);
    }

    while (1) {
        printf("ed> ");
        fflush(stdout);           /* El prompt no lleva '\n': hay que forzar que salga */

        /* fgets devuelve NULL en EOF (Ctrl+D). Salimos limpiamente. */
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            printf("\n");
            break;
        }

        quitar_salto(entrada);

        /* Línea vacía: no es un error, simplemente volvemos a preguntar. */
        if (entrada[0] == '\0') continue;

        char clave = entrada[0];

        /* --- Comandos que maneja el propio bucle --- */
        if (clave == 'q') {
            break;                /* La limpieza real ocurre después del bucle */
        }
        if (clave == 'h') {
            mostrar_ayuda();
            continue;
        }

        /* --- Despacho por tabla --- */
        char *arg = separar_argumento(entrada);

        int encontrado = 0;
        for (int i = 0; i < n_comandos; i++) {
            if (tabla[i].clave == clave) {
                tabla[i].fn(&ed, arg);
                encontrado = 1;
                break;
            }
        }

        if (!encontrado) {
            printf("Comando '%c' no reconocido. Escribe 'h' para la ayuda.\n", clave);
        }
    }

    /* ÚNICO punto de salida: un solo lugar donde se libera todo.
       Así es imposible olvidar un close() o un free() en algún camino raro. */
    ed_cerrar(&ed);
    printf("Editor cerrado. Hasta luego.\n");

    return 0;
}