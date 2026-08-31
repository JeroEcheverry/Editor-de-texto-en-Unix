# Editor de texto en Unix

Editor de texto interactivo por linea de comandos escrito en C, desarrollado para
la asignatura **Sistemas Operativos (SO2026B)** de la Universidad EAFIT.

Toda la manipulacion del archivo se realiza mediante llamadas al sistema POSIX.
No se utilizan las funciones de alto nivel de la biblioteca estandar de C
(`fopen`, `fread`, `fwrite`, `fclose`), tal como exige el enunciado.

## Compilacion

```bash
make          # compila y genera el ejecutable ./editor
make clean    # elimina el ejecutable y los archivos objeto
```

Requisitos: `gcc` y `make` sobre Linux.

## Ejecucion

```bash
./editor                 # inicia sin archivo abierto
./editor notas.txt       # abre (o crea) notas.txt al arrancar
```

## Comandos

| Comando | Descripcion |
| :--- | :--- |
| `o <archivo>` | Abre un archivo. Si no existe, lo crea con permisos 0644. |
| `p` | Imprime el archivo completo con numeracion de lineas. |
| `p <n>` | Imprime unicamente la linea `n`. |
| `a <texto>` | Anade el texto como una nueva linea al final del archivo. |
| `d <n>` | Borra la linea `n`, desplazando el resto del archivo. |
| `h` | Muestra la ayuda. |
| `q` | Cierra el archivo y termina el programa. |

Las lineas se numeran desde 1.

## Estructura del proyecto

| Archivo | Responsabilidad |
| :--- | :--- |
| `editor.h` | Tipos de datos y prototipos. Define el contrato entre las capas. |
| `archivo.c` | Capa de disco. Unico modulo que ejecuta llamadas al sistema sobre el archivo. |
| `comandos.c` | Capa de comandos. Valida argumentos y traduce ordenes del usuario. |
| `main.c` | Bucle interactivo y despacho de comandos por tabla de punteros a funcion. |
| `makefile` | Reglas de compilacion (`all`, `clean`). |

### Decision de diseno: indice de lineas sobre el archivo

Para el kernel un archivo es una secuencia plana de bytes; el concepto de linea no
existe a ese nivel. El editor mantiene en memoria un **indice de lineas**: un arreglo
dinamico en el que cada entrada guarda el desplazamiento donde empieza una linea y
cuantos bytes ocupa.

El contenido del texto **no** se carga en memoria: la unica fuente de verdad es el
archivo en disco. Las consecuencias de esa decision son:

- Las modificaciones se hacen directamente sobre el archivo con `lseek`, `read`,
  `write` y `ftruncate`, que es lo que pide el enunciado para el comando `d`.
- El consumo de memoria depende del numero de lineas, no del tamano del archivo,
  por lo que el editor puede operar sobre archivos mayores que la memoria disponible.
- El indice se reconstruye completo despues de cada modificacion. Es una operacion
  lineal en el tamano del archivo, pero elimina la posibilidad de que el indice y el
  contenido queden desincronizados.

## Llamadas al sistema utilizadas

| Llamada | Uso en el proyecto |
| :--- | :--- |
| `open(2)` | Abrir o crear el archivo con `O_RDWR \| O_CREAT` y permisos 0644. |
| `read(2)` | Leer bloques del archivo para indexar, imprimir y desplazar bytes. |
| `write(2)` | Escribir en el archivo y en STDOUT (descriptor 1). |
| `lseek(2)` | Posicionar el cursor del archivo y obtener su tamano con `SEEK_END`. |
| `ftruncate(2)` | Recortar el archivo tras borrar una linea. |
| `close(2)` | Liberar el descriptor al cerrar o al cambiar de archivo. |

## Manejo de errores

Toda llamada al sistema verifica su valor de retorno. Los fallos se reportan con
`perror`, que traduce el codigo de `errno` a un mensaje legible. Las funciones
`leer_exacto` y `escribir_todo` repiten la operacion cuando `read` o `write`
transfieren menos bytes de los solicitados, y la reintentan cuando fue interrumpida
por una senal (`EINTR`).

El programa tiene un unico punto de salida, en el que se cierra el descriptor y se
libera la memoria del indice.

## Estado del proyecto

Implementado: `o`, `p`, `a`, `d`, `q`.

Pendiente: insercion arbitraria (`i`), busqueda (`s`), metadatos con `fstat` (`m`),
copiar y pegar (`y` / `x`), deshacer y rehacer (`u` / `r`) e integracion con el
shell de la asignatura.

## Integrantes

_(completar con los nombres del equipo)_
