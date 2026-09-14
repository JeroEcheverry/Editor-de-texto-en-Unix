# Editor de texto en Unix
Editor de texto interactivo por linea de comandos escrito en C, desarrollado para
la asignatura Sistemas Operativos (SO2026B) de la Universidad EAFIT.

Toda la manipulacion del archivo se realiza mediante llamadas al sistema.
No se utilizan las funciones de alto nivel de la biblioteca estandar de C
(`fopen`, `fread`, `fwrite`, `fclose`), tal como exige el enunciado.

## Compilacion y ejecucion
### Requisitos
Linux (o WSL sobre Windows) con gcc, make y git. En Ubuntu o Debian:

```bash
sudo apt update && sudo apt install build-essential git
```
El proyecto no usa librerias externas: solo llamadas al sistema POSIX y la
biblioteca estandar de C.
> **Nota para usuarios de WSL:** trabaja sobre el sistema de archivos nativo de
> Linux (por ejemplo `~/`), no sobre `/mnt/c`. En las unidades de Windows el
> comando `m` reporta permisos `777` porque NTFS no implementa el modelo de
> permisos de Unix, y la compilacion es notablemente mas lenta.

##### Opcion A: el editor como programa independiente

```bash
git clone https://github.com/JeroEcheverry/Editor-de-texto-en-Unix.git
cd Editor-de-texto-en-Unix
make
```
Esto genera el ejecutable `./editor`.
```bash
./editor                 # inicia sin archivo abierto
./editor notas.txt       # abre (o crea) notas.txt al arrancar
```
Dentro del editor, `h` muestra la ayuda y `q` cierra el programa.

#### Opcion B: el editor integrado en el shell eafitOS
El editor se compila dentro del shell de la asignatura. Como son dos repositorios
distintos, hay que clonarlos por separado y juntarlos.
1. Clonar los dos repositorios, uno al lado del otro
```bash
git clone https://github.com/evalenciEAFIT/SO2026B.git
git clone https://github.com/JeroEcheverry/Editor-de-texto-en-Unix.git
```
2. Situarse en la carpeta del shell
```bash
cd SO2026B/shell
```
3. Copiar los fuentes del editor (todos EXCEPTO `main.c`)
```bash
cp ../../Editor-de-texto-en-Unix/{editor.h,repl.c,archivo.c,edicion.c,busqueda.c,portapapeles.c,historial.c,comandos.c,cat_edicion.c} .
```
4. Copiar los tres archivos del shell ya modificados
```bash
cp ../../Editor-de-texto-en-Unix/shell-modificado/shell.h .
cp ../../Editor-de-texto-en-Unix/shell-modificado/Makefile .
cp ../../Editor-de-texto-en-Unix/shell-modificado/main-shell.c main.c
```
5. Compilar y ejecutar
```bash
make clean && make
./eafitOS
```
Por que no se copia `main.c`: el shell ya tiene su propia funcion `main`. Si se
copiaran las dos, el enlazado fallaria con `multiple definition of main`. El bucle
del editor vive en `repl.c`, dentro de la funcion `editor_ejecutar`, que es la que
invoca `cat_edicion.c` cuando el usuario escribe `e_edit`.
El detalle completo de la integracion esta en INTEGRACION_SHELL.md.

## Comandos
Comando	Descripcion
`o <archivo>`	Abre un archivo. Si no existe, lo crea con permisos 0644.
`p`	Imprime el archivo completo con numeracion de lineas.
`p <n>`	Imprime unicamente la linea `n`.
`a <texto>`	Anade el texto como una nueva linea al final del archivo.
`d <n>`	Borra la linea `n`, desplazando el resto del archivo.
`i <n> <texto>`	Inserta el texto como nueva linea `n`, desplazando las siguientes.
`s <palabra>`	Busca la palabra e imprime las lineas donde aparece.
`m`	Muestra metadatos del archivo con `fstat`: tamano, permisos, inodo y fecha.
`y <n>`	Copia la linea `n` al portapapeles.
`x <n>`	Pega el portapapeles como nueva linea `n`.
`u`	Deshace la ultima modificacion.
`r`	Rehace la modificacion deshecha.
`h`	Muestra la ayuda.
`q`	Cierra el archivo y termina el programa.
Las lineas se numeran desde 1.

## Script de demostracion
El repositorio incluye `script_pruebas.sh`, que ejecuta el editor de forma
automatica y muestra cada comando funcionando sobre un archivo de prueba.
```bash
chmod +x script_pruebas.sh
./script_pruebas.sh
```
