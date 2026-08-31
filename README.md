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