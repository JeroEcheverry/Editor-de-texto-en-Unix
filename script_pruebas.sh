#!/bin/bash
# ====================================================================
#  demo.sh -- Demostracion del funcionamiento del editor de texto
#  Universidad EAFIT - Sistemas Operativos (SO2026B) - Parcial 1
#
#  Ejecuta el editor alimentandolo por su entrada estandar y muestra
#  el resultado de cada operacion. Esto es posible porque el bucle del
#  editor lee las ordenes con fgets(..., stdin), y a fgets le da igual
#  si los bytes vienen del teclado o de una redireccion.
#
#  Uso:  chmod +x demo.sh  &&  ./demo.sh
# ====================================================================

ARCHIVO=demo.txt

# paso <titulo> : imprime un encabezado para separar cada demostracion.
paso() {
    echo
    echo "=================================================="
    echo "  $1"
    echo "=================================================="
}

make clean > /dev/null && make > /dev/null || { echo "Fallo la compilacion"; exit 1; }
rm -f "$ARCHIVO"      # se parte de cero en cada ejecucion


paso "1. Crear el archivo y anadir lineas (comandos o, a)"
./editor <<FIN
o $ARCHIVO
a primera linea
a segunda linea
a tercera linea
p
q
FIN


paso "2. Insertar una linea en el medio (comando i)"
./editor <<FIN
o $ARCHIVO
i 2 linea insertada
p
q
FIN


paso "3. Borrar una linea (comando d)"
./editor <<FIN
o $ARCHIVO
d 2
p
q
FIN


paso "4. Copiar y pegar (comandos y, x)"
./editor <<FIN
o $ARCHIVO
y 1
x 3
p
q
FIN


paso "5. Buscar una palabra (comando s)"
./editor <<FIN
o $ARCHIVO
s linea
q
FIN


paso "6. Deshacer y rehacer (comandos u, r)"
./editor <<FIN
o $ARCHIVO
a linea temporal
u
p
r
p
q
FIN


paso "7. Metadatos del archivo con fstat (comando m)"
./editor <<FIN
o $ARCHIVO
m
q
FIN


paso "8. Robustez: manejo de entradas invalidas"
./editor <<FIN
p
o $ARCHIVO
p 0
p 99
d abc
i
x 1
z
o /tmp
o /etc/shadow
q
FIN


paso "Contenido final de $ARCHIVO"
cat "$ARCHIVO"
echo
