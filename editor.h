/**
 * ====================================================================================
 *  editor.h  --  Interfaz publica del editor de texto CLI
 * ====================================================================================
 *  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 *  El editor esta dividido en dos capas:
 *
 *    Capa de disco    (archivo.c, edicion.c) : unicos modulos que ejecutan llamadas
 *                                    al sistema sobre el archivo de texto. No conocen
 *                                    comandos ni formatos de pantalla.
 *                                      archivo.c : abrir, cerrar, indexar, leer.
 *                                      edicion.c : anadir, insertar, borrar.
 *
 *    Capa de comandos (comandos.c) : valida los argumentos que escribe el usuario
 *                                    y los traduce en llamadas a la capa de disco.
 *
 *  El bucle interactivo vive en repl.c, dentro de la funcion editor_ejecutar,
 *  para que puedan invocarlo tanto el programa independiente (main.c) como el
 *  shell eafitOS (cat_edicion.c).
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
#define ED_MAX_VERSIONES  50   /* Estados que guarda el historial de deshacer/rehacer*/

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
 * Historial de deshacer y rehacer.
 *
 * Cada version es una copia completa del archivo guardada en el directorio /tmp
 * (un archivo de intercambio, o "swap"). El campo 'actual' indica cual de esas
 * versiones corresponde al contenido que hay ahora mismo en el archivo real.
 *
 *   rutas[0] ... rutas[actual] ... rutas[n-1]
 *                     ^                 ^
 *                 estado actual    versiones que se
 *                                  recuperan con rehacer
 *
 * Deshacer retrocede 'actual' y restaura esa version; rehacer lo avanza.
 * Todos los archivos se eliminan con unlink(2) al cerrar el editor.
 */
typedef struct {
    char rutas[ED_MAX_VERSIONES][ED_MAX_RUTA];  /* Rutas de los archivos swap    */
    int  n;          /* Versiones guardadas                                      */
    int  actual;     /* Indice de la version que refleja el archivo en disco     */
    int  contador;   /* Numero consecutivo para generar nombres unicos           */
} Historial;

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

    char  *portapapeles;        /* Linea copiada con 'y'. NULL si esta vacio         */
    size_t portapapeles_largo;  /* Longitud del texto copiado, sin el '\0'           */

    Historial hist;             /* Historial de deshacer y rehacer                   */
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

/* ------------------------------------------------------------------ */
/* Capa de disco: modificaciones  (edicion.c)                         */
/* ------------------------------------------------------------------ */

/* Anade 'texto' como una nueva linea al final del archivo.
   Usa lseek(2) con SEEK_END y write(2). */
int  ed_anexar(Editor *ed, const char *texto);

/* Inserta 'texto' como nueva linea en la posicion 'idx' (base 0), desplazando
   hacia adelante la linea que ocupaba esa posicion y las siguientes.
   Si 'idx' es igual a n_lineas, equivale a anadir al final. */
int  ed_insertar(Editor *ed, size_t idx, const char *texto);

/* Borra la linea 'idx' (base 0): desplaza los bytes posteriores hacia atras
   y recorta el archivo con ftruncate(2). */
int  ed_borrar_linea(Editor *ed, size_t idx);

/* Copia la linea 'idx' (base 0) a un buffer nuevo reservado con malloc, sin el
   '\n' final y terminado en '\0'. El llamador debe liberarlo con free().
   Si 'largo_out' no es NULL, recibe la longitud del texto. Devuelve NULL si falla. */
char *ed_linea_a_memoria(Editor *ed, size_t idx, size_t *largo_out);

/* ------------------------------------------------------------------ */
/* Utilidades de entrada/salida  (archivo.c)                          */
/* ------------------------------------------------------------------ */

/* read(2) y write(2) pueden transferir menos bytes de los solicitados sin que
   eso constituya un error. Estas funciones repiten la operacion hasta completar
   la cantidad pedida y reintentan si la llamada fue interrumpida por una senal. */
