/**
 * ====================================================================================
 *  edicion.c  --  Capa de disco: operaciones que modifican el archivo
 * ====================================================================================
 *  Contiene las tres operaciones que alteran el contenido del archivo: anadir al
 *  final, insertar en una posicion arbitraria y borrar una linea.
 *
 *  Syscalls utilizadas: read(2), write(2), lseek(2), ftruncate(2)
 *
 *  Idea comun a las tres: el kernel no ofrece ninguna llamada que inserte o quite
 *  bytes en medio de un archivo. La unica forma de hacerlo es desplazar a mano
 *  todos los bytes posteriores y ajustar despues el tamano del archivo.
 * ====================================================================================
 */

#include "editor.h"

#include <unistd.h>     /* read, write, lseek, ftruncate */
#include <stdio.h>      /* perror                        */
#include <stdlib.h>     /* malloc, free                  */
#include <string.h>     /* memcpy, strlen                */

/* ==================================================================================
 * a <texto>  --  Anadir una linea al final
 * ================================================================================== */

/**
 * Anade 'texto' como una nueva linea al final del archivo.
 *
 * Si el archivo no termina en '\n', se antepone uno; de lo contrario el texto
 * nuevo quedaria pegado al final de la ultima linea existente.
 *
 * La linea completa se arma en un buffer reservado con malloc y se envia con un
 * unico write(2). Agrupar la escritura reduce el numero de llamadas al sistema y
 * evita que otro proceso pueda leer el archivo con la linea escrita a medias.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_anexar(Editor *ed, const char *texto)
{
    if (!ed_esta_abierto(ed)) return -1;

    size_t largo       = strlen(texto);
    int    falta_salto = 0;

    /* Se lee el ultimo byte del archivo para saber si termina en salto de linea.
       El desplazamiento -1 respecto a SEEK_END corresponde a ese ultimo byte. */
    if (ed->tam > 0) {
        char ultimo;
        if (lseek(ed->fd, -1, SEEK_END) == -1) { perror("lseek"); return -1; }
        if (leer_exacto(ed->fd, &ultimo, 1) != 1) return -1;
        if (ultimo != '\n') falta_salto = 1;
    }

    size_t n_total = largo + 1 + (falta_salto ? 1 : 0);

    char *buf = malloc(n_total);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }

    size_t n = 0;
    if (falta_salto) buf[n++] = '\n';
    memcpy(buf + n, texto, largo);
    n += largo;
    buf[n++] = '\n';

    if (lseek(ed->fd, 0, SEEK_END) == -1) {
        perror("lseek");
        free(buf);
        return -1;
    }
    if (escribir_todo(ed->fd, buf, n) == -1) {
        free(buf);
        return -1;
    }

    free(buf);

    return ed_indexar(ed);          /* El archivo cambio: se rehace el indice */
}

/* ==================================================================================
 * i <n> <texto>  --  Insertar una linea en una posicion arbitraria
 * ================================================================================== */

