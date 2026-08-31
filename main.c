/**
 * ====================================================================================
 *  main.c  --  Bucle interactivo y despacho de comandos
 * ====================================================================================
 *  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 *  El editor funciona como un ciclo lectura-evaluacion-impresion:
 *    1. Lee una linea del teclado con fgets().
 *    2. Toma el primer caracter como comando y el resto como argumento.
 *    3. Ejecuta el manejador correspondiente.
 *    4. Repite hasta recibir 'q' o Ctrl+D.
 *
 *  El enunciado permite printf y fgets unicamente para leer los comandos por STDIN
 *  e imprimir en consola. El archivo de texto se manipula siempre con llamadas al
 *  sistema, en archivo.c.
 * ====================================================================================
 */

#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENTRADA 8192   /* Longitud maxima de una linea de comando */

/* ==================================================================================
 * Tabla de comandos
 * ==================================================================================
 * Se sigue el mismo patron del shell de la asignatura: una tabla de datos con
 * punteros a funcion, en lugar de una cadena de if/else.
 *
 * Agregar un comando nuevo consiste en escribir su manejador y anadir una fila a
 * la tabla. El bucle principal no se modifica.
 */

typedef int (*Manejador)(Editor *ed, const char *arg);

typedef struct {
    char        clave;        /* Letra que escribe el usuario     */
    const char *uso;          /* Sintaxis, para el menu de ayuda  */
    const char *descripcion;  /* Que hace el comando              */
    Manejador   fn;           /* Funcion que lo implementa        */
} ComandoEd;

static const ComandoEd tabla[] = {
    { 'o', "o <archivo>", "Abre un archivo (lo crea si no existe).", cmd_o },
    { 'p', "p [n]",       "Imprime la linea n, o todo el archivo.",  cmd_p },
    { 'a', "a <texto>",   "Anade el texto como linea al final.",     cmd_a },
    { 'd', "d <n>",       "Borra la linea n.",                       cmd_d },
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
 * Separacion del comando y su argumento
 * ==================================================================================
 * A diferencia del shell de la asignatura, aqui no se divide la entrada en un
 * arreglo de tokens: el comando es un unico caracter y todo lo que sigue es el
 * argumento, tal cual lo escribio el usuario.
 *
 * Esto permite que 'a hola mundo con espacios' conserve los espacios sin tener que
 * volver a unir los tokens. Es el mismo comportamiento del editor ed de Unix.
 *
 * Devuelve un puntero al argumento dentro de la misma cadena. Nunca devuelve NULL:
 * si no hay argumento, apunta al terminador nulo.
 */
static char *separar_argumento(char *linea)
{
    char *p = linea + 1;                     /* Se omite el caracter del comando */
    while (*p == ' ' || *p == '\t') p++;     /* Se omiten los espacios iniciales */
    return p;
}

/* Elimina el salto de linea que fgets deja al final de la cadena. */
static void quitar_salto(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/* ==================================================================================
 * Programa principal
 * ==================================================================================
 * Acepta un argumento opcional con el nombre de un archivo, de modo que
 * './editor archivo.txt' lo abre de entrada. Esa es la forma en que el shell de la
 * asignatura invocara al editor.
 */
int main(int argc, char **argv)
{
    Editor ed;
    ed_init(&ed);                 /* Estado inicial conocido antes de cualquier operacion */

    char entrada[MAX_ENTRADA];

    printf("=========================================\n");
    printf("  Editor de texto CLI  -  SO2026B (EAFIT)\n");
    printf("  Escribe 'h' para ver la ayuda.\n");
    printf("=========================================\n");

    if (argc > 1) {
        cmd_o(&ed, argv[1]);
    }

    while (1) {
        printf("ed> ");
        fflush(stdout);           /* El prompt no lleva salto de linea: hay que forzar
                                     el vaciado del buffer para que aparezca en pantalla */

        /* fgets devuelve NULL al llegar al fin de la entrada (Ctrl+D). */
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            printf("\n");
            break;
        }

        quitar_salto(entrada);

        if (entrada[0] == '\0') continue;   /* Linea vacia: no es un error */

        char clave = entrada[0];

        /* Comandos atendidos por el propio bucle */
        if (clave == 'q') {
            break;                /* La liberacion de recursos ocurre al salir del bucle */
        }
        if (clave == 'h') {
            mostrar_ayuda();
            continue;
        }

        /* Busqueda del manejador en la tabla de comandos */
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

    /* Unico punto de salida del programa: garantiza que el descriptor se cierre y
       la memoria se libere sin importar por que camino se termine el bucle. */
    ed_cerrar(&ed);
    printf("Editor cerrado. Hasta luego.\n");

    return 0;
}