ssize_t leer_exacto(int fd, void *buf, size_t n);
ssize_t escribir_todo(int fd, const void *buf, size_t n);

/* ------------------------------------------------------------------ */
/* Bucle interactivo  (repl.c)                                        */
/* ------------------------------------------------------------------ */

/* Ejecuta el ciclo interactivo del editor hasta que el usuario escriba 'q' o
   pulse Ctrl+D. 'ruta_inicial' es el archivo a abrir al arrancar, o NULL para
   empezar sin archivo. Libera todos los recursos antes de retornar. */
int editor_ejecutar(const char *ruta_inicial);

/* ------------------------------------------------------------------ */
/* Busqueda y metadatos  (busqueda.c)                                 */
/* ------------------------------------------------------------------ */

/* Busca 'palabra' en todas las lineas e imprime las coincidencias.
   Devuelve el numero de lineas donde aparecio, o -1 en error. */
int ed_buscar(Editor *ed, const char *palabra);

/* Imprime los metadatos del archivo abierto usando fstat(2):
   tamano, permisos, numero de inodo y fecha de ultima modificacion. */
int ed_metadatos(Editor *ed);

/* ------------------------------------------------------------------ */
/* Portapapeles  (portapapeles.c)                                     */
/* ------------------------------------------------------------------ */

/* Copia la linea 'idx' (base 0) al portapapeles, reemplazando su contenido. */
int  ed_copiar(Editor *ed, size_t idx);

/* Inserta el contenido del portapapeles como nueva linea 'idx' (base 0). */
int  ed_pegar(Editor *ed, size_t idx);

/* Libera la memoria del portapapeles. Seguro de llamar si esta vacio. */
void ed_portapapeles_liberar(Editor *ed);

/* ------------------------------------------------------------------ */
/* Historial de deshacer y rehacer  (historial.c)                     */
/* ------------------------------------------------------------------ */

/* Deja el historial vacio. No accede al disco. */
void hist_init(Editor *ed);

/* Guarda el estado actual del archivo como una version nueva en /tmp.
   Se llama despues de cada modificacion y tambien al abrir el archivo. */
int  hist_registrar(Editor *ed);

/* Restaura la version anterior (deshacer). Devuelve 0 si lo hizo,
   1 si ya no hay nada que deshacer, -1 en error. */
int  hist_deshacer(Editor *ed);

/* Restaura la version siguiente (rehacer). Mismos codigos de retorno. */
int  hist_rehacer(Editor *ed);

/* Elimina con unlink(2) todos los archivos swap y vacia el historial. */
void hist_limpiar(Editor *ed);

/* ------------------------------------------------------------------ */
/* Capa de comandos  (comandos.c)                                     */
/* ------------------------------------------------------------------ */
/* Cada funcion recibe el resto de la linea escrita por el usuario, ya sin la
   letra del comando ni los espacios iniciales. Devuelven 0 en exito y -1 en error. */

int cmd_o(Editor *ed, const char *arg);   /* o <archivo>    : abrir o crear   */
int cmd_p(Editor *ed, const char *arg);   /* p [n]          : imprimir        */
int cmd_a(Editor *ed, const char *arg);   /* a <texto>      : anadir al final */
int cmd_d(Editor *ed, const char *arg);   /* d <n>          : borrar linea    */
int cmd_i(Editor *ed, const char *arg);   /* i <n> <texto>  : insertar linea  */
int cmd_s(Editor *ed, const char *arg);   /* s <palabra>    : buscar          */
int cmd_m(Editor *ed, const char *arg);   /* m              : metadatos       */
int cmd_y(Editor *ed, const char *arg);   /* y <n>          : copiar linea    */
int cmd_x(Editor *ed, const char *arg);   /* x <n>          : pegar linea     */
int cmd_u(Editor *ed, const char *arg);   /* u              : deshacer        */
int cmd_r(Editor *ed, const char *arg);   /* r              : rehacer         */

#endif /* EDITOR_H */
