/**
 * ====================================================================================
 *  historial.c  --  Deshacer y rehacer mediante archivos de intercambio (swap)
 * ====================================================================================
 *  Implementa los comandos 'u' (deshacer) y 'r' (rehacer).
 *
 *  ESTRATEGIA
 *
 *  Cada vez que el archivo cambia, se guarda una copia completa de su contenido en
 *  un archivo temporal dentro de /tmp. Esa copia es una "version". El historial es
 *  la lista de versiones mas un indice que senala cual de ellas corresponde al
 *  contenido que hay en este momento en el archivo real.
 *
 *      rutas[0]      rutas[1]      rutas[2]      rutas[3]
 *      (al abrir)    (tras 'a')    (tras 'd')    (tras 'i')
 *                                      ^
 *                                    actual
 *
 *  Deshacer retrocede el indice y copia esa version sobre el archivo real.
 *  Rehacer lo avanza y hace lo mismo. Si despues de deshacer se hace una
 *  modificacion nueva, las versiones que quedaban por delante se descartan: ese es
 *  el comportamiento habitual del deshacer en cualquier editor.
 *
 *  POR QUE /tmp Y NO MEMORIA
 *
 *  Guardar el historial en archivos y no en RAM permite que el editor deshaga
 *  cambios sobre archivos mas grandes que la memoria disponible, que es la misma
 *  razon por la que el editor no carga el texto en memoria. El precio es una copia
 *  completa por version, aceptable para los tamanos de esta evaluacion.
 *
 *  Syscalls utilizadas: open(2), read(2), write(2), lseek(2), ftruncate(2),
 *                       close(2), unlink(2), getpid(2)
 * ====================================================================================
 */

#include "editor.h"

#include <fcntl.h>      /* open, O_WRONLY, O_CREAT, O_TRUNC, O_RDONLY */
#include <unistd.h>     /* read, write, lseek, ftruncate, close, unlink, getpid */
#include <stdio.h>      /* printf, perror, snprintf                   */
#include <string.h>

/* ==================================================================================
 * Utilidades internas
 * ================================================================================== */

/**
 * Copia el contenido completo del archivo abierto hacia el archivo 'ruta'.
 *
 * El archivo temporal se crea con permisos 0600 (lectura y escritura solo para el
 * dueno). En /tmp escriben todos los usuarios del sistema, de modo que dejar un
 * temporal legible expondria el contenido del documento a cualquiera.
 *
 * Retorna 0 en exito, -1 en error.
 */
static int copiar_a_swap(Editor *ed, const char *ruta)
{
    int fd_dst = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd_dst == -1) {
        perror("open (swap)");
        return -1;
    }

    if (lseek(ed->fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd_dst);
        return -1;
    }

    char    bloque[ED_BLOQUE];
    ssize_t leidos;

    while ((leidos = read(ed->fd, bloque, sizeof(bloque))) > 0) {
        if (escribir_todo(fd_dst, bloque, (size_t)leidos) == -1) {
            close(fd_dst);
            return -1;
        }
    }

    if (leidos == -1) {
        perror("read");
        close(fd_dst);
        return -1;
    }

    close(fd_dst);
    return 0;
}

/**
 * Copia el contenido del archivo 'ruta' sobre el archivo que edita el usuario.
 *
 * Despues de copiar se llama a ftruncate para ajustar el tamano: si la version
 * restaurada es mas corta que el contenido actual, sin ese recorte quedaria una
 * cola de bytes viejos al final.
 *
 * Retorna 0 en exito, -1 en error.
 */
static int restaurar_desde_swap(Editor *ed, const char *ruta)
{
    int fd_src = open(ruta, O_RDONLY);
    if (fd_src == -1) {
        perror("open (swap)");
        return -1;
    }

    if (lseek(ed->fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd_src);
        return -1;
    }

    char    bloque[ED_BLOQUE];
    off_t   total = 0;
    ssize_t leidos;

    while ((leidos = read(fd_src, bloque, sizeof(bloque))) > 0) {
        if (escribir_todo(ed->fd, bloque, (size_t)leidos) == -1) {
            close(fd_src);
            return -1;
        }
        total += leidos;
    }

    if (leidos == -1) {
        perror("read");
        close(fd_src);
        return -1;
    }

    close(fd_src);

    if (ftruncate(ed->fd, total) == -1) {
        perror("ftruncate");
        return -1;
    }

    return ed_indexar(ed);
}

