/**
 * ====================================================================================
 *  archivo.c  --  Capa de disco: apertura, indexado y lectura
 * ====================================================================================
 *  Contiene el ciclo de vida del archivo (abrir y cerrar), la construccion del
 *  indice de lineas, la lectura de lineas y las utilidades de entrada/salida que
 *  usa el resto del proyecto.
 *
 *  Las operaciones que MODIFICAN el archivo estan en edicion.c.
 *
 *  Syscalls utilizadas: open(2), read(2), write(2), lseek(2), close(2)
 *
 *  Segun el enunciado, queda prohibido el uso de fopen, fread, fwrite y fclose
 *  para manipular el archivo. Ninguna de esas funciones aparece en este archivo.
 * ====================================================================================
 */

#include "editor.h"

#include <fcntl.h>      /* open, O_RDWR, O_CREAT                */
#include <unistd.h>     /* read, write, lseek, ftruncate, close */
#include <stdio.h>      /* perror, printf, fflush               */
#include <stdlib.h>     /* malloc, realloc, free                */
#include <string.h>     /* memcpy, strncpy, strlen              */
#include <errno.h>      /* errno, EINTR                         */

/* ==================================================================================
 * 1. Utilidades de entrada/salida
 * ==================================================================================
 * read(2) y write(2) no garantizan transferir todos los bytes solicitados en una
 * sola llamada. Un retorno menor al pedido no es un error. Ademas, si una senal
 * interrumpe la llamada antes de transferir datos, esta falla con errno == EINTR
 * y debe reintentarse. Las dos funciones siguientes encapsulan ambos casos.
 */

/**
 * Lee hasta 'n' bytes desde 'fd' hacia 'buf'.
 *
 * Retorna: la cantidad de bytes leidos realmente (puede ser menor que 'n' si se
 *          alcanzo el fin del archivo), o -1 si ocurrio un error.
 */
ssize_t leer_exacto(int fd, void *buf, size_t n)
{
    size_t total = 0;
    char  *p     = (char *)buf;

    while (total < n) {
        ssize_t r = read(fd, p + total, n - total);

        if (r < 0) {
            if (errno == EINTR) continue;   /* Interrumpida por una senal: reintentar */
            perror("read");
            return -1;
        }
        if (r == 0) break;                  /* Fin del archivo */

        total += (size_t)r;
    }
    return (ssize_t)total;
}

/**
 * Escribe los 'n' bytes de 'buf' en 'fd'.
 *
 * Retorna: 'n' si logro escribirlos todos, o -1 si ocurrio un error.
 */
ssize_t escribir_todo(int fd, const void *buf, size_t n)
{
    size_t      total = 0;
    const char *p     = (const char *)buf;

    while (total < n) {
        ssize_t w = write(fd, p + total, n - total);

        if (w < 0) {
            if (errno == EINTR) continue;
            perror("write");
            return -1;
        }
        total += (size_t)w;
    }
    return (ssize_t)total;
}

/* ==================================================================================
 * 2. Ciclo de vida del editor
 * ================================================================================== */

/**
 * Deja la estructura en un estado vacio y conocido.
 *
 * El descriptor se inicializa en -1 porque ese valor nunca es un descriptor
 * valido: permite distinguir "sin archivo" de "archivo abierto en el fd 0".
 */
void ed_init(Editor *ed)
{
    ed->fd       = -1;
    ed->ruta[0]  = '\0';
    ed->lineas   = NULL;
    ed->n_lineas = 0;
    ed->cap      = 0;
    ed->tam      = 0;

    ed->portapapeles       = NULL;
    ed->portapapeles_largo = 0;

    hist_init(ed);
}

int ed_esta_abierto(const Editor *ed)
{
    return ed->fd >= 0;
}

