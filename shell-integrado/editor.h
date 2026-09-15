/*
 * editor.h -- interfaz publica del editor de texto CLI.
 * Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
 *
 * Capa de disco (archivo.c, edicion.c): unicos modulos que hacen syscalls
 * sobre el archivo de texto. Capa de comandos (comandos.c): valida los
 * argumentos y llama a la capa de disco. El bucle interactivo esta en
 * repl.c (editor_ejecutar), para poder llamarlo desde main.c o desde el
 * shell (cat_edicion.c).
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <sys/types.h>   /* off_t, ssize_t */
#include <stddef.h>      /* size_t         */

/* ------------------------------------------------------------------ */
/* Constantes de configuracion                                        */
/* ------------------------------------------------------------------ */

#define ED_MAX_RUTA      512   /* longitud maxima del nombre de archivo */
#define ED_BLOQUE       4096   /* bytes por operacion de lectura/escritura */
#define ED_CAP_INICIAL    64   /* entradas iniciales del indice de lineas */
#define ED_MAX_VERSIONES  50   /* estados que guarda el historial de deshacer/rehacer */

/* ------------------------------------------------------------------ */
/* Estructuras de datos                                               */
/* ------------------------------------------------------------------ */

/*
 * Entrada del indice de lineas: donde empieza y cuanto mide cada linea.
 *
 *   "hola\nmundo\n"
 *     lineas[0] = { offset = 0, largo = 5 }  -> "hola\n"
 *     lineas[1] = { offset = 5, largo = 6 }  -> "mundo\n"
 *
 * 'largo' incluye el '\n' final, salvo en la ultima linea si el archivo
 * no termina en salto de linea.
 */
typedef struct {
    off_t  offset;
    size_t largo;
} Linea;

/*
 * Historial de deshacer/rehacer. Cada version es una copia completa del
 * archivo guardada en /tmp. 'actual' indica cual version corresponde al
 * contenido que hay ahora en el archivo real; deshacer/rehacer mueven ese
 * indice y restauran la copia correspondiente.
 */
typedef struct {
    char rutas[ED_MAX_VERSIONES][ED_MAX_RUTA];
    int  n;
    int  actual;
    int  contador;
} Historial;

/*
 * Estado del editor. Se pasa por parametro a todas las funciones en vez
 * de usar variables globales.
 */
typedef struct {
    int    fd;                  /* -1 = sin archivo abierto */
    char   ruta[ED_MAX_RUTA];

    Linea *lineas;
    size_t n_lineas;
    size_t cap;

    off_t  tam;

    char  *portapapeles;        /* NULL si esta vacio */
    size_t portapapeles_largo;

    Historial hist;
} Editor;

/* ------------------------------------------------------------------ */
/* Capa de disco  (archivo.c)                                         */
/* ------------------------------------------------------------------ */

void ed_init(Editor *ed);
int  ed_esta_abierto(const Editor *ed);
int  ed_abrir(Editor *ed, const char *ruta);
int  ed_cerrar(Editor *ed);
int  ed_indexar(Editor *ed);
int  ed_imprimir_linea(Editor *ed, size_t idx);

/* ------------------------------------------------------------------ */
/* Capa de disco: modificaciones  (edicion.c)                         */
/* ------------------------------------------------------------------ */

int  ed_anexar(Editor *ed, const char *texto);
int  ed_insertar(Editor *ed, size_t idx, const char *texto);
int  ed_borrar_linea(Editor *ed, size_t idx);
char *ed_linea_a_memoria(Editor *ed, size_t idx, size_t *largo_out);

/* ------------------------------------------------------------------ */
/* Utilidades de entrada/salida  (archivo.c)                          */
/* ------------------------------------------------------------------ */

ssize_t leer_exacto(int fd, void *buf, size_t n);
ssize_t escribir_todo(int fd, const void *buf, size_t n);

/* ------------------------------------------------------------------ */
/* Bucle interactivo  (repl.c)                                        */
/* ------------------------------------------------------------------ */

int editor_ejecutar(const char *ruta_inicial);

/* ------------------------------------------------------------------ */
/* Busqueda y metadatos  (busqueda.c)                                 */
/* ------------------------------------------------------------------ */

int ed_buscar(Editor *ed, const char *palabra);
int ed_metadatos(Editor *ed);

/* ------------------------------------------------------------------ */
/* Portapapeles  (portapapeles.c)                                     */
/* ------------------------------------------------------------------ */

int  ed_copiar(Editor *ed, size_t idx);
int  ed_pegar(Editor *ed, size_t idx);
void ed_portapapeles_liberar(Editor *ed);

/* ------------------------------------------------------------------ */
/* Historial de deshacer y rehacer  (historial.c)                     */
/* ------------------------------------------------------------------ */

void hist_init(Editor *ed);
int  hist_registrar(Editor *ed);
int  hist_deshacer(Editor *ed);
int  hist_rehacer(Editor *ed);
void hist_limpiar(Editor *ed);

/* ------------------------------------------------------------------ */
/* Capa de comandos  (comandos.c)                                     */
/* ------------------------------------------------------------------ */

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
