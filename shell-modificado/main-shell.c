#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64
#define MAX_LINE 2048

/* Tabla de comandos registrados en el shell, con su metadata para 'help' */
Command commands[] = {
    /* --- Categoria: Datos --- */
    {
        "d_create", "datos",
        "d_create <archivo> \"<texto>\"",
        "Crea un archivo escribiendo un texto en el.",
        "open(2), write(2), close(2)",
        cmd_d_create
    },
    {
        "d_read", "datos",
        "d_read <archivo>",
        "Lee y muestra el contenido de un archivo.",
        "open(2), read(2), close(2)",
        cmd_d_read
    },
    {
        "d_info", "datos",
        "d_info <archivo>",
        "Muestra metadatos detallados de un archivo.",
        "stat(2)",
        cmd_d_info
    },
    {
        "d_copy", "datos",
        "d_copy <origen> <destino>",
        "Copia recursiva o lineal de bytes entre archivos.",
        "open(2), read(2), write(2), close(2)",
        cmd_d_copy
    },

    /* --- Categoria: Memoria --- */
    {
        "m_sbrk", "memoria",
        "m_sbrk <incremento_bytes>",
        "Modifica el program break de la seccion heap.",
        "sbrk(2) / brk(2)",
        cmd_m_sbrk
    },
    {
        "m_mmap", "memoria",
        "m_mmap <tamano_bytes>",
        "Mapea una zona de memoria anonima y escribe un patron.",
        "mmap(2), munmap(2)",
        cmd_m_mmap
    },
    {
        "m_info", "memoria",
        "m_info",
        "Muestra el estado del mapa de memoria del proceso actual.",
        "Lectura directa de /proc/self/status",
        cmd_m_info
    },

    /* --- Categoria: Monitoreo/Procesos --- */
    {
        "p_fork", "monitoreo",
        "p_fork",
        "Crea un proceso hijo, demuestra sincronizacion y codigos de salida.",
        "fork(2), getpid(2), getppid(2), waitpid(2)",
        cmd_p_fork
    },
    {
        "p_exec", "monitoreo",
        "p_exec <comando> [argumentos...]",
        "Crea un proceso hijo y ejecuta un comando externo del sistema.",
        "fork(2), execvp(3), waitpid(2)",
        cmd_p_exec
    },
    {
        "p_kill", "monitoreo",
        "p_kill <pid> <numero_senal>",
        "Envia una senal especifica a un proceso en ejecucion.",
        "kill(2)",
        cmd_p_kill
    },
    {
        "p_monitor", "monitoreo",
        "p_monitor",
        "Muestra el uso detallado de recursos de la CPU y memoria del shell.",
        "getrusage(2)",
        cmd_p_monitor
    },

    /* --- Categoria: Edicion --- */
    {
        "e_edit", "edicion",
        "e_edit [archivo]",
        "Abre el editor de texto interactivo integrado.",
        "open(2), read(2), write(2), lseek(2), ftruncate(2), fstat(2), unlink(2), close(2)",
        cmd_e_edit
    },

    /* --- Categoria: Utilidades --- */
    {
        "saludar", "utilidades",
        "saludar",
        "Muestra un saludo personalizado para el usuario actual.",
        "getuid(2)",
        cmd_saludar
    },
    {
        "despedir", "utilidades",
        "despedir",
        "Muestra un mensaje de despedida personalizado para el usuario actual.",
        "getuid(2)",
        cmd_despedir
    },
    {
        "hora", "utilidades",
        "hora",
        "Muestra la hora actual del sistema.",
        "time(2)",
        cmd_hora
    },
    {
        "fecha", "utilidades",
        "fecha",
        "Muestra la fecha actual del sistema.",
        "time(2)",
        cmd_fecha
    }
};

const int num_commands = sizeof(commands) / sizeof(commands[0]);

/*
 * Divide 'line' en argumentos (argv), soportando comillas dobles para
 * argumentos con espacios, por ejemplo: d_create archivo.txt "con espacios".
 * Modifica 'line' colocando '\0' donde terminan los tokens.
 */
int parse_line(char *line, char **argv) {
    int argc = 0;
    char *p = line;
    int in_quote = 0;
    char *arg_start = NULL;

    while (*p) {
        if (!in_quote && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
            if (arg_start != NULL) {
                *p = '\0';
                argv[argc++] = arg_start;
                arg_start = NULL;
            }
        } else if (*p == '"') {
            if (in_quote) {
                *p = '\0';
                argv[argc++] = arg_start;
                arg_start = NULL;
                in_quote = 0;
            } else {
                in_quote = 1;
                arg_start = p + 1;
            }
        } else {
            if (arg_start == NULL) {
                arg_start = p;
            }
        }
        p++;
    }
    if (arg_start != NULL) {
        argv[argc++] = arg_start;
    }
    argv[argc] = NULL;
    return argc;
}

