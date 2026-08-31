/**
 * ====================================================================================
 *  editor.h  --  Interfaz pública del editor de texto CLI
 * ====================================================================================
 *  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 *  Este archivo define el CONTRATO del editor: qué tipos de datos existen y qué
 *  funciones ofrece cada capa. La arquitectura está partida en dos capas:
 *
 *    CAPA DE DISCO   (archivo.c)  -> habla con el kernel vía syscalls.
 *                                    No sabe nada de comandos ni de la pantalla.
 *    CAPA DE COMANDOS(comandos.c) -> traduce lo que el usuario escribe en llamadas
 *                                    a la capa de disco. No usa syscalls de archivo
 *                                    directamente (salvo write() a FD 1 para imprimir).
 *
 *  Separarlas no es estética: permite probar la capa de disco sin teclado, y es el
 *  argumento de diseño que sustentamos en el PDF.
 * ====================================================================================
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <sys/types.h>   /* off_t, ssize_t */
#include <stddef.h>      /* size_t         */

/* ------------------------------------------------------------------ */
/* Constantes de configuración                                        */
/* ------------------------------------------------------------------ */
#define ED_MAX_LINEA    4096   /* Máximo de bytes que aceptamos en una línea       */
#define ED_MAX_RUTA      512   /* Máximo de caracteres del nombre de archivo       */
#define ED_BLOQUE       4096   /* Tamaño del búfer de trabajo para mover bytes.
                                  4096 = tamaño típico de página/bloque en Linux.  */
#define ED_CAP_INICIAL    64   /* Cuántas líneas reservamos de entrada en el índice*/

/* ------------------------------------------------------------------ */
/* Estructuras de datos                                               */
/* ------------------------------------------------------------------ */

/**
 * Una entrada del ÍNDICE DE LÍNEAS.
 *
 * El kernel no sabe qué es una línea: para él el archivo es una tira plana de
 * bytes. Las líneas son una ficción que construimos nosotros contando '\n'.
 * Esta struct guarda, para cada línea, dónde empieza y cuánto mide.
 *
 *   Archivo "hola\nmundo\n"
 *     linea[0] = { offset = 0, largo = 5 }   -> "hola\n"
 *     linea[1] = { offset = 5, largo = 6 }   -> "mundo\n"
 *
 * IMPORTANTE: 'largo' INCLUYE el '\n' final. La única línea que puede no
 * terminar en '\n' es la última, si el archivo no termina en salto de línea.
 */
typedef struct {
    off_t  offset;   /* Byte donde arranca la línea, contado desde el inicio (0)  */
    size_t largo;    /* Cuántos bytes ocupa, contando el '\n' si lo tiene         */
} Linea;

/**
 * Estado completo del editor. Un solo objeto que se pasa a todas las funciones
 * (en vez de usar variables globales: eso nos permite, más adelante, tener
 * VARIOS archivos abiertos a la vez sin reescribir nada).
 */
typedef struct {
    int    fd;                  /* File descriptor devuelto por open(). -1 = sin archivo */
    char   ruta[ED_MAX_RUTA];   /* Nombre del archivo abierto, para mensajes             */

    Linea *lineas;              /* ÍNDICE: arreglo dinámico (malloc/realloc/free)        */
    size_t n_lineas;            /* Cuántas líneas hay realmente                          */
    size_t cap;                 /* Cuántas caben sin volver a hacer realloc              */

    off_t  tam;                 /* Tamaño del archivo en bytes (según lseek SEEK_END)    */
} Editor;

/* ------------------------------------------------------------------ */
/* CAPA DE DISCO  (archivo.c)                                         */
/* ------------------------------------------------------------------ */

/* Deja la estructura en un estado limpio y conocido. No toca el disco. */
void ed_init(Editor *ed);

/* ¿Hay un archivo abierto ahora mismo? 1 = sí, 0 = no. */
int  ed_esta_abierto(const Editor *ed);

/* open(2) con O_RDWR|O_CREAT. Si ya había uno abierto, lo cierra primero.
   Devuelve 0 si todo bien, -1 si falló (ya reportó el error con perror). */
int  ed_abrir(Editor *ed, const char *ruta);

/* close(2) + free() del índice. Seguro de llamar aunque no haya nada abierto. */
int  ed_cerrar(Editor *ed);

/* Recorre TODO el archivo contando '\n' y reconstruye el índice de líneas.
   Se llama al abrir y después de cada modificación. */
int  ed_indexar(Editor *ed);

/* Copia la línea 'idx' (base 0) al búfer 'destino'.
   Devuelve cuántos bytes copió, o -1 si hubo error. */
ssize_t ed_leer_linea(Editor *ed, size_t idx, char *destino, size_t cap);

/* Añade 'texto' como una nueva línea al final del archivo (lseek SEEK_END + write). */
int  ed_anexar(Editor *ed, const char *texto);

/* Borra la línea 'idx' (base 0): sobrescribe con la cola y trunca. */
int  ed_borrar_linea(Editor *ed, size_t idx);

/* ------------------------------------------------------------------ */
/* Utilidades de E/S robusta  (archivo.c)                             */
/* ------------------------------------------------------------------ */

/* read() puede devolver MENOS bytes de los pedidos sin que sea un error.
   Estas dos funciones insisten en bucle hasta completar. Son la base de
   todo el proyecto: nunca llames read()/write() "a pelo". */
ssize_t leer_exacto(int fd, void *buf, size_t n);
ssize_t escribir_todo(int fd, const void *buf, size_t n);

/* ------------------------------------------------------------------ */
/* CAPA DE COMANDOS  (comandos.c)                                     */
/* ------------------------------------------------------------------ */
/* Todos reciben el resto de la línea que escribió el usuario (ya sin el
   comando ni los espacios iniciales). Devuelven 0 si bien, -1 si mal. */

int cmd_o(Editor *ed, const char *arg);   /* o [archivo] : abrir/crear   */
int cmd_p(Editor *ed, const char *arg);   /* p [n]       : imprimir      */
int cmd_a(Editor *ed, const char *arg);   /* a [texto]   : anexar        */
int cmd_d(Editor *ed, const char *arg);   /* d [n]       : borrar línea  */

#endif /* EDITOR_H */