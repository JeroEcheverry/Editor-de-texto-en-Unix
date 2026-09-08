/**
 * ====================================================================================
 *  portapapeles.c  --  Copiar y pegar lineas
 * ====================================================================================
 *  Implementa los comandos 'y' (copiar una linea) y 'x' (pegar).
 *
 *  El portapapeles es un buffer reservado con malloc que guarda el texto de una
 *  linea. Vive dentro de la struct Editor, de modo que su contenido se conserva
 *  entre comandos y se libera al cerrar el archivo.
 *
 *  El tamano del buffer se ajusta exactamente a la linea copiada, asi que no hay
 *  limite de longitud.
 * ====================================================================================
 */

#include "editor.h"

#include <stdio.h>    /* printf */
#include <stdlib.h>   /* free   */

/**
 * Libera la memoria del portapapeles y lo deja vacio.
 * Es seguro llamarla aunque el portapapeles ya este vacio.
 */
void ed_portapapeles_liberar(Editor *ed)
{
    free(ed->portapapeles);          /* free(NULL) esta definido y no hace nada */
    ed->portapapeles       = NULL;
    ed->portapapeles_largo = 0;
}

/* ==================================================================================
 * y <n>  --  Copiar la linea n al portapapeles
 * ================================================================================== */

/**
 * Copia la linea 'idx' (base 0) al portapapeles.
 *
 * El contenido anterior se descarta: el portapapeles guarda una sola linea, igual
 * que el registro de borrado de editores como vi. La linea se guarda sin su '\n'
 * final, porque el salto lo vuelve a poner ed_insertar al pegar.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_copiar(Editor *ed, size_t idx)
{
    if (!ed_esta_abierto(ed)) return -1;
    if (idx >= ed->n_lineas)  return -1;

    size_t largo = 0;
    char  *texto = ed_linea_a_memoria(ed, idx, &largo);
    if (texto == NULL) return -1;

    /* Se libera lo que hubiera antes para no perder ese bloque de memoria. */
    ed_portapapeles_liberar(ed);

    ed->portapapeles       = texto;   /* Se adopta el buffer, no se copia otra vez */
    ed->portapapeles_largo = largo;

    return 0;
}

/* ==================================================================================
 * x <n>  --  Pegar el portapapeles como nueva linea n
 * ================================================================================== */

/**
 * Inserta el contenido del portapapeles como una nueva linea en la posicion 'idx'
 * (base 0), desplazando hacia adelante las lineas siguientes.
 *
 * La insercion se delega en ed_insertar, que ya resuelve el desplazamiento de
 * bytes y la actualizacion del indice. El portapapeles no se vacia al pegar, de
 * modo que la misma linea puede pegarse varias veces.
 *
 * Retorna 0 en exito, -1 en error.
 */
int ed_pegar(Editor *ed, size_t idx)
{
    if (!ed_esta_abierto(ed)) return -1;

    if (ed->portapapeles == NULL) {
        printf("Error: el portapapeles esta vacio. Usa primero: y <n>\n");
        return -1;
    }

    return ed_insertar(ed, idx, ed->portapapeles);
}