/* Muestra la ayuda del shell: general, por categoria, o de un comando puntual */
void print_help(const char *arg) {
    if (arg == NULL) {
        printf(COLOR_TITLE "\n--- Shell de Aprendizaje de Syscalls (SO2026B) ---\n" COLOR_RESET);
        printf("Este shell te permite explorar como funcionan las llamadas al sistema en Linux.\n");
        printf("Los comandos estan clasificados en categorias.\n\n");
        printf("Categorias disponibles:\n");
        printf("  " COLOR_CATEGORY "datos" COLOR_RESET "      - Comandos de archivos y datos (open, read, write, stat, ...)\n");
        printf("  " COLOR_CATEGORY "memoria" COLOR_RESET "    - Comandos de control de heap y memoria (sbrk, mmap, ...)\n");
        printf("  " COLOR_CATEGORY "monitoreo" COLOR_RESET "  - Comandos de procesos, senales y recursos (fork, exec, kill, getrusage)\n");
        printf("  " COLOR_CATEGORY "utilidades" COLOR_RESET " - Comandos utiles del sistema (saludar, hora, fecha, despedir)\n");
        printf("  " COLOR_CATEGORY "edicion" COLOR_RESET "    - Aplicaciones interactivas que toman el control (e_edit)\n\n");
        printf("Uso general:\n");
        printf("  " COLOR_PROMPT "help <categoria>" COLOR_RESET "  - Muestra comandos especificos de una categoria.\n");
        printf("  " COLOR_PROMPT "help <comando>" COLOR_RESET "    - Explica el uso y las syscalls de un comando especifico.\n");
        printf("  " COLOR_PROMPT "clear" COLOR_RESET "             - Limpia la pantalla.\n");
        printf("  " COLOR_PROMPT "exit" COLOR_RESET "              - Cierra el shell.\n\n");
        return;
    }

    if (strcmp(arg, "datos") == 0 || strcmp(arg, "memoria") == 0 ||
        strcmp(arg, "monitoreo") == 0 || strcmp(arg, "utilidades") == 0 ||
        strcmp(arg, "edicion") == 0) {
        printf(COLOR_TITLE "\n--- Categoria: %s ---\n" COLOR_RESET, arg);
        for (int i = 0; i < num_commands; i++) {
            if (strcmp(commands[i].category, arg) == 0) {
                printf("  " COLOR_PROMPT "%-10s" COLOR_RESET " -> %s\n", commands[i].name, commands[i].description);
                printf("                " COLOR_INFO "Llamada(s): %s" COLOR_RESET "\n\n", commands[i].syscalls);
            }
        }
        return;
    }

    for (int i = 0; i < num_commands; i++) {
        if (strcmp(commands[i].name, arg) == 0) {
            printf(COLOR_TITLE "\nDetalles de comando: %s\n" COLOR_RESET, commands[i].name);
            printf("  Descripcion:  %s\n", commands[i].description);
            printf("  Uso:          " COLOR_PARAM "%s" COLOR_RESET "\n", commands[i].usage);
            printf("  Syscalls:     " COLOR_SYSCALL "%s" COLOR_RESET "\n\n", commands[i].syscalls);
            return;
        }
    }

    printf(COLOR_ERROR "Categoria o comando '%s' no reconocido. Escribe 'help' para ver la ayuda.\n" COLOR_RESET, arg);
}

/* Bucle principal del shell: lee, tokeniza, busca el comando y lo ejecuta */
int main() {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    printf(COLOR_TITLE "========================================================\n" COLOR_RESET);
    printf(COLOR_TITLE "   Shell Educativo (Llamadas al Sistema de Linux) (EAFITOS)\n" COLOR_RESET);
    printf(COLOR_INFO "    Asignatura: SO2026B0 (Sistemas Operativos)\n" COLOR_RESET);
    printf(COLOR_INFO "    Escribe 'help' para iniciar. Desarrollado en C.\n" COLOR_RESET);
    printf(COLOR_TITLE "========================================================\n\n" COLOR_RESET);

    while (1) {
        printf(COLOR_PROMPT "eafitOS> " COLOR_RESET);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        int argc = parse_line(line, argv);
        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            printf(COLOR_INFO "Saliendo del shell educativo. ¡Hasta luego!\n" COLOR_RESET);
            break;
        } else if (strcmp(argv[0], "clear") == 0) {
            printf("\033[H\033[J");
            continue;
        } else if (strcmp(argv[0], "help") == 0) {
            if (argc > 1) {
                print_help(argv[1]);
            } else {
                print_help(NULL);
            }
            continue;
        }

        int found = 0;
        for (int i = 0; i < num_commands; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].handler(argc, argv);
                found = 1;
                break;
            }
        }

        if (!found) {
            printf(COLOR_ERROR "Comando '%s' no encontrado en el shell educativo.\n" COLOR_RESET, argv[0]);
            printf(COLOR_INFO "Prueba usando 'p_exec %s' si quieres ejecutarlo como un binario de Linux externo, o escribe 'help'.\n" COLOR_RESET, argv[0]);
        }
        printf("\n");
    }

    return 0;
}
