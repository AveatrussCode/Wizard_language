#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "memory.h"
#include "object.h"
#include "vm.h"
#include "wizard_ui.h"

#define SOURCE_EDITOR_CAPACITY (64 * 1024)
#define UI_LOG_CAPACITY (16 * 1024)

typedef enum {
  UI_EDITING,
  UI_COMPILED,
  UI_RUNNING,
  UI_PAUSED,
  UI_HALTED,
  UI_ERROR
} UIState;

typedef struct {
  char text[SOURCE_EDITOR_CAPACITY];
  bool modified;
} SourceEditor;

typedef struct {
  char output[UI_LOG_CAPACITY];
  char errors[UI_LOG_CAPACITY];
} UILog;

typedef struct {
  bool valid;
  uint8_t opcode;
  char operand[128];
  char firstStackValue[128];
  char secondStackValue[128];
  char result[128];
} InstructionSnapshot;

static UILog* activeLog = NULL;

static void appendLog(char* destination, const char* text) {
  size_t used = strlen(destination);
  if (used >= UI_LOG_CAPACITY - 1) return;
  strncat(destination, text, UI_LOG_CAPACITY - used - 1);
}

static void captureOutput(const char* text) {
  if (activeLog != NULL) appendLog(activeLog->output, text);
}

static void captureError(const char* text) {
  if (activeLog != NULL) appendLog(activeLog->errors, text);
}

static const char* uiStateName(UIState state) {
  switch (state) {
    case UI_EDITING: return "Not Compiled";
    case UI_COMPILED: return "Compiled";
    case UI_RUNNING: return "Running";
    case UI_PAUSED: return "Paused";
    case UI_HALTED: return "Halted";
    case UI_ERROR: return "Error";
  }
  return "Unknown";
}

static UIState stateFromResult(InterpretResult result) {
  switch (result) {
    case INTERPRET_OK: return UI_HALTED;
    case INTERPRET_COMPILE_ERROR:
    case INTERPRET_RUNTIME_ERROR: return UI_ERROR;
    case INTERPRET_RUNNING: return UI_PAUSED;
  }
  return UI_ERROR;
}

static const char* opcodeName(uint8_t code) {
  static const char* names[] = {
    "OP_CONSTANT", "OP_NIL", "OP_TRUE", "OP_FALSE", "OP_POP", "OP_GET_LOCAL", "OP_SET_LOCAL",
    "OP_GET_GLOBAL", "OP_DEFINE_GLOBAL", "OP_SET_GLOBAL", "OP_GET_UPVALUE", "OP_SET_UPVALUE",
    "OP_GET_PROPERTY", "OP_SET_PROPERTY", "OP_GET_SUPER", "OP_EQUAL", "OP_GREATER", "OP_LESS",
    "OP_ADD", "OP_SUBTRACT", "OP_MULTIPLY", "OP_DIVIDE", "OP_NOT", "OP_NEGATE", "OP_PRINT",
    "OP_JUMP", "OP_JUMP_IF_FALSE", "OP_LOOP", "OP_CALL", "OP_INVOKE", "OP_SUPER_INVOKE",
    "OP_CLOSURE", "OP_CLOSE_UPVALUE", "OP_RETURN", "OP_CLASS", "OP_INHERIT", "OP_METHOD"
  };
  return code <= OP_METHOD ? names[code] : "<invalid opcode>";
}

static int instructionSize(Chunk* chunk, int offset) {
  switch (chunk->code[offset]) {
    case OP_CONSTANT: case OP_GET_LOCAL: case OP_SET_LOCAL: case OP_GET_GLOBAL:
    case OP_DEFINE_GLOBAL: case OP_SET_GLOBAL: case OP_GET_UPVALUE: case OP_SET_UPVALUE:
    case OP_GET_PROPERTY: case OP_SET_PROPERTY: case OP_GET_SUPER: case OP_CALL:
    case OP_CLASS: case OP_METHOD: return 2;
    case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_LOOP: case OP_INVOKE: case OP_SUPER_INVOKE: return 3;
    case OP_CLOSURE: {
      Valux constant = chunk->constants.values[chunk->code[offset + 1]];
      return IS_FUNCTION(constant) ? 2 + AS_FUNCTION(constant)->upvalueCount * 2 : 2;
    }
    default: return 1;
  }
}

