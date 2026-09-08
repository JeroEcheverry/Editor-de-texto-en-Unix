# Integracion del editor con el shell eafitOS

Pasos para compilar el editor dentro del shell de la asignatura
(`evalenciEAFIT/SO2026B`, carpeta `shell`).

## 1. Copiar los archivos del editor a la carpeta del shell

Todos los `.c` y `.h` del editor **excepto `main.c`**. El shell ya tiene su propia
funcion `main`, y tener dos provocaria un error de enlazado:

```
editor.h  repl.c  archivo.c  edicion.c  busqueda.c
portapapeles.c  historial.c  comandos.c  cat_edicion.c
```

## 2. `shell.h` — declarar el comando nuevo

Agregar antes del bloque de la categoria Utilidades:

```c
/* --- Categoría: Edición (cat_edicion.c) --- */
int cmd_e_edit(int argc, char **argv);   /* Editor de texto integrado */
```

## 3. `main.c` del shell — registrar el comando en la tabla

Agregar esta entrada al arreglo `commands[]`, antes del bloque de Utilidades:

```c
    /* --- Categoría: Edición --- */
    {
        "e_edit", "edicion",
        "e_edit [archivo]",
        "Abre el editor de texto interactivo integrado.",
        "open(2), read(2), write(2), lseek(2), ftruncate(2), fstat(2), unlink(2), close(2)",
        cmd_e_edit
    },
```

## 4. `main.c` del shell — registrar la categoria en la ayuda

La funcion `print_help` tiene las categorias escritas a mano en dos lugares, asi que
hay que tocar los dos. Es una limitacion del shell original: agregar una categoria
obliga a modificar la funcion de ayuda.

**4a.** En el listado de categorias, despues de la linea de `utilidades`:

```c
        printf("  " COLOR_CATEGORY "edicion" COLOR_RESET "    - Aplicaciones interactivas que toman el control (e_edit)\n\n");
```

(quitar el `\n\n` final de la linea de `utilidades` y dejarle uno solo)

**4b.** En la condicion que valida el nombre de la categoria:

```c
    if (strcmp(arg, "datos") == 0 || strcmp(arg, "memoria") == 0 || 
        strcmp(arg, "monitoreo") == 0 || strcmp(arg, "utilidades") == 0 ||
        strcmp(arg, "edicion") == 0) {
```

## 5. `Makefile` del shell

Agregar los fuentes del editor a `SRCS`:

```make
SRCS = main.c cat_datos.c cat_memoria.c cat_monitoreo.c cat_util.c \
       cat_edicion.c repl.c archivo.c edicion.c busqueda.c portapapeles.c historial.c comandos.c
```

Hacer que la regla de los objetos dependa tambien de `editor.h`, para que un cambio
en la cabecera recompile lo que corresponde:

```make
%.o: %.c shell.h editor.h
	$(CC) $(CFLAGS) -c $< -o $@
```

### Dos errores del Makefile original que hay que corregir

**`clean` no funciona.** La regla usa `$(TARGET)`, una variable que no existe en ese
Makefile (la del ejecutable se llama `ARCHSALIDA`), asi que el binario nunca se
borra:

```make
clean:
	rm -f $(ARCHSALIDA) $(OBJS)
```

**`all` ejecuta el programa.** La regla de enlazado termina con `./$(ARCHSALIDA)`, de
modo que `make` lanza el shell en vez de solo compilarlo. Eso impide usar `make` en
un script de pruebas. Hay que quitar esa linea:

```make
$(ARCHSALIDA): $(OBJS)
	$(CC) $(CFLAGS) -o $(ARCHSALIDA) $(OBJS)
```

## 6. Compilar y probar

```bash
make clean
make
./eafitOS
```

Dentro del shell:

```
eafitOS> help
eafitOS> help edicion
eafitOS> e_edit notas.txt
ed> h
ed> a primera linea
ed> p
ed> q
eafitOS> exit
```

El editor toma el control de la entrada estandar mientras esta abierto, y lo
devuelve al shell al escribir `q`. Todos los recursos (descriptor, indice,
portapapeles y archivos temporales de `/tmp`) se liberan antes de retornar.
