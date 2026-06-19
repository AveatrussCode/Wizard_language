#include "visualizer.h"

#include <stdio.h>
#include "raylib.h"
#include "chunk.h"
#include "value.h"
#include "vm.h"

#define COLOR_BG       (Color){24, 24, 32, 255}
#define COLOR_PANEL    (Color){38, 38, 48, 255}
#define COLOR_TEXT     (Color){230, 230, 235, 255}
#define COLOR_MUTED    (Color){150, 150, 160, 255}
#define COLOR_ACTIVE   (Color){80, 130, 255, 255}
#define COLOR_SUCCESS  (Color){80, 200, 120, 255}
#define COLOR_DANGER   (Color){220, 90, 90, 255}

static void drawPanel(Rectangle rect, const char* title) {
    DrawRectangleRounded(rect, 0.06f, 12, COLOR_PANEL);
    DrawText(title, rect.x + 20, rect.y + 15, 22, COLOR_TEXT);
    DrawLine(rect.x + 20, rect.y + 48, rect.x + rect.width - 20, rect.y + 48, COLOR_MUTED);
}

static void valueToString(Valux value, char* buffer, int size) {
    if (IS_NUMBER(value)) {
        snprintf(buffer, size, "%g", AS_NUMBER(value));
    } else if (IS_BOOL(value)) {
        snprintf(buffer, size, AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        snprintf(buffer, size, "nil");
    } else {
        snprintf(buffer, size, "unknown");
    }
}

static const char* instructionName(uint8_t instruction) {
    switch (instruction) {
        case OP_CONSTANT: return "OP_CONSTANT";
        case OP_NIL: return "OP_NIL";
        case OP_TRUE: return "OP_TRUE";
        case OP_FALSE: return "OP_FALSE";
        case OP_EQUAL: return "OP_EQUAL";
        case OP_GREATER: return "OP_GREATER";
        case OP_LESS: return "OP_LESS";
        case OP_ADD: return "OP_ADD";
        case OP_SUBTRACT: return "OP_SUBTRACT";
        case OP_MULTIPLY: return "OP_MULTIPLY";
        case OP_DIVIDE: return "OP_DIVIDE";
        case OP_NOT: return "OP_NOT";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_RETURN: return "OP_RETURN";
        default: return "UNKNOWN";
    }
}

void visualizeChunk(Chunk* chunk, const char* title, Valux resultado) {
    InitWindow(1000, 650, title);
    SetTargetFPS(60);

    int instructionOffsets[512]; 
    int instructionCount = 0;

    for (int i = 0; i < chunk->count; ) {
        if (instructionCount < 512) {
            instructionOffsets[instructionCount++] = i;
        }
        if (chunk->code[i] == OP_CONSTANT) {
            i += 2;
        } else {
            i += 1; 
        }
    }

    int selectedInstruction = 0; 

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_DOWN) && selectedInstruction < instructionCount - 1) {
            selectedInstruction++;
        }
        if (IsKeyPressed(KEY_UP) && selectedInstruction > 0) {
            selectedInstruction--;
        }

        BeginDrawing();
        ClearBackground(COLOR_BG);

        DrawText("Wizard VM Visualizer", 40, 25, 30, COLOR_TEXT);
        DrawText("Visualizacion del bytecode generado por el compilador", 40, 60, 18, COLOR_MUTED);

        Rectangle bytecodePanel = {40, 100, 560, 470};
        Rectangle constantsPanel = {630, 100, 330, 220};
        Rectangle resultPanel = {630, 300, 330, 100};
        Rectangle helpPanel = {630, 400, 330, 220};

        drawPanel(bytecodePanel, "BYTECODE");
        drawPanel(constantsPanel, "CONSTANTES");
        drawPanel(resultPanel, "RESULTADO");
        drawPanel(helpPanel, "AYUDA");

        int y = 170;

        for (int i = 0; i < instructionCount; i++) {
            int offset = instructionOffsets[i];
            uint8_t instruction = chunk->code[offset];

            Color rowColor = (i == selectedInstruction) ? COLOR_ACTIVE : COLOR_PANEL;
            Color textColor = COLOR_TEXT;

            DrawRectangleRounded(
                (Rectangle){60, y - 8, 520, 30},
                0.15f,
                8,
                rowColor
            );

            char line[256];

            if (instruction == OP_CONSTANT) {
                uint8_t constant = chunk->code[offset + 1];
                Valux value = chunk->constants.values[constant];

                char valueText[64];
                valueToString(value, valueText, sizeof(valueText));

                snprintf(line, sizeof(line),
                         "%04d   %-14s index=%d   value=%s",
                         offset,
                         instructionName(instruction),
                         constant,
                         valueText);

                DrawText(line, 75, y, 18, textColor);
            } else if (instruction == OP_RETURN) {
                snprintf(line, sizeof(line),
                         "%04d   %-14s",
                         offset,
                         instructionName(instruction));

                DrawText(line, 75, y, 18, COLOR_SUCCESS);
            } else if (instruction == 255) {
                snprintf(line, sizeof(line), "%04d   UNKNOWN", offset);
                DrawText(line, 75, y, 18, COLOR_DANGER);
            } else {
                snprintf(line, sizeof(line),
                         "%04d   %-14s",
                         offset,
                         instructionName(instruction));

                DrawText(line, 75, y, 18, textColor);
            }

            y += 38;
        }

        int cy = 170;
        for (int i = 0; i < chunk->constants.count; i++) {
            char valueText[64];
            valueToString(chunk->constants.values[i], valueText, sizeof(valueText));

            char line[128];
            snprintf(line, sizeof(line), "index %d  ->  %s", i, valueText);

            DrawRectangleRounded(
                (Rectangle){650, cy - 8, 290, 30},
                0.15f,
                8,
                (Color){50, 50, 62, 255}
            );

            DrawText(line, 665, cy, 18, COLOR_TEXT);
            cy += 38;
        }

        char resultadoText[64];
        valueToString(resultado, resultadoText, sizeof(resultadoText));

        DrawRectangleRounded(
            (Rectangle){650, 370, 290, 30}, 0.15f, 8, (Color){50, 50, 62, 255}
        );

        DrawText("Valor final:", 660, 345, 18, COLOR_MUTED);
        DrawText(resultadoText, 665, 378, 20, COLOR_SUCCESS);

        DrawText("UP / DOWN", 660, 485, 20, COLOR_ACTIVE);
        DrawText("Mover seleccion", 660, 510, 18, COLOR_TEXT);

        DrawText("ESC", 660, 540, 20, COLOR_ACTIVE);
        DrawText("Cerrar ventana", 660, 560, 18, COLOR_TEXT);

        EndDrawing();
    }

    CloseWindow();
}