static void valueToText(Valux value, char* out, size_t size) {
  if (IS_NIL(value)) snprintf(out, size, "nil");
  else if (IS_BOOL(value)) snprintf(out, size, AS_BOOL(value) ? "true" : "false");
  else if (IS_NUMBER(value)) snprintf(out, size, "%g", AS_NUMBER(value));
  else if (IS_STRING(value)) snprintf(out, size, "string \"%.36s\"", AS_CSTRING(value));
  else if (IS_FUNCTION(value)) snprintf(out, size, "function %s", AS_FUNCTION(value)->name ? AS_FUNCTION(value)->name->chars : "<script>");
  else if (IS_CLOSURE(value)) snprintf(out, size, "closure %s", AS_CLOSURE(value)->function->name ? AS_CLOSURE(value)->function->name->chars : "<script>");
  else if (IS_CLASS(value)) snprintf(out, size, "class %s", AS_CLASS(value)->name->chars);
  else if (IS_INSTANCE(value)) snprintf(out, size, "instance of %s", AS_INSTANCE(value)->klass->name->chars);
  else if (IS_NATIVE(value)) snprintf(out, size, "native function");
  else snprintf(out, size, "object @%p", (void*)AS_OBJ(value));
}

static const char* objectType(ObjType type) {
  static const char* names[] = { "bound method", "class", "closure", "function", "instance", "native", "string", "upvalue" };
  return names[type];
}

static void snapshotInstructionBefore(InstructionSnapshot* snapshot) {
  memset(snapshot, 0, sizeof(*snapshot));
  if (!vmIsRunning()) return;

  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  Chunk* chunk = &frame->closure->function->chunk;
  int offset = (int)(frame->ip - chunk->code);
  if (offset < 0 || offset >= chunk->count) return;

  snapshot->valid = true;
  snapshot->opcode = chunk->code[offset];
  int size = instructionSize(chunk, offset);
  if (size == 2) {
    uint8_t operand = chunk->code[offset + 1];
    switch (snapshot->opcode) {
      case OP_CONSTANT: case OP_GET_GLOBAL: case OP_DEFINE_GLOBAL:
      case OP_SET_GLOBAL: case OP_GET_PROPERTY: case OP_SET_PROPERTY:
      case OP_GET_SUPER: case OP_CLASS: case OP_METHOD:
        valueToText(chunk->constants.values[operand], snapshot->operand,
                    sizeof(snapshot->operand));
        break;
      default:
        snprintf(snapshot->operand, sizeof(snapshot->operand), "%u", operand);
        break;
    }
  }

  int stackCount = (int)(vm.stackTop - vm.stack);
  if (stackCount > 0) valueToText(vm.stack[stackCount - 1], snapshot->secondStackValue,
                                  sizeof(snapshot->secondStackValue));
  if (stackCount > 1) valueToText(vm.stack[stackCount - 2], snapshot->firstStackValue,
                                  sizeof(snapshot->firstStackValue));
}

static void snapshotInstructionAfter(InstructionSnapshot* snapshot) {
  if (!snapshot->valid) return;
  int stackCount = (int)(vm.stackTop - vm.stack);
  if (stackCount > 0) valueToText(vm.stack[stackCount - 1], snapshot->result,
                                  sizeof(snapshot->result));
}

static InterpretResult stepWithSnapshot(InstructionSnapshot* snapshot) {
  snapshotInstructionBefore(snapshot);
  InterpretResult result = stepVM();
  snapshotInstructionAfter(snapshot);
  return result;
}

static void drawBytecode(UIState state) {
  if (state == UI_EDITING || !vmHasPreparedProgram()) {
    igText("Compile the source to generate bytecode.");
    return;
  }

  ObjFunction* function = vm.preparedFunction;
  int current = -1;
  if (vmIsRunning()) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    function = frame->closure->function;
    current = (int)(frame->ip - function->chunk.code);
  }
  Chunk* chunk = &function->chunk;
  if (current >= 0) {
    igText("Function: %s   IP: %04d   %d bytes", function->name ? function->name->chars : "<script>", current, chunk->count);
  } else {
    igText("Function: %s   execution complete   %d bytes", function->name ? function->name->chars : "<script>", chunk->count);
  }
  igSeparator();
  for (int offset = 0; offset < chunk->count; offset += instructionSize(chunk, offset)) {
    bool active = offset == current;
    if (active) igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.10f, 0.25f, 0.60f, 1.0f});
    char operand[72] = "";
    uint8_t op = chunk->code[offset];
    if (instructionSize(chunk, offset) == 2) snprintf(operand, sizeof(operand), " %u", chunk->code[offset + 1]);
    else if (instructionSize(chunk, offset) == 3) snprintf(operand, sizeof(operand), " %u %u", chunk->code[offset + 1], chunk->code[offset + 2]);
    igText("%s %04d  line %-4d %-20s%s", active ? ">" : " ", offset, chunk->lines[offset], opcodeName(op), operand);
    if (active) igPopStyleColor(1);
  }
}

