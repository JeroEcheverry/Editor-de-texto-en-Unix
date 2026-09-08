/**
 * ====================================================================================
 *  busqueda.c  --  Busqueda de texto y metadatos del archivo
 * ====================================================================================
 *  Implementa los comandos 's' (buscar una palabra) y 'm' (mostrar metadatos).
 *
 *  Syscall destacada: fstat(2), que consulta la informacion que el sistema de
 *  archivos guarda en el inodo: tamano, permisos, numero de inodo y fechas.
 * ====================================================================================
 */

#include "editor.h"

#include <sys/stat.h>   /* fstat, struct stat, macros S_I... */
#include <unistd.h>
#include <stdio.h>      /* printf, perror                    */
#include <stdlib.h>     /* free                              */
#include <string.h>     /* strstr                            */
#include <time.h>       /* localtime, strftime               */

/* ==================================================================================
 * s <palabra>  --  Buscar una palabra en el archivo
 * ================================================================================== */

/**
 * Busca 'palabra' en todas las lineas del archivo e imprime las coincidencias
 * con su numero de linea.
 *
 * Cada linea se copia a memoria con ed_linea_a_memoria, que entrega una cadena
 * terminada en '\0' del tamano exacto de la linea. Eso permite usar strstr, que
 * es la funcion estandar de C para buscar una subcadena dentro de otra: devuelve
 * un puntero a la primera aparicion, o NULL si no la encuentra.
 *
 * La busqueda es de subcadena, no de palabra completa: buscar "casa" tambien
 * encuentra "casaca".
 *
 * Retorna el numero de lineas en las que aparecio la palabra, o -1 en error.
 */
int ed_buscar(Editor *ed, const char *palabra)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (palabra == NULL || *palabra == '\0') return -1;

    int encontradas = 0;

    for (size_t i = 0; i < ed->n_lineas; i++) {
        char *texto = ed_linea_a_memoria(ed, i, NULL);
        if (texto == NULL) return -1;

        if (strstr(texto, palabra) != NULL) {
            printf("%4zu | %s\n", i + 1, texto);
            encontradas++;
        }

        free(texto);   /* La memoria se libera en cada vuelta del bucle */
    }

    return encontradas;
}

/* ==================================================================================
 * m  --  Metadatos del archivo
 * ================================================================================== */

/**
 * Convierte el campo de permisos de struct stat a la notacion de nueve caracteres
 * que muestra 'ls -l', por ejemplo "rw-r--r--".
 *
 * El campo st_mode es un entero en el que cada permiso ocupa un bit. Las macros
 * S_IRUSR, S_IWUSR, etc. son las mascaras que permiten consultar cada bit con el
 * operador AND: si el resultado es distinto de cero, el permiso esta activo.
 *
 * El buffer 'salida' debe tener espacio para al menos 10 caracteres.
 */
static void permisos_a_texto(mode_t modo, char *salida)
{
    salida[0] = (modo & S_IRUSR) ? 'r' : '-';   /* Dueno: lectura    */
    salida[1] = (modo & S_IWUSR) ? 'w' : '-';   /* Dueno: escritura  */
    salida[2] = (modo & S_IXUSR) ? 'x' : '-';   /* Dueno: ejecucion  */
    salida[3] = (modo & S_IRGRP) ? 'r' : '-';   /* Grupo             */
    salida[4] = (modo & S_IWGRP) ? 'w' : '-';
    salida[5] = (modo & S_IXGRP) ? 'x' : '-';
    salida[6] = (modo & S_IROTH) ? 'r' : '-';   /* Otros             */
    salida[7] = (modo & S_IWOTH) ? 'w' : '-';
    salida[8] = (modo & S_IXOTH) ? 'x' : '-';
    salida[9] = '\0';
}

/**
 * Imprime los metadatos del archivo abierto.
 *
 * Se usa fstat(2) y no stat(2). La diferencia es el primer parametro: stat recibe
 * una ruta y fstat recibe un descriptor ya abierto. Como el editor mantiene el
 * archivo abierto, fstat consulta exactamente el mismo archivo sobre el que se
 * esta trabajando, sin volver a resolver el nombre. Si alguien renombrara o
 * reemplazara el archivo mientras el editor esta corriendo, stat informaria sobre
 * el archivo nuevo y fstat sigue informando sobre el que realmente se edita.
 *
 * Datos que devuelve struct stat y que se muestran aqui:
 *   st_size  : tamano en bytes.
 *   st_mode  : tipo de archivo y permisos.
 *   st_ino   : numero de inodo, el identificador del archivo dentro de su
 *              sistema de archivos. El nombre es solo una entrada de directorio
 *              que apunta a este numero.
 *   st_nlink : cuantos nombres (enlaces duros) apuntan a ese inodo.
 *   st_mtime : fecha y hora de la ultima modificacion del contenido.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_metadatos(Editor *ed)
{
    if (!ed_esta_abierto(ed)) return -1;

    struct stat st;

    if (fstat(ed->fd, &st) == -1) {
        perror("fstat");
        return -1;
    }

    char permisos[10];
    permisos_a_texto(st.st_mode, permisos);

    /* st_mtime es un tiempo en segundos desde el 1 de enero de 1970 (epoch Unix).
       localtime lo convierte a fecha y hora local, y strftime le da formato. */
    char fecha[64];
    struct tm *t = localtime(&st.st_mtime);
    if (t != NULL) {
        strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", t);
    } else {
        fecha[0] = '\0';
    }

    printf("Archivo:       %s\n",        ed->ruta);
    printf("Tamano:        %ld bytes\n", (long)st.st_size);
    printf("Lineas:        %zu\n",       ed->n_lineas);
    printf("Permisos:      %s (%o)\n",   permisos, st.st_mode & 07777);
    printf("Inodo:         %lu\n",       (unsigned long)st.st_ino);
    printf("Enlaces:       %lu\n",       (unsigned long)st.st_nlink);
    printf("Modificado:    %s\n",        fecha);
    printf("Descriptor:    %d\n",        ed->fd);

    return 0;
}