/**
 * Elimina del disco las versiones que estan por delante de 'desde' y las descarta
 * del historial. Se usa cuando el usuario modifica el archivo despues de deshacer.
 */
static void descartar_desde(Editor *ed, int desde)
{
    for (int i = desde; i < ed->hist.n; i++) {
        unlink(ed->hist.rutas[i]);
    }
    ed->hist.n = desde;
}

/* ==================================================================================
 * Interfaz publica
 * ================================================================================== */

void hist_init(Editor *ed)
{
    ed->hist.n        = 0;
    ed->hist.actual   = -1;
    ed->hist.contador = 0;
}

/**
 * Guarda el estado actual del archivo como una version nueva.
 *
 * Se llama al abrir el archivo (para tener el estado inicial) y despues de cada
 * modificacion exitosa.
 *
 * Si el historial esta lleno se descarta la version mas antigua: se elimina su
 * archivo con unlink y las demas se corren una posicion hacia atras. El usuario
 * pierde la capacidad de deshacer hasta el principio, pero el editor no se queda
 * sin espacio para guardar.
 *
 * Retorna 0 en exito, -1 en error.
 */
int hist_registrar(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;

    /* Una modificacion nueva invalida las versiones que quedaban por rehacer. */
    if (ed->hist.actual >= 0 && ed->hist.actual + 1 < ed->hist.n) {
        descartar_desde(ed, ed->hist.actual + 1);
    }

    /* Si no queda cupo, se elimina la version mas antigua. */
    if (ed->hist.n == ED_MAX_VERSIONES) {
        unlink(ed->hist.rutas[0]);
        for (int i = 1; i < ed->hist.n; i++) {
            strcpy(ed->hist.rutas[i - 1], ed->hist.rutas[i]);
        }
        ed->hist.n--;
        ed->hist.actual--;
    }

    /* El nombre lleva el PID del proceso para que dos editores abiertos a la vez
       no se pisen los archivos temporales, y un consecutivo para que cada version
       tenga un nombre distinto. */
    char ruta[ED_MAX_RUTA];
    snprintf(ruta, sizeof(ruta), "/tmp/editor_%d_%d.swap",
             (int)getpid(), ed->hist.contador);

    if (copiar_a_swap(ed, ruta) == -1) return -1;

    strncpy(ed->hist.rutas[ed->hist.n], ruta, ED_MAX_RUTA - 1);
    ed->hist.rutas[ed->hist.n][ED_MAX_RUTA - 1] = '\0';

    ed->hist.n++;
    ed->hist.actual = ed->hist.n - 1;
    ed->hist.contador++;

    return 0;
}

/**
 * Restaura la version anterior a la actual.
 *
 * Retorna 0 si deshizo, 1 si ya no hay nada que deshacer, -1 en error.
 */
int hist_deshacer(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (ed->hist.actual <= 0) return 1;     /* La version 0 es el estado inicial */

    if (restaurar_desde_swap(ed, ed->hist.rutas[ed->hist.actual - 1]) == -1) {
        return -1;
    }

    ed->hist.actual--;
    return 0;
}

/**
 * Restaura la version siguiente a la actual.
 *
 * Retorna 0 si rehizo, 1 si ya no hay nada que rehacer, -1 en error.
 */
int hist_rehacer(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (ed->hist.actual < 0)                return 1;
    if (ed->hist.actual + 1 >= ed->hist.n)  return 1;

    if (restaurar_desde_swap(ed, ed->hist.rutas[ed->hist.actual + 1]) == -1) {
        return -1;
    }

    ed->hist.actual++;
    return 0;
}

/**
 * Elimina todos los archivos temporales y vacia el historial.
 *
 * unlink(2) borra la entrada de directorio que apunta al inodo. Cuando ya no queda
 * ningun nombre ni ningun proceso con el archivo abierto, el sistema libera el
 * espacio. Esta es la limpieza que exige el enunciado al cerrar el editor.
 */
void hist_limpiar(Editor *ed)
{
    for (int i = 0; i < ed->hist.n; i++) {
        unlink(ed->hist.rutas[i]);
    }
    hist_init(ed);
}