static void drawInstructionInspector(const InstructionSnapshot* snapshot) {
  if (!snapshot->valid) {
    igText("Step the VM to inspect an instruction.");
    return;
  }

  igText("Last executed instruction");
  igSeparator();
  igText("%s", opcodeName(snapshot->opcode));
  switch (snapshot->opcode) {
    case OP_CONSTANT:
      igText("Operand: %s", snapshot->operand);
      igText("Effect: pushes the constant onto the VM stack.");
      break;
    case OP_ADD: case OP_SUBTRACT: case OP_MULTIPLY: case OP_DIVIDE: {
      const char* symbol = snapshot->opcode == OP_ADD ? "+" :
                           snapshot->opcode == OP_SUBTRACT ? "-" :
                           snapshot->opcode == OP_MULTIPLY ? "*" : "/";
      igText("Stack effect:");
      igText("%s %s %s", snapshot->firstStackValue, symbol, snapshot->secondStackValue);
      igText("Result: %s", snapshot->result);
      break;
    }
    case OP_NEGATE:
      igText("Operand: %s", snapshot->secondStackValue);
      igText("Result: %s", snapshot->result);
      break;
    case OP_PRINT:
      igText("Value: %s", snapshot->secondStackValue);
      igText("Effect: removes the value from the stack and writes it to output.");
      break;
    case OP_GET_GLOBAL:
      igText("Variable: %s", snapshot->operand);
      igText("Value pushed: %s", snapshot->result);
      break;
    case OP_DEFINE_GLOBAL:
      igText("Variable: %s", snapshot->operand);
      igText("Value stored: %s", snapshot->secondStackValue);
      igText("Effect: defines the global and removes the value from the stack.");
      break;
    case OP_SET_GLOBAL:
      igText("Variable: %s", snapshot->operand);
      igText("Value: %s", snapshot->secondStackValue);
      break;
    default:
      if (snapshot->operand[0] != '\0') igText("Operand: %s", snapshot->operand);
      igText("This instruction's detailed explanation is not available yet.");
      break;
  }
}

static void drawStack(void) {
  int count = (int)(vm.stackTop - vm.stack);
  igText("%d / %d slots", count, STACK_MAX);
  igSeparator();
  for (int i = count - 1; i >= 0; i--) {
    char text[128]; valueToText(vm.stack[i], text, sizeof(text));
    igText("[%03d] %s", i, text);
  }
}

static void drawTable(Table* table) {
  igText("entries: %d   capacity: %d", table->count, table->capacity);
  igSeparator();
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;
    char value[128]; valueToText(entry->value, value, sizeof(value));
    igText("[%03d] %s = %s", i, entry->key->chars, value);
  }
}

static void drawHeap(void) {
  igText("allocated: %zu bytes   next collection: %zu", vm.bytesAllocated, vm.nextGC);
  if (igButton("Collect now", (ImVec2){0, 0})) collectGarbage();
  igSeparator();
  int n = 0;
  for (Obj* object = vm.objects; object != NULL; object = object->next) {
    igText("%c  @%p  %s", object->isMarked ? '*' : ' ', (void*)object, objectType(object->type));
    n++;
  }
  igSeparator(); igText("%d linked objects (* = marked during active GC)", n);
}

static void drawFrames(void) {
  for (int i = 0; i < vm.frameCount; i++) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* fn = frame->closure->function;
    igText("frame %d  %s  ip=%td  slots=%td", i, fn->name ? fn->name->chars : "<script>",
      frame->ip - fn->chunk.code, frame->slots - vm.stack);
  }
  if (vm.frameCount == 0) igText("No active call frames.");
}

static void drawUpvalues(void) {
  if (vm.openUpvalues == NULL) igText("Open upvalues: none");
  for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
    char value[96]; valueToText(*upvalue->location, value, sizeof(value));
    igText("upvalue @%p -> stack[%td] = %s", (void*)upvalue, upvalue->location - vm.stack, value);
  }
}

