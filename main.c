#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void runFile(const char* path);
void runPrompt(void);
void run(const char* source);

void error(int line, const char* message);
void report(int line, const char* where, const char* message);

bool hadError = false;

int main(int argc, char* argv[]) {
    if (argc > 2) {
        printf("Uso: Wizard [script]\n");
        return 64;
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        runPrompt();
    }

    return 0;
}

void runFile(const char* path) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "No se pudo abrir el archivo: %s\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* buffer = malloc(fileSize + 1);

    if (buffer == NULL) {
        fprintf(stderr, "No hay suficiente memoria.\n");
        fclose(file);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    fclose(file);

    run(buffer);

    free(buffer);

    if (hadError) {
        exit(65);
    }
}

void runPrompt(void) {
    char line[1024];

    for (;;) {
        printf("> ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        run(line);

        hadError = false;
    }
}

void run(const char* source) {
    
    printf("Código recibido: %s", source);

}

void error(int line, const char* message) {
    report(line, "", message);
}

void report(int line, const char* where, const char* message) {
    fprintf(stderr, "[line %d] Error%s: %s\n", line, where, message);
    hadError = true;
}