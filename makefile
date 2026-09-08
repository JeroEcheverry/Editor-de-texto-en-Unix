# ====================================================================================
#  Makefile  --  Editor de texto CLI (SO2026B - EAFIT)
# ====================================================================================
#  Targets exigidos por el enunciado: all, clean
# ====================================================================================

CC      = gcc

# -Wall -Wextra : activan TODOS los avisos del compilador. Un warning es un bug
#                 que todavía no te ha mordido. La rúbrica pide "compilación limpia".
# -std=gnu99    : estándar C99 + extensiones GNU.
# -g            : símbolos de depuración, para poder usar gdb/valgrind.
# -D_GNU_SOURCE : expone las declaraciones POSIX/GNU (ftruncate, etc.).
CFLAGS  = -Wall -Wextra -std=gnu99 -g -D_GNU_SOURCE

TARGET  = editor
SRCS    = main.c repl.c archivo.c edicion.c busqueda.c portapapeles.c historial.c comandos.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

# Enlazado final: junta los .o en el ejecutable.
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regla genérica: cada .c produce su .o.
# La dependencia de editor.h hace que TODO se recompile si cambia la cabecera.
%.o: %.c editor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
