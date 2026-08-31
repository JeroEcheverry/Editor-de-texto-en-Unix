/**
 * ====================================================================================
 *  archivo.c  --  CAPA DE DISCO
 * ====================================================================================
 *  Aquí vive TODO el contacto con el kernel. Ninguna otra parte del programa llama
 *  a open/read/write/lseek/ftruncate/close sobre el archivo de texto.
 *
 *  Syscalls usadas: open(2), read(2), write(2), lseek(2), ftruncate(2), close(2)
 *  Prohibido (por enunciado): fopen, fread, fwrite, fclose.
 * ====================================================================================
 */

#include "editor.h"

#include <fcntl.h>      /* open, O_RDWR, O_CREAT           */
#include <unistd.h>     /* read, write, lseek, close, ftruncate */
#include <stdio.h>      /* perror, printf                  */
#include <stdlib.h>     /* malloc, realloc, free           */
#include <string.h>     /* memcpy, strncpy, strlen         */
#include <errno.h>      /* errno                           */

/* ==================================================================================
 * 1. UTILIDADES DE E/S ROBUSTA
 * ==================================================================================
 * El error #1 de los principiantes con syscalls: asumir que read(fd, buf, 100)
 * siempre devuelve 100. NO. Puede devolver 40 porque llegó una señal, porque es
 * un pipe, porque el archivo se acabó. Devolver menos NO es un error.
 * Por eso todo se hace en bucle.
 */

/**
 * Lee exactamente 'n' bytes (o hasta que se acabe el archivo).
 * Retorna: cuántos bytes leyó de verdad, o -1 si hubo un error real.
 */
ssize_t leer_exacto(int fd, void *buf, size_t n)
{
    size_t total = 0;                 /* Cuántos llevamos acumulados */
    char  *p     = (char *)buf;       /* Puntero que avanza dentro del búfer */

    while (total < n) {
        ssize_t r = read(fd, p + total, n - total);

        if (r < 0) {
            if (errno == EINTR) continue;   /* Interrumpido por señal: reintentar */
            perror("read");
            return -1;
        }
        if (r == 0) break;            /* EOF: no hay más bytes que leer */

        total += (size_t)r;
    }
    return (ssize_t)total;
}

/**
 * Escribe exactamente 'n' bytes. write() también puede escribir de a poquitos.
 * Retorna: n si lo logró, -1 si falló.
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
 * 2. CICLO DE VIDA DEL EDITOR
 * ================================================================================== */

/**
 * Pone la estructura en un estado limpio. Todo puntero a NULL, todo contador a 0,
 * fd a -1 (que NO es un descriptor válido, así distinguimos "sin archivo" de
 * "archivo abierto en el fd 0").
 */
void ed_init(Editor *ed)
{
    ed->fd       = -1;
    ed->ruta[0]  = '\0';
    ed->lineas   = NULL;
    ed->n_lineas = 0;
    ed->cap      = 0;
    ed->tam      = 0;
}

int ed_esta_abierto(const Editor *ed)
{
    return ed->fd >= 0;
}

/**
 * Abre (o crea) el archivo.
 *
 *   O_RDWR  : lo queremos leer Y escribir. Un editor necesita ambos.
 *   O_CREAT : si no existe, créalo. El enunciado lo pide explícitamente.
 *   0644    : permisos en octal -> dueño rw-, grupo r--, otros r--.
 *             Solo se aplican si el archivo se CREA. Si ya existía, se respetan
 *             los suyos. Además el sistema les resta la umask del proceso.
 */
int ed_abrir(Editor *ed, const char *ruta)
{
    /* Si ya había un archivo abierto, lo cerramos: si no, fugamos el descriptor.
       Los file descriptors son un recurso finito del proceso (ulimit -n). */
    if (ed_esta_abierto(ed)) {
        ed_cerrar(ed);
    }

    int fd = open(ruta, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");        /* perror traduce errno a texto: "Permission denied", etc. */
        return -1;
    }

    ed->fd = fd;
    strncpy(ed->ruta, ruta, ED_MAX_RUTA - 1);
    ed->ruta[ED_MAX_RUTA - 1] = '\0';   /* strncpy NO garantiza el '\0'. Lo ponemos a mano. */

    /* Reservamos el índice de líneas con un tamaño inicial razonable. */
    ed->lineas = malloc(ED_CAP_INICIAL * sizeof(Linea));
    if (ed->lineas == NULL) {
        perror("malloc");
        close(fd);
        ed->fd = -1;
        return -1;
    }
    ed->cap      = ED_CAP_INICIAL;
    ed->n_lineas = 0;

    /* Recorremos el archivo para saber dónde está cada línea. */
    if (ed_indexar(ed) == -1) {
        ed_cerrar(ed);
        return -1;
    }

    return 0;
}