static void applyStyle(void) {
  ImGuiStyle* style = igGetStyle();
  igStyleColorsLight(style);
  style->WindowRounding = 0.0f;
  style->FrameRounding = 0.0f;
  style->WindowBorderSize = 2.0f;
  style->FrameBorderSize = 1.0f;
  style->WindowPadding = (ImVec2){10.0f, 8.0f};
  style->FramePadding = (ImVec2){6.0f, 4.0f};
  style->Colors[ImGuiCol_WindowBg] = (ImVec4){0.88f, 0.86f, 0.80f, 1.0f};
  style->Colors[ImGuiCol_TitleBg] = (ImVec4){0.23f, 0.27f, 0.31f, 1.0f};
  style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.16f, 0.31f, 0.52f, 1.0f};
  style->Colors[ImGuiCol_Border] = (ImVec4){0.15f, 0.15f, 0.15f, 1.0f};
  style->Colors[ImGuiCol_Button] = (ImVec4){0.65f, 0.67f, 0.65f, 1.0f};
  style->Colors[ImGuiCol_ButtonHovered] = (ImVec4){0.48f, 0.62f, 0.78f, 1.0f};
}

int wizardUIRun(const char* source, const char* title) {
  if (!glfwInit()) return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  GLFWwindow* window = glfwCreateWindow(1440, 900, title, NULL, NULL);
  if (window == NULL) { glfwTerminate(); return 1; }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  igCreateContext(NULL);
  applyStyle();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  SourceEditor editor = {{0}, false};
  UILog log = {{0}, {0}};
  InstructionSnapshot instruction = {0};
  if (source != NULL) {
    strncpy(editor.text, source, sizeof(editor.text) - 1);
    editor.text[sizeof(editor.text) - 1] = '\0';
  }

  /* Opening the workbench never compiles or runs the initial text. */
  InterpretResult result = INTERPRET_OK;
  UIState state = UI_EDITING;
  bool autoplay = false;
  int stepsPerFrame = 1;
  activeLog = &log;
  vmSetOutputCallback(captureOutput);
  vmSetErrorCallback(captureError);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    igSetNextWindowPos((ImVec2){0, 0}, ImGuiCond_Always, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){(float)windowWidth, (float)windowHeight}, ImGuiCond_Always);
    igBegin("WIZARD VM VISUALIZER", NULL,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    igText("WIZARD VM VISUALIZER");
    igSameLine(0, 20);
    if (igButton("Compile", (ImVec2){0, 0})) {
      autoplay = false;
      log.output[0] = '\0';
      log.errors[0] = '\0';
      result = vmCompileSource(editor.text);
      if (result == INTERPRET_OK) result = vmRestartPrepared();
      state = result == INTERPRET_RUNNING ? UI_COMPILED : stateFromResult(result);
      instruction.valid = false;
      editor.modified = false;
    }
    igSameLine(0, -1);
    if (state == UI_COMPILED || state == UI_PAUSED) {
      if (igButton("Step", (ImVec2){0, 0}) && result == INTERPRET_RUNNING) {
        result = stepWithSnapshot(&instruction);
        state = stateFromResult(result);
      }
      igSameLine(0, -1);
      if (igButton("Run", (ImVec2){0, 0}) && result == INTERPRET_RUNNING) {
        autoplay = true;
        state = UI_RUNNING;
      }
      igSameLine(0, -1);
    } else if (state == UI_RUNNING) {
      if (igButton("Pause", (ImVec2){0, 0})) {
        autoplay = false;
        state = UI_PAUSED;
      }
      igSameLine(0, -1);
    }
    if ((state == UI_COMPILED || state == UI_PAUSED || state == UI_RUNNING || state == UI_HALTED) &&
        igButton("Restart", (ImVec2){0, 0})) {
      /* Restart reuses the already retained bytecode. */
      autoplay = false;
      result = vmRestartPrepared();
      state = result == INTERPRET_RUNNING ? UI_COMPILED : stateFromResult(result);
      instruction.valid = false;
    }
    igSameLine(0, -1);
    igText("Status: %s", uiStateName(state));
    if (state == UI_RUNNING) {
      igSameLine(0, -1);
      igSliderInt("Speed", &stepsPerFrame, 1, 100, "%d", 0);
    }

    if (autoplay && result == INTERPRET_RUNNING) {
      for (int i = 0; i < stepsPerFrame && result == INTERPRET_RUNNING; i++) {
        result = stepWithSnapshot(&instruction);
      }
      if (result != INTERPRET_RUNNING) {
        autoplay = false;
        state = stateFromResult(result);
      }
    }

    igSeparator();
    ImVec2 available = igGetContentRegionAvail();
    float gap = 8.0f;
    float columnWidth = (available.x - gap) * 0.5f;
    float topHeight = available.y * 0.42f;
    if (igBeginChild_Str("SourcePanel", (ImVec2){columnWidth, topHeight}, ImGuiChildFlags_Borders, 0)) {
      igText("SOURCE CODE");
      igSeparator();
      igText("Edit the Lox program, then press Compile.");
      ImVec2 editorSize = igGetContentRegionAvail();
      if (editor.modified) editorSize.y -= 22.0f;
      if (igInputTextMultiline("##source", editor.text, sizeof(editor.text), editorSize, 0, NULL, NULL)) {
        editor.modified = true;
        autoplay = false;
        if (state != UI_EDITING) state = UI_EDITING;
      }
      if (editor.modified) igText("Code modified. Compile to update VM.");
    }
    igEndChild();
    igSameLine(0, gap);
    if (igBeginChild_Str("BytecodePanel", (ImVec2){columnWidth, topHeight}, ImGuiChildFlags_Borders, 0)) {
      igText("BYTECODE");
      igSeparator();
      drawBytecode(state);
    }
    igEndChild();

    available = igGetContentRegionAvail();
    float middleHeight = available.y * 0.42f;
    if (igBeginChild_Str("StackPanel", (ImVec2){columnWidth, middleHeight}, ImGuiChildFlags_Borders, 0)) {
      igText("VM STACK");
      igSeparator();
      drawStack();
    }
    igEndChild();
    igSameLine(0, gap);
    if (igBeginChild_Str("InspectorPanel", (ImVec2){columnWidth, middleHeight}, ImGuiChildFlags_Borders, 0)) {
      igText("INSTRUCTION INSPECTOR");
      igSeparator();
      drawInstructionInspector(&instruction);
    }
    igEndChild();

    available = igGetContentRegionAvail();
    float outputHeight = available.y * 0.40f;
    if (igBeginChild_Str("OutputPanel", (ImVec2){0, outputHeight}, ImGuiChildFlags_Borders, 0)) {
      igText("OUTPUT / ERRORS");
      igSeparator();
      if (log.output[0] == '\0' && log.errors[0] == '\0') {
        igText("Program output and compiler/runtime errors will appear here.");
      } else {
        if (log.output[0] != '\0') {
          igText("OUTPUT");
          igTextUnformatted(log.output, NULL);
        }
        if (log.errors[0] != '\0') {
          if (log.output[0] != '\0') igSeparator();
          igText("ERROR");
          igTextUnformatted(log.errors, NULL);
        }
      }
    }
    igEndChild();

    if (igBeginChild_Str("AdvancedPanel", (ImVec2){0, 0}, ImGuiChildFlags_Borders, 0)) {
      igText("ADVANCED PANELS");
      if (igBeginTabBar("AdvancedTabs", 0)) {
        if (igBeginTabItem("Frames", NULL, 0)) { drawFrames(); igEndTabItem(); }
        if (igBeginTabItem("Upvalues", NULL, 0)) { drawUpvalues(); igEndTabItem(); }
        if (igBeginTabItem("Globals", NULL, 0)) { drawTable(&vm.globals); igEndTabItem(); }
        if (igBeginTabItem("Strings", NULL, 0)) { drawTable(&vm.strings); igEndTabItem(); }
        if (igBeginTabItem("Heap / GC", NULL, 0)) { drawHeap(); igEndTabItem(); }
        igEndTabBar();
      }
    }
    igEndChild();

    igEnd();

    igRender();
    int width, height; glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.72f, 0.72f, 0.69f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    glfwSwapBuffers(window);
  }
  vmSetOutputCallback(NULL);
  vmSetErrorCallback(NULL);
  activeLog = NULL;
  ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); igDestroyContext(NULL);
  glfwDestroyWindow(window); glfwTerminate();
  return (result == INTERPRET_COMPILE_ERROR || result == INTERPRET_RUNTIME_ERROR) ? 1 : 0;
}