/**
 * Abre el archivo indicado, o lo crea si no existe.
 *
 * Banderas de open(2):
 *   O_RDWR  : lectura y escritura, ambas necesarias para un editor.
 *   O_CREAT : crear el archivo cuando no existe, tal como pide el enunciado.
 *   0644    : permisos aplicados solo si el archivo se crea (rw-r--r--).
 *             El sistema les resta la umask del proceso.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_abrir(Editor *ed, const char *ruta)
{
    /* Si ya habia un archivo abierto se cierra antes de abrir el nuevo.
       De lo contrario se perderia el descriptor anterior, que es un recurso
       limitado del proceso (ver ulimit -n). */
    if (ed_esta_abierto(ed)) {
        ed_cerrar(ed);
    }

    int fd = open(ruta, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    ed->fd = fd;
    strncpy(ed->ruta, ruta, ED_MAX_RUTA - 1);
    ed->ruta[ED_MAX_RUTA - 1] = '\0';   /* strncpy no garantiza el terminador nulo */

    /* Reserva inicial del indice de lineas. */
    ed->lineas = malloc(ED_CAP_INICIAL * sizeof(Linea));
    if (ed->lineas == NULL) {
        perror("malloc");
        close(fd);
        ed->fd = -1;
        return -1;
    }
    ed->cap      = ED_CAP_INICIAL;
    ed->n_lineas = 0;

    if (ed_indexar(ed) == -1) {
        ed_cerrar(ed);
        return -1;
    }

    return 0;
}

/**
 * Cierra el descriptor y libera la memoria del indice.
 *
 * Es seguro llamarla varias veces seguidas o sin archivo abierto, lo que
 * garantiza que el editor siempre termine sin descriptores ni memoria sin liberar.
 */
int ed_cerrar(Editor *ed)
{
    int r = 0;

    /* Los archivos temporales de /tmp se eliminan con unlink(2) antes de cerrar,
       para no dejar basura en el sistema al terminar el programa. */
    hist_limpiar(ed);
    ed_portapapeles_liberar(ed);

    if (ed->fd >= 0) {
        if (close(ed->fd) == -1) {
            perror("close");
            r = -1;
        }
        ed->fd = -1;
    }

    free(ed->lineas);      /* free(NULL) esta definido y no hace nada */
    ed->lineas   = NULL;
    ed->n_lineas = 0;
    ed->cap      = 0;
    ed->tam      = 0;
    ed->ruta[0]  = '\0';

    return r;
}

/* ==================================================================================
 * 3. Indice de lineas
 * ================================================================================== */

/**
 * Agrega una entrada al indice, ampliando el arreglo cuando se llena.
 *
 * La palabra clave 'static' limita la visibilidad de la funcion a este archivo.
 *
 * Al agotarse la capacidad se duplica en lugar de crecer de a uno. Asi el costo
 * total de N inserciones es lineal en N, y no cuadratico.
 */
static int indice_agregar(Editor *ed, off_t offset, size_t largo)
{
    if (ed->n_lineas == ed->cap) {
        size_t nueva_cap = (ed->cap == 0) ? ED_CAP_INICIAL : ed->cap * 2;

        /* El resultado de realloc se recibe en un puntero temporal. Si se asignara
           directamente a ed->lineas y realloc fallara (devolviendo NULL), se
           perderia la referencia al bloque original y con ella la memoria. */
        Linea *tmp = realloc(ed->lineas, nueva_cap * sizeof(Linea));
        if (tmp == NULL) {
            perror("realloc");
            return -1;
        }
        ed->lineas = tmp;
        ed->cap    = nueva_cap;
    }

    ed->lineas[ed->n_lineas].offset = offset;
    ed->lineas[ed->n_lineas].largo  = largo;
    ed->n_lineas++;
    return 0;
}

