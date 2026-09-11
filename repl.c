/*
 * repl.c -- bucle interactivo y despacho de comandos del editor.
 * Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 * Ciclo lectura-evaluacion-impresion: lee una linea con fgets, toma el
 * primer caracter como comando y el resto como argumento, ejecuta el
 * manejador correspondiente y repite hasta 'q' o Ctrl+D.
 *
 * El bucle esta en editor_ejecutar (no en main) para poder llamarlo tanto
 * desde main.c (programa independiente) como desde cat_edicion.c (shell
 * eafitOS).
 */
#include "editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ENTRADA 8192

/* ---------------------------------------------------------------- */
/* Tabla de comandos                                                   */
/* ---------------------------------------------------------------- */
/* Mismo patron del shell de la asignatura: tabla de punteros a funcion
   en vez de una cadena de if/else. Agregar un comando es escribir su
   manejador y anadir una fila aqui. */

typedef int (*Manejador)(Editor *ed, const char *arg);

typedef struct {
    char        clave;
    const char *uso;
    const char *descripcion;
    Manejador   fn;
} ComandoEd;

static const ComandoEd tabla[] = {
    { 'o', "o <archivo>", "Abre un archivo (lo crea si no existe).", cmd_o },
    { 'p', "p [n]",       "Imprime la linea n, o todo el archivo.",  cmd_p },
    { 'a', "a <texto>",   "Anade el texto como linea al final.",     cmd_a },
    { 'd', "d <n>",       "Borra la linea n.",                       cmd_d },
    { 'i', "i <n> <txt>", "Inserta el texto como nueva linea n.",    cmd_i },
    { 's', "s <palabra>", "Busca una palabra e imprime las lineas.",  cmd_s },
    { 'm', "m",           "Muestra los metadatos del archivo.",       cmd_m },
    { 'y', "y <n>",       "Copia la linea n al portapapeles.",        cmd_y },
    { 'x', "x <n>",       "Pega el portapapeles como nueva linea n.", cmd_x },
    { 'u', "u",           "Deshace la ultima modificacion.",          cmd_u },
    { 'r', "r",           "Rehace la modificacion deshecha.",         cmd_r },
};

static const int n_comandos = (int)(sizeof(tabla) / sizeof(tabla[0]));

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

/*
 * Separa el comando (primer caracter) de su argumento. A diferencia del
 * shell de la asignatura, aqui no se tokeniza: todo lo que sigue al
 * comando es el argumento tal cual, para que 'a hola mundo' conserve los
 * espacios (mismo comportamiento que el editor ed de Unix).
 *
 * Nunca devuelve NULL: si no hay argumento, apunta al terminador nulo.
 */
static char *separar_argumento(char *linea)
{
    char *p = linea + 1;
    while (*p == ' ' || *p == '\t') p++;
    return p;
}

/* Quita el salto de linea que deja fgets al final de la cadena. */
static void quitar_salto(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

/*
 * Bucle principal del editor. 'ruta_inicial' es el archivo a abrir al
 * arrancar, o NULL para empezar sin archivo. Libera todos los recursos
 * antes de retornar. Siempre retorna 0.
 */
int editor_ejecutar(const char *ruta_inicial)
{
    Editor ed;
    ed_init(&ed);

    char entrada[MAX_ENTRADA];

    if (ruta_inicial != NULL) {
        cmd_o(&ed, ruta_inicial);
    }

    while (1) {
        printf("ed> ");
        fflush(stdout);   /* el prompt no lleva '\n', hay que forzar el vaciado */

        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            printf("\n");
            break;
        }

        quitar_salto(entrada);

        if (entrada[0] == '\0') continue;

        char clave = entrada[0];

        if (clave == 'q') {
            break;
        }
        if (clave == 'h') {
            mostrar_ayuda();
            continue;
        }

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

    ed_cerrar(&ed);
    printf("Editor cerrado. Hasta luego.\n");

    return 0;
}
