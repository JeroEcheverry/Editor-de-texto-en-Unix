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
| `i <n> <texto>` | Inserta el texto como nueva linea `n`, desplazando las siguientes. |
| `s <palabra>` | Busca la palabra e imprime las lineas donde aparece. |
| `m` | Muestra metadatos del archivo con `fstat`: tamano, permisos, inodo y fecha. |
| `y <n>` | Copia la linea `n` al portapapeles. |
| `x <n>` | Pega el portapapeles como nueva linea `n`. |
| `u` | Deshace la ultima modificacion. |
| `r` | Rehace la modificacion deshecha. |
| `h` | Muestra la ayuda. |
| `q` | Cierra el archivo y termina el programa. |

Las lineas se numeran desde 1.