/**
 * Reconstruye el indice recorriendo el archivo de principio a fin.
 *
 * La lectura se hace por bloques de ED_BLOQUE bytes y no byte por byte. Cada
 * read(2) implica un cambio de contexto al kernel: recorrer 1 MB de a un byte
 * costaria un millon de llamadas al sistema, y de a 4 KB cuesta 256.
 *
 * El indice se reconstruye completo despues de cada modificacion. La alternativa
 * seria actualizar solo las entradas afectadas, lo que es mas rapido pero abre la
 * posibilidad de que el indice y el archivo queden desincronizados. Para los
 * tamanos de archivo de esta evaluacion se prioriza la correccion.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_indexar(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;

    /* lseek(2) hasta el final devuelve el desplazamiento resultante, que
       equivale al tamano del archivo en bytes. */
    off_t tam = lseek(ed->fd, 0, SEEK_END);
    if (tam == -1) { perror("lseek"); return -1; }
    ed->tam = tam;

    if (lseek(ed->fd, 0, SEEK_SET) == -1) { perror("lseek"); return -1; }

    ed->n_lineas = 0;               /* Se descarta el indice anterior */

    char    bloque[ED_BLOQUE];
    off_t   pos    = 0;             /* Desplazamiento absoluto del inicio del bloque */
    off_t   inicio = 0;             /* Desplazamiento donde empieza la linea actual  */
    ssize_t leidos;

    while ((leidos = read(ed->fd, bloque, sizeof(bloque))) > 0) {
        for (ssize_t i = 0; i < leidos; i++) {
            if (bloque[i] == '\n') {
                off_t  fin_linea = pos + i;                       /* posicion del '\n' */
                size_t largo     = (size_t)(fin_linea - inicio + 1);

                if (indice_agregar(ed, inicio, largo) == -1) return -1;
                inicio = fin_linea + 1;
            }
        }
        pos += leidos;
    }

    if (leidos == -1) { perror("read"); return -1; }

    /* Caso borde: archivo que no termina en '\n'. Los bytes que quedan entre
       'inicio' y el final del archivo forman una linea sin salto de linea. */
    if (inicio < ed->tam) {
        if (indice_agregar(ed, inicio, (size_t)(ed->tam - inicio)) == -1) return -1;
    }

    return 0;
}

/* ==================================================================================
 * 4. Operaciones sobre lineas
 * ================================================================================== */

/**
 * Escribe la linea 'idx' (base 0) en STDOUT.
 *
 * La linea se copia del archivo a la salida en bloques de ED_BLOQUE bytes, sin
 * almacenarla completa en memoria. De este modo el comando funciona con lineas
 * de cualquier longitud.
 *
 * Se llama a fflush(stdout) antes de escribir porque printf acumula su salida en
 * un buffer de la biblioteca estandar mientras que write(2) va directo al kernel.
 * Sin ese vaciado previo, los mensajes de printf podrian aparecer despues del
 * texto escrito con write.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_imprimir_linea(Editor *ed, size_t idx)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (idx >= ed->n_lineas)  return -1;

    size_t restante        = ed->lineas[idx].largo;
    int    termina_en_salto = 0;
    char   bloque[ED_BLOQUE];

    fflush(stdout);

    if (lseek(ed->fd, ed->lineas[idx].offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }

    while (restante > 0) {
        size_t pedir = (restante > ED_BLOQUE) ? ED_BLOQUE : restante;

        ssize_t leidos = leer_exacto(ed->fd, bloque, pedir);
        if (leidos <= 0) break;

        if (escribir_todo(1, bloque, (size_t)leidos) == -1) return -1;

        termina_en_salto = (bloque[leidos - 1] == '\n');
        restante -= (size_t)leidos;
    }

    /* La ultima linea del archivo puede no traer '\n'. Se agrega en la salida
       para que el siguiente prompt no quede pegado al texto. */
    if (!termina_en_salto) {
        if (escribir_todo(1, "\n", 1) == -1) return -1;
    }

    return 0;
}

/**
 * Copia la linea 'idx' (base 0) a un buffer nuevo reservado con malloc.
 *
 * El '\n' final no se incluye y el buffer queda terminado en '\0', de modo que
 * puede tratarse como una cadena de C normal. El tamano se ajusta a la linea, asi
 * que no hay limite de longitud ni riesgo de truncamiento.
 *
 * El llamador es responsable de liberar el buffer con free().
 *
 * Retorna el puntero al buffer, o NULL si hubo error.
 */
char *ed_linea_a_memoria(Editor *ed, size_t idx, size_t *largo_out)
{
    if (!ed_esta_abierto(ed)) return NULL;
    if (idx >= ed->n_lineas)  return NULL;

    size_t largo = ed->lineas[idx].largo;

    char *buf = malloc(largo + 1);
    if (buf == NULL) {
        perror("malloc");
        return NULL;
    }

    if (lseek(ed->fd, ed->lineas[idx].offset, SEEK_SET) == -1) {
        perror("lseek");
        free(buf);
        return NULL;
    }
    if (leer_exacto(ed->fd, buf, largo) != (ssize_t)largo) {
        free(buf);
        return NULL;
    }

    /* Se descarta el salto de linea final si lo hay. */
    if (largo > 0 && buf[largo - 1] == '\n') largo--;

    buf[largo] = '\0';
    if (largo_out) *largo_out = largo;

    return buf;
}
