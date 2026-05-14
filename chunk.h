#ifndef WIZARD_CHUNK
#define WIZARD_CHUNK

#include "common.h"

typedef enum{
    OP_RETURN;
}OpCode;
typedef struct{
    int count;
    int capacity;
    uint8_t* code;
}
void initChunk(Chunk* chunk);
#endif