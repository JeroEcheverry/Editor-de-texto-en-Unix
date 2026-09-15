#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Codigos de escape ANSI para colorear la salida en consola */
#define COLOR_RESET     "\033[0m"
#define COLOR_PROMPT    "\033[1;36m"   /* prompt interactivo */
#define COLOR_SYSCALL   "\033[1;35m"   /* nombre de syscalls */
#define COLOR_PARAM     "\033[0;33m"   /* parametros */
#define COLOR_RESULT    "\033[1;32m"   /* resultados exitosos */
#define COLOR_ERROR     "\033[1;31m"   /* errores */
#define COLOR_INFO      "\033[0;90m"   /* texto informativo */
#define COLOR_TITLE     "\033[1;97m"   /* titulos y banners */
#define COLOR_CATEGORY  "\033[1;34m"   /* categorias del comando 'help' */

/*
 * Macros para imprimir las syscalls que ejecuta cada comando, al estilo
 * de 'strace', para que se vea que llamada se hizo y con que resultado.
 */

#define LOG_SYSCALL(syscall_name, format, ...) \
    printf(COLOR_SYSCALL "[syscall] " syscall_name COLOR_RESET "(" format ") ... " COLOR_RESET, ##__VA_ARGS__)

#define LOG_SYSCALL_RESULT(result) \
    printf("= " COLOR_RESULT "%ld" COLOR_RESET "\n", (long)(result))

#define LOG_SYSCALL_RESULT_PTR(result) \
    printf("= " COLOR_RESULT "%p" COLOR_RESET "\n", (void*)(result))

#define LOG_SYSCALL_ERROR(err_name) \
    printf("= " COLOR_ERROR "-1 (%s)" COLOR_RESET "\n", err_name)

/* Un comando del shell y su metadata para el sistema de ayuda */
typedef struct {
    const char *name;
    const char *category;    /* "datos", "memoria", "monitoreo", "utilidades", "edicion" */
    const char *usage;
    const char *description;
    const char *syscalls;
    int (*handler)(int argc, char **argv);
} Command;

/* --- Categoria: Datos y Archivos (cat_datos.c) --- */
int cmd_d_create(int argc, char **argv);
int cmd_d_read(int argc, char **argv);
int cmd_d_info(int argc, char **argv);
int cmd_d_copy(int argc, char **argv);

/* --- Categoria: Memoria (cat_memoria.c) --- */
int cmd_m_sbrk(int argc, char **argv);
int cmd_m_mmap(int argc, char **argv);
int cmd_m_info(int argc, char **argv);

/* --- Categoria: Monitoreo/Procesos (cat_monitoreo.c) --- */
int cmd_p_fork(int argc, char **argv);
int cmd_p_exec(int argc, char **argv);
int cmd_p_kill(int argc, char **argv);
int cmd_p_monitor(int argc, char **argv);

/* --- Categoria: Edicion (cat_edicion.c) --- */
int cmd_e_edit(int argc, char **argv);

/* --- Categoria: Utilidades (cat_util.c) --- */
int cmd_saludar(int argc, char **argv);
int cmd_despedir(int argc, char **argv);
int cmd_hora(int argc, char **argv);
int cmd_fecha(int argc, char **argv);

#endif /* SHELL_H */
