#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "cimgui_impl.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <stdio.h>

#include "chunk.h"
#include "memory.h"
#include "object.h"
#include "vm.h"
#include "wizard_ui.h"

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

static void placeWindow(float x, float y, float width, float height) {
  igSetNextWindowPos((ImVec2){x, y}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
  igSetNextWindowSize((ImVec2){width, height}, ImGuiCond_FirstUseEver);
}

static void drawBytecode(void) {
  placeWindow(10, 100, 580, 430);
  igBegin("Bytecode / Chunk", NULL, 0);
  if (!vmIsRunning()) { igText("No active call frame."); igEnd(); return; }
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  Chunk* chunk = &frame->closure->function->chunk;
  int current = (int)(frame->ip - chunk->code);
  igText("Function: %s   IP: %04d   %d bytes", frame->closure->function->name ? frame->closure->function->name->chars : "<script>", current, chunk->count);
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
  igEnd();
}

static void drawStack(void) {
  placeWindow(600, 100, 400, 430);
  igBegin("VM Stack", NULL, 0);
  int count = (int)(vm.stackTop - vm.stack);
  igText("%d / %d slots", count, STACK_MAX);
  igSeparator();
  for (int i = count - 1; i >= 0; i--) {
    char text[128]; valueToText(vm.stack[i], text, sizeof(text));
    igText("[%03d] %s", i, text);
  }
  igEnd();
}

static void drawTable(const char* title, Table* table) {
  if (table == &vm.globals) placeWindow(10, 540, 700, 340);
  else placeWindow(720, 540, 710, 340);
  igBegin(title, NULL, 0);
  igText("entries: %d   capacity: %d", table->count, table->capacity);
  igSeparator();
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;
    char value[128]; valueToText(entry->value, value, sizeof(value));
    igText("[%03d] %s = %s", i, entry->key->chars, value);
  }
  igEnd();
}

static void drawHeap(void) {
  placeWindow(1010, 320, 420, 210);
  igBegin("Heap / Garbage Collector", NULL, 0);
  igText("allocated: %zu bytes   next collection: %zu", vm.bytesAllocated, vm.nextGC);
  if (igButton("Collect now", (ImVec2){0, 0})) collectGarbage();
  igSeparator();
  int n = 0;
  for (Obj* object = vm.objects; object != NULL; object = object->next) {
    igText("%c  @%p  %s", object->isMarked ? '*' : ' ', (void*)object, objectType(object->type));
    n++;
  }
  igSeparator(); igText("%d linked objects (* = marked during active GC)", n);
  igEnd();
}

static void drawUpvaluesAndFrames(void) {
  placeWindow(1010, 100, 420, 210);
  igBegin("Call Frames / Upvalues", NULL, 0);
  for (int i = 0; i < vm.frameCount; i++) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* fn = frame->closure->function;
    igText("frame %d  %s  ip=%td  slots=%td", i, fn->name ? fn->name->chars : "<script>",
      frame->ip - fn->chunk.code, frame->slots - vm.stack);
  }
  igSeparator();
  if (vm.openUpvalues == NULL) igText("Open upvalues: none");
  for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
    char value[96]; valueToText(*upvalue->location, value, sizeof(value));
    igText("upvalue @%p -> stack[%td] = %s", (void*)upvalue, upvalue->location - vm.stack, value);
  }
  igEnd();
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

  InterpretResult result = beginInterpret(source);
  bool autoplay = false;
  int stepsPerFrame = 1;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();

    placeWindow(10, 10, 1420, 80);
    igBegin("Wizard Control Console", NULL, ImGuiWindowFlags_NoCollapse);
    igText("Status: %s", result == INTERPRET_RUNNING ? (autoplay ? "running" : "paused") : (result == INTERPRET_OK ? "halted" : "error"));
    if (igButton("Step instruction", (ImVec2){0, 0}) && result == INTERPRET_RUNNING) result = stepVM();
    igSameLine(0, -1);
    if (igButton(autoplay ? "Pause" : "Run", (ImVec2){0, 0}) && result == INTERPRET_RUNNING) autoplay = !autoplay;
    igSameLine(0, -1);
    if (igButton("Restart", (ImVec2){0, 0})) { result = beginInterpret(source); autoplay = false; }
    igSameLine(0, -1); igSliderInt("steps/frame", &stepsPerFrame, 1, 100, "%d", 0);
    igText("Single-pass clox observer | UI pacing controls VM execution.");
    igEnd();

    if (autoplay && result == INTERPRET_RUNNING) {
      for (int i = 0; i < stepsPerFrame && result == INTERPRET_RUNNING; i++) result = stepVM();
    }
    drawBytecode(); drawStack(); drawTable("Globals Hash Table", &vm.globals);
    drawTable("String Interning Table", &vm.strings); drawHeap(); drawUpvaluesAndFrames();

    igRender();
    int width, height; glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.72f, 0.72f, 0.69f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
    glfwSwapBuffers(window);
  }
  ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); igDestroyContext(NULL);
  glfwDestroyWindow(window); glfwTerminate();
  return (result == INTERPRET_COMPILE_ERROR || result == INTERPRET_RUNTIME_ERROR) ? 1 : 0;
}