/**
 * Inserta 'texto' como una nueva linea en la posicion 'idx' (base 0), desplazando
 * hacia adelante la linea que ocupaba esa posicion y todas las siguientes.
 *
 * Si 'idx' es igual al numero de lineas, la insercion equivale a anadir al final
 * y se delega en ed_anexar, que ya resuelve el caso del archivo sin '\n' final.
 *
 * ALGORITMO
 *
 *   Abrir un hueco de 'hueco' bytes en la posicion 'off' implica mover toda la
 *   region [off, tam) esa misma cantidad hacia adelante. El desplazamiento se
 *   recorre DEL FINAL HACIA EL PRINCIPIO porque origen y destino se solapan (el
 *   destino queda por delante del origen): copiar de principio a fin
 *   sobrescribiria bytes que todavia no se han leido. Es el mismo criterio que
 *   distingue a memcpy de memmove cuando las regiones se solapan.
 *
 *   No hace falta ftruncate: el archivo crece solo al escribir mas alla de su
 *   ultimo byte.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_insertar(Editor *ed, size_t idx, const char *texto)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (idx > ed->n_lineas)   return -1;

    /* Insertar despues de la ultima linea es exactamente anadir al final. */
    if (idx == ed->n_lineas) {
        return ed_anexar(ed, texto);
    }

    off_t  off   = ed->lineas[idx].offset;   /* Posicion donde se abre el hueco */
    size_t largo = strlen(texto);
    size_t hueco = largo + 1;                /* El texto mas su '\n' */

    /* --- Fase 1: desplazar la cola hacia adelante, del final al principio --- */

    char  bloque[ED_BLOQUE];
    off_t fin = ed->tam;    /* Limite superior de la region que falta por mover */

    while (fin > off) {
        off_t  restante = fin - off;
        size_t pedir    = (restante > ED_BLOQUE) ? ED_BLOQUE : (size_t)restante;

        off_t src = fin - (off_t)pedir;      /* Ultimo bloque aun sin mover */

        if (lseek(ed->fd, src, SEEK_SET) == -1) { perror("lseek"); return -1; }
        if (leer_exacto(ed->fd, bloque, pedir) != (ssize_t)pedir) return -1;

        if (lseek(ed->fd, src + (off_t)hueco, SEEK_SET) == -1) { perror("lseek"); return -1; }
        if (escribir_todo(ed->fd, bloque, pedir) == -1) return -1;

        fin = src;
    }

    /* --- Fase 2: escribir la linea nueva en el hueco --- */

    char *buf = malloc(hueco);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }
    memcpy(buf, texto, largo);
    buf[largo] = '\n';

    if (lseek(ed->fd, off, SEEK_SET) == -1) {
        perror("lseek");
        free(buf);
        return -1;
    }
    if (escribir_todo(ed->fd, buf, hueco) == -1) {
        free(buf);
        return -1;
    }

    free(buf);

    return ed_indexar(ed);
}

/* ==================================================================================
 * d <n>  --  Borrar una linea
 * ================================================================================== */

/**
 * Borra la linea 'idx' (base 0) del archivo.
 *
 * ALGORITMO
 *
 *   1. Desplazamiento: todo lo que viene despues de la linea se copia hacia
 *      atras, sobrescribiendo los bytes de la linea eliminada.
 *   2. Recorte: al terminar, el archivo conserva una copia sobrante al final.
 *      ftruncate(2) lo reduce al tamano correcto.
 *
 *   Aqui el destino queda POR DETRAS del origen, de modo que el recorrido correcto
 *   es del principio hacia el final: es la direccion opuesta a la de ed_insertar,
 *   y por la misma razon de solapamiento explicada alli.
 *
 *   El desplazamiento se hace por bloques y no cargando el archivo completo en
 *   memoria, de modo que el editor funciona con archivos mas grandes que la
 *   memoria disponible.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_borrar_linea(Editor *ed, size_t idx)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (idx >= ed->n_lineas)  return -1;

    off_t  inicio = ed->lineas[idx].offset;
    size_t largo  = ed->lineas[idx].largo;

    off_t src = inicio + (off_t)largo;   /* Origen: primer byte despues de la linea */
    off_t dst = inicio;                  /* Destino: donde empezaba la linea        */

    char bloque[ED_BLOQUE];

    while (src < ed->tam) {
        off_t  restante = ed->tam - src;
        size_t pedir    = (restante > ED_BLOQUE) ? ED_BLOQUE : (size_t)restante;

        /* Leer desde la posicion de origen */
        if (lseek(ed->fd, src, SEEK_SET) == -1) { perror("lseek"); return -1; }
        ssize_t leidos = leer_exacto(ed->fd, bloque, pedir);
        if (leidos <= 0) break;

        /* Escribir en la posicion de destino */
        if (lseek(ed->fd, dst, SEEK_SET) == -1) { perror("lseek"); return -1; }
        if (escribir_todo(ed->fd, bloque, (size_t)leidos) == -1) return -1;

        src += leidos;
        dst += leidos;
    }

    /* Se recorta la copia sobrante del final. Si la linea borrada era la ultima,
       el bucle anterior no movio ningun byte y este ftruncate hace todo el trabajo. */
    off_t nuevo_tam = ed->tam - (off_t)largo;
    if (ftruncate(ed->fd, nuevo_tam) == -1) {
        perror("ftruncate");
        return -1;
    }

    return ed_indexar(ed);
}