/**
 * Cierra el archivo y libera la memoria. Es idempotente: llamarla dos veces
 * no rompe nada. Esto es lo que garantiza "salir sin dejar fugas".
 */
int ed_cerrar(Editor *ed)
{
    int r = 0;

    if (ed->fd >= 0) {
        if (close(ed->fd) == -1) {
            perror("close");
            r = -1;
        }
        ed->fd = -1;
    }

    free(ed->lineas);      /* free(NULL) es legal y no hace nada. */
    ed->lineas   = NULL;
    ed->n_lineas = 0;
    ed->cap      = 0;
    ed->tam      = 0;
    ed->ruta[0]  = '\0';

    return r;
}

/* ==================================================================================
 * 3. EL ÍNDICE DE LÍNEAS
 * ================================================================================== */

/**
 * Agrega una entrada al índice, creciendo el arreglo si hace falta.
 * 'static' = solo visible dentro de este archivo .c (encapsulamiento en C).
 *
 * Estrategia de crecimiento: DUPLICAR la capacidad. Así el costo total de N
 * inserciones es O(N) amortizado, en vez de O(N²) si creciéramos de a uno.
 */
static int indice_agregar(Editor *ed, off_t offset, size_t largo)
{
    if (ed->n_lineas == ed->cap) {
        size_t nueva_cap = (ed->cap == 0) ? ED_CAP_INICIAL : ed->cap * 2;

        /* Usamos un puntero temporal: si realloc falla devuelve NULL, y si
           hubiéramos asignado directo a ed->lineas perderíamos el bloque
           original -> fuga de memoria clásica. */
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
 * Reconstruye el índice recorriendo el archivo de principio a fin.
 *
 * Se lee por BLOQUES de 4096 bytes, no byte por byte: cada read() es un cruce
 * al kernel (cambio de contexto). Leer 1 MB de a un byte = 1.000.000 de syscalls;
 * de a 4 KB = 256 syscalls. Esa es exactamente la razón por la que existe fread().
 *
 * DECISIÓN DE DISEÑO: reindexamos completo después de cada modificación.
 * Es O(tamaño del archivo), pero elimina toda una clase de bugs (índices
 * desincronizados que corrompen el archivo). Para los tamaños de esta evaluación
 * es intercambio correcto: robustez > microoptimización.
 */
int ed_indexar(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;

    /* lseek al final devuelve el tamaño del archivo en bytes. Truco clásico. */
    off_t tam = lseek(ed->fd, 0, SEEK_END);
    if (tam == -1) { perror("lseek"); return -1; }
    ed->tam = tam;

    /* Volvemos al inicio para recorrerlo. */
    if (lseek(ed->fd, 0, SEEK_SET) == -1) { perror("lseek"); return -1; }

    ed->n_lineas = 0;               /* Descartamos el índice viejo */

    char    bloque[ED_BLOQUE];
    off_t   pos    = 0;             /* Offset absoluto del primer byte del bloque */
    off_t   inicio = 0;             /* Dónde empezó la línea que estamos armando  */
    ssize_t leidos;

    while ((leidos = read(ed->fd, bloque, sizeof(bloque))) > 0) {
        for (ssize_t i = 0; i < leidos; i++) {
            if (bloque[i] == '\n') {
                off_t fin_linea = pos + i;               /* offset absoluto del '\n' */
                size_t largo = (size_t)(fin_linea - inicio + 1);  /* +1 = incluir '\n' */

                if (indice_agregar(ed, inicio, largo) == -1) return -1;
                inicio = fin_linea + 1;                  /* La siguiente arranca después */
            }
        }
        pos += leidos;
    }

    if (leidos == -1) { perror("read"); return -1; }

    /* CASO BORDE: archivo que no termina en '\n'.
       Quedan bytes sueltos desde 'inicio' hasta el final: son una línea también. */
    if (inicio < ed->tam) {
        if (indice_agregar(ed, inicio, (size_t)(ed->tam - inicio)) == -1) return -1;
    }

    return 0;
}

/* ==================================================================================
 * 4. OPERACIONES SOBRE LÍNEAS
 * ================================================================================== */

/**
 * Copia la línea 'idx' (base 0) dentro de 'destino'.
 * NO añade '\0' al final: devuelve la cantidad de bytes para que el llamador sepa.
 */
ssize_t ed_leer_linea(Editor *ed, size_t idx, char *destino, size_t cap)
{
    if (!ed_esta_abierto(ed))   return -1;
    if (idx >= ed->n_lineas)    return -1;

    size_t largo = ed->lineas[idx].largo;
    if (largo > cap) largo = cap;          /* Nunca escribir fuera del búfer */

    /* Nos posicionamos en el byte exacto donde empieza la línea... */
    if (lseek(ed->fd, ed->lineas[idx].offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    /* ...y leemos justo su longitud. Ni un byte más. */
    return leer_exacto(ed->fd, destino, largo);
}

/**
 * Añade 'texto' como línea nueva al final.
 *
 * Sutileza: si el archivo NO termina en '\n', escribir directamente pegaría el
 * texto nuevo al final de la última línea. Hay que poner el salto primero.
 */
int ed_anexar(Editor *ed, const char *texto)
{
    if (!ed_esta_abierto(ed)) return -1;

    size_t largo = strlen(texto);
    if (largo + 1 > ED_MAX_LINEA) {
        fprintf(stderr, "Error: línea demasiado larga (máx %d bytes).\n", ED_MAX_LINEA - 1);
        return -1;
    }

    /* ¿El archivo termina en '\n'? Miramos el último byte.
       lseek con offset negativo desde SEEK_END: -1 = "el último byte". */
    int falta_salto = 0;
    if (ed->tam > 0) {
        char ultimo;
        if (lseek(ed->fd, -1, SEEK_END) == -1) { perror("lseek"); return -1; }
        if (leer_exacto(ed->fd, &ultimo, 1) != 1) return -1;
        if (ultimo != '\n') falta_salto = 1;
    }

    /* Armamos la línea completa en memoria y la escribimos de UN solo write().
       Un write = una syscall = una operación atómica frente a otros procesos. */
    char buf[ED_MAX_LINEA + 1];
    size_t n = 0;

    if (falta_salto) buf[n++] = '\n';
    memcpy(buf + n, texto, largo);
    n += largo;
    buf[n++] = '\n';                       /* Toda línea termina en salto */

    /* SEEK_END mueve el cursor al final: escribir ahí = anexar. */
    if (lseek(ed->fd, 0, SEEK_END) == -1) { perror("lseek"); return -1; }
    if (escribir_todo(ed->fd, buf, n) == -1) return -1;

    return ed_indexar(ed);                 /* El mapa cambió: lo rehacemos */
}

/**
 * Borra la línea 'idx' (base 0).
 *
 * ALGORITMO (el corazón del proyecto):
 *   1. La línea ocupa los bytes [inicio, inicio+largo).
 *   2. Todo lo que viene DESPUÉS hay que moverlo 'largo' bytes hacia atrás.
 *      No existe una syscall que "quite" bytes del medio: SOBRESCRIBIMOS.
 *   3. Al terminar, el archivo tiene una cola duplicada. La cortamos con
 *      ftruncate() al tamaño nuevo.
 *
 * Se mueve por bloques de 4 KB, no cargando todo el archivo en RAM: así el
 * editor funciona con archivos más grandes que la memoria disponible.
 */
int ed_borrar_linea(Editor *ed, size_t idx)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (idx >= ed->n_lineas)  return -1;

    off_t  inicio = ed->lineas[idx].offset;
    size_t largo  = ed->lineas[idx].largo;

    off_t src = inicio + (off_t)largo;   /* De dónde leo (después de la línea muerta) */
    off_t dst = inicio;                  /* Dónde escribo (encima de la línea muerta) */

    char bloque[ED_BLOQUE];

    while (src < ed->tam) {
        /* Cuánto falta por mover; nunca más de un bloque a la vez. */
        off_t restante = ed->tam - src;
        size_t pedir = (restante > ED_BLOQUE) ? ED_BLOQUE : (size_t)restante;

        /* --- LEER desde la posición fuente --- */
        if (lseek(ed->fd, src, SEEK_SET) == -1) { perror("lseek"); return -1; }
        ssize_t leidos = leer_exacto(ed->fd, bloque, pedir);
        if (leidos <= 0) break;

        /* --- ESCRIBIR en la posición destino --- */
        if (lseek(ed->fd, dst, SEEK_SET) == -1) { perror("lseek"); return -1; }
        if (escribir_todo(ed->fd, bloque, (size_t)leidos) == -1) return -1;

        src += leidos;
        dst += leidos;
    }

    /* Cortamos la cola sobrante. Si borramos la ÚLTIMA línea, el bucle de arriba
       no movió ni un byte y este ftruncate hace todo el trabajo. */
    off_t nuevo_tam = ed->tam - (off_t)largo;
    if (ftruncate(ed->fd, nuevo_tam) == -1) {
        perror("ftruncate");
        return -1;
    }

    return ed_indexar(ed);
}