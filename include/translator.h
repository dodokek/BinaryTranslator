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
// Enums     ||
//           \/

#define DEF_CMD(CMD, offset, id) CMD = id, 

enum EnumCommands
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


enum ExitCodes
{
    SUCCESS,
    ALLOCATION_FAILURE,
};

// ===============================================


// ===============================================
// Structs   ||
//           \/

typedef double elem_t;

struct CmdsBuffer
{
    const char* content;
    int len;
};


struct Command 
{
    EnumCommands name;
    
    int reg_index;
    elem_t value;

    bool is_immed;
    bool use_reg;
    bool use_mem;
};


struct TranslatorMain
{
    CmdsBuffer src_cmds;
    CmdsBuffer dst_x86;

    Command** cmds_array;
    int cmds_counter;

};




// ===============================================
// Constants ||
//           \/

const char* INPUT_FILE_PATH = "../data/cmds.bin";

const int HEADER_OFFSET = 8;

// ===============================================


// ===============================================
// Functions ||
//           \/

void DumpRawCmds (TranslatorMain* self);

void FillCmdInfo (const char* code, TranslatorMain* self);

void TranslatorCtor (TranslatorMain* self);

void InitializeTranslation (TranslatorMain* self);

void ReadFileToStruct (TranslatorMain* self, FILE* file);

int CmdToStruct (const char* code, TranslatorMain* self);

int AllocateCmdArrays (TranslatorMain* self);

// ===============================================

#endif