/**
 * ====================================================================================
 *  editor.h  --  Interfaz publica del editor de texto CLI
 * ====================================================================================
 *  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 *  El editor esta dividido en dos capas:
 *
 *    Capa de disco    (archivo.c)  : unico modulo que ejecuta llamadas al sistema
 *                                    sobre el archivo de texto. No conoce comandos
 *                                    ni formatos de pantalla.
 *
 *    Capa de comandos (comandos.c) : valida los argumentos que escribe el usuario
 *                                    y los traduce en llamadas a la capa de disco.
 *
 *  El bucle interactivo vive en main.c.
 * ====================================================================================
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <sys/types.h>   /* off_t, ssize_t */
#include <stddef.h>      /* size_t         */

/* ------------------------------------------------------------------ */
/* Constantes de configuracion                                        */
/* ------------------------------------------------------------------ */

#define ED_MAX_RUTA      512   /* Longitud maxima del nombre de archivo             */
#define ED_BLOQUE       4096   /* Bytes por operacion de lectura/escritura.
                                  4096 es el tamano de pagina tipico en Linux.      */
#define ED_CAP_INICIAL    64   /* Entradas reservadas al crear el indice de lineas  */

/* ------------------------------------------------------------------ */
/* Estructuras de datos                                               */
/* ------------------------------------------------------------------ */

/**
 * Entrada del indice de lineas.
 *
 * Para el kernel un archivo es una secuencia plana de bytes: no existe el
 * concepto de linea. El editor lo construye recorriendo el archivo y anotando
 * donde empieza y cuanto mide cada linea.
 *
 *   Archivo "hola\nmundo\n"
 *     lineas[0] = { offset = 0, largo = 5 }   -> "hola\n"
 *     lineas[1] = { offset = 5, largo = 6 }   -> "mundo\n"
 *
 * El campo 'largo' incluye el '\n' final. La unica linea que puede no tenerlo
 * es la ultima, cuando el archivo no termina en salto de linea.
 */
typedef struct {
    off_t  offset;   /* Byte en el que arranca la linea, contado desde 0 */
    size_t largo;    /* Bytes que ocupa, incluyendo el '\n' si lo tiene  */
} Linea;

/**
 * Estado completo del editor.
 *
 * Se pasa por parametro a todas las funciones en lugar de usar variables
 * globales. Eso permite, mas adelante, manejar varios archivos abiertos a la
 * vez sin modificar la logica existente.
 */
typedef struct {
    int    fd;                  /* Descriptor devuelto por open(). -1 = sin archivo  */
    char   ruta[ED_MAX_RUTA];   /* Nombre del archivo abierto                        */

    Linea *lineas;              /* Indice de lineas: arreglo dinamico                */
    size_t n_lineas;            /* Lineas registradas en el indice                   */
    size_t cap;                 /* Entradas que caben sin volver a hacer realloc     */

    off_t  tam;                 /* Tamano del archivo en bytes                       */
} Editor;

/* ------------------------------------------------------------------ */
/* Capa de disco  (archivo.c)                                         */
/* ------------------------------------------------------------------ */

/* Inicializa la estructura en un estado vacio. No accede al disco. */
void ed_init(Editor *ed);

/* Indica si hay un archivo abierto. Devuelve 1 si lo hay, 0 si no. */
int  ed_esta_abierto(const Editor *ed);

/* Abre o crea el archivo con open(2) usando O_RDWR|O_CREAT.
   Si ya habia un archivo abierto lo cierra primero.
   Devuelve 0 en exito y -1 en error (el error ya fue reportado con perror). */
int  ed_abrir(Editor *ed, const char *ruta);

/* Cierra el descriptor con close(2) y libera el indice.
   Se puede llamar aunque no haya ningun archivo abierto. */
int  ed_cerrar(Editor *ed);

/* Recorre el archivo completo contando '\n' y reconstruye el indice de lineas.
   Se invoca al abrir y despues de cada modificacion. */
int  ed_indexar(Editor *ed);

/* Escribe la linea 'idx' (base 0) en STDOUT usando write(2).
   Lee por bloques, de modo que no impone limite a la longitud de la linea.
   Anade un '\n' final si la linea no lo trae. Devuelve 0 en exito, -1 en error. */
int  ed_imprimir_linea(Editor *ed, size_t idx);

/* Anade 'texto' como una nueva linea al final del archivo.
   Usa lseek(2) con SEEK_END y write(2). */
int  ed_anexar(Editor *ed, const char *texto);

/* Borra la linea 'idx' (base 0): desplaza los bytes posteriores hacia atras
   y recorta el archivo con ftruncate(2). */
int  ed_borrar_linea(Editor *ed, size_t idx);

/* ------------------------------------------------------------------ */
/* Utilidades de entrada/salida  (archivo.c)                          */
/* ------------------------------------------------------------------ */

/* read(2) y write(2) pueden transferir menos bytes de los solicitados sin que
   eso constituya un error. Estas funciones repiten la operacion hasta completar
   la cantidad pedida y reintentan si la llamada fue interrumpida por una senal. */
ssize_t leer_exacto(int fd, void *buf, size_t n);
ssize_t escribir_todo(int fd, const void *buf, size_t n);

/* ------------------------------------------------------------------ */
/* Capa de comandos  (comandos.c)                                     */
/* ------------------------------------------------------------------ */
/* Cada funcion recibe el resto de la linea escrita por el usuario, ya sin la
   letra del comando ni los espacios iniciales. Devuelven 0 en exito y -1 en error. */

int cmd_o(Editor *ed, const char *arg);   /* o <archivo> : abrir o crear  */
int cmd_p(Editor *ed, const char *arg);   /* p [n]       : imprimir       */
int cmd_a(Editor *ed, const char *arg);   /* a <texto>   : anadir al final*/
int cmd_d(Editor *ed, const char *arg);   /* d <n>       : borrar linea   */

#endif /* EDITOR_H */
