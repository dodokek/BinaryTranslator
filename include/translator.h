#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "file_utils.h"

// ===============================================
// Defines   ||
//           \/

#define LOG(...) fprintf (stderr, __VA_ARGS__);

// ===============================================

// ===============================================
// Structs   ||
//           \/


struct CmdFile
{
    const char* content;
    int len;
};


struct TranslatorMain
{
    CmdFile src_cmds;
    CmdFile dst_x86;
};


struct Command 
{
    /* data */
};


// ===============================================
// Constants ||
//           \/

const char* INPUT_FILE_PATH = "../data/cmds.bin";

const int HEADER_OFFSET = 8;

// ===============================================

// ===============================================
// Enums     ||
//           \/

#define DEF_CMD(CMD, offset, id) CMD = id, 

enum Commands
{
    HLT = 0,

    #include "../codegen/cmds.h"
};

#undef DEF_CMD


enum BitMasks
{
    CMD_BITMASK  = 0b00011111,
    SPEC_BITMASK = 0b11100000,
    ARG_IMMED    = 0b00100000,
    ARG_REG      = 0b01000000,
    ARG_MEM      = 0b10000000,
};


enum OffsetsAndFlags
{
    ZERO_OFFSET = 0,
    MULTI_BYTE_OFFSET = 9,
    BYTE_OFFSET = 1,
    JMP_OFFSET = 6,

    END_OF_PROG = -1,
};

// ===============================================

// ===============================================
// Functions ||
//           \/

void InitializeTranslation (TranslatorMain* self);

void ReadFileToStruct (TranslatorMain* self, FILE* file);

int CmdToStruct (const char* cur_ptr);

// ===============================================

#endif