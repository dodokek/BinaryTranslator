#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#include "file_utils.h"

// ===============================================
// Defines   ||
//           \/

#define LOG(...) fprintf (LOG_FILE, __VA_ARGS__);
// #define LOG(...) printf (__VA_ARGS__);

#define BYTE(X) 8*X

// ===============================================


// Constants ||
//           \/

const int MEMORY_ALIGNMENT = 4096;

const int MAX_IP = 1000;

const int MAX_LABELS = 100;

const int MAX_OPCODE_LEN = 6;

const char* INPUT_FILE_PATH = "../../Processor/data/cmds.bin";

const int HEADER_OFFSET = 8;

const int PAGESIZE = 4096;

const char* NotFound = "Not found\n";

const int MEMORY_SIZE = 4096;

const uint64_t NIL = 0x0;

const uint64_t DWORD_SIZE = 16;

const uint64_t WORD_SIZE = 8;

// ===============================================


// ===============================================
// Commands     ||
//              \/


enum OPCODES_x86 : uint64_t // everything reversed
{
    CALL_OP = 0xE8,
    RET_OP = 0xC3,

    JMP_OP = 0xE9,
    COND_JMP = 0x000F,

    PUSH_RSI = 0x56,
    PUSH_RDI = 0x57,

    POP_RSI = 0x5E,
    POP_RDI = 0x5F,

    CMP_RDI_RSI = 0xf73948,

    PUSH_ALL = 0x50515253,
    POP_ALL = 0x5B5A5958,

    MOV_RBP_RSP = 0xE48949,
    MOV_RSP_RBP = 0xE4894C,
    AND_RSP_FF = 0xF0E48348,

    MOV_RSI = 0xBE48, // mov rsi, (double num)
                  // next 8 bytes should be double number wrote as another cmd 

    PUSH_REG = 0x50, // by adding the index of register, getting different variations
    POP_REG = 0x58,
    //  0, 1, 2, 3 depends on register

    ARITHM_XMM0_XMM1 = 0xC1000FF2,  // add(sub, mul, div) xmm0, xmm1
                        // ^------changing this byte to get right operation    

                //         ,------- xmm 0 - 4
    MOV_XMM_RSP = 0x002400100FF2, // movsd xmm0-4, [rsp + ?]
                //  ^--[rsp + ?]

                
    MOV_RSP_XMM = 0x002400110FF2, // same as previous

    ADD_RSP = 0x00C48348,
    //          ^-- how much add

    // mov rdi, [r10 + ?]
    MOV_RDI_R10 = 0xBA8B49, // this must be followed with uint32 ptr

    // mov [r10 + ?], rdi
    MOV_R10_RDI = 0xBA8949, // this must be followed with uint32 ptr



};


enum OPCODE_MASKS : uint64_t
{
    ADD_MASK = 0x58,
    SUB_MASK = 0x5c,
    MUL_MASK = 0x59,
    DIV_MASK = 0x5e,

    XMM0_MASK = 0x44,   // special for xmm, [rsp + n]
    XMM1_MASK = 0x4c,

    JE_MASK = 0x84,
    JNE_MASK = 0x85,
    JG_MASK = 0x8c,
    JAE_MASK = 0x8d,
    JGE_MASK = 0x8e,
    JA_MASK = 0x8f,

};


enum OPCODE_SIZES
{
    SIZE_PUSH_RSI = 1,
    SIZE_PUSH_RDI = 1,
    SIZE_POP_RSI = 1,
    SIZE_POP_RDI = 1,

    SIZE_PUSH_POP_All = 4,
    SIZE_AND_RSP = 4,

    SIZE_CMP_RSI_RDI = 3,
    SIZE_MOV_RSI     = 2,

    SIZE_PUSH_REG = 1,
    SIZE_POP_REG = 1,

    SIZE_ARITHM_XMM = 4,
    SIZE_MOV_XMM_RSP = 6,

    SIZE_ADD_RSP = 4,

    SIZE_MOV_REG_REG = 3,

    SIZE_JMP = 1,
    SIZE_COND_JMP = 2,
    SIZE_RET = 1,
};


// ===============================================
// Enums     ||
//           \/

#define DEF_CMD(CMD, id) CMD = id, 

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


enum PushPopVariations
{
    VOID = 0,
    IMM = 1,
    REG = 3,
    RAM = 5,

    IMM_REG = 4,
    IMM_RAM = 6,
    REG_RAM = 8,
    IMM_REG_RAM = 9,
};


enum PushPopSizes
{
    POP_REG_SIZE = 1, 
    POP_IMM_RAM_SIZE = 8,
    POP_REG_RAM_SIZE = 4,
    POP_IMM_REG_RAM_SIZE = 14, 

    PUSH_REG_SIZE = 1, 
    PUSH_IMM_SIZE = 11, 
    PUSH_IMM_RAM_SIZE = 8,
    PUSH_REG_RAM_SIZE = 4,
    PUSH_IMM_REG_RAM_SIZE = 14, 
};


enum Registers
{
    RAX = 0,
    RBX,
    RCX,
    RDX,
};

// ===============================================


// ===============================================
// Structs   ||
//           \/

typedef double elem_t;

struct Opcode
{
    uint64_t code;
    size_t size;
};


struct InstructionSizes
{
    EnumCommands id;
    
    int original_size;
    int x86_size;
};

const struct InstructionSizes InstrSizes[30] =
{
    {HLT,  1,   1},
    {PUSH, 10,  1}, 
    {MUL, 1, 26},
    {ADD, 1, 26},
    {SUB, 1, 26},
    {DIV, 1, 26},
    {POP, 10,  1},
    {OUT, 1, 29},
    {},
    {JMP, 10,  5},
    {JG,  10,  11},
    {JGE, 10,  11},
    {JA,  10,  11},
    {JAE, 10,  11},
    {JE,  10,  11},
    {JNE, 10,  11},
    {},
    {CALL, 10,  5},
    {RET, 1,  1},
};


struct CmdsBuffer
{
    char* content;
    size_t len;
};


// IR
struct Command 
{
    EnumCommands name;
    int orig_ip;
    int x86_ip;
    
    int reg_index;
    elem_t value;

    bool is_immed;
    bool use_reg;
    bool use_mem;
};

// static_assert( sizeof(Command ) == 32 );


struct TranslatorInfo
{
    CmdsBuffer src_cmds;
    CmdsBuffer dst_x86;

    Command** cmds_array;
    int cmds_counter;

    int orig_ip_counter;
    int x86_ip_counter;

    char* memory_buffer;
};


// ===============================================

// ===============================================
// Functions ||
//           \/

void WritePtr (TranslatorInfo* self, uint32_t ptr);

void WriteDoubleNum (TranslatorInfo* self, double value);

void WriteCmd (TranslatorInfo* self, Opcode cmd);

void FillJumpLables (TranslatorInfo* self);

int FindJumpIp (TranslatorInfo* self, int orig_ip);

char* GetNameFromId (EnumCommands id);

void DumpRawCmds (TranslatorInfo* self);

void Dump86Buffer (TranslatorInfo* self);

int FillCmdInfo (const char* code, TranslatorInfo* self);

void TranslatorCtor (TranslatorInfo* self);

void ParseOnStructs (TranslatorInfo* self);

void ReadFileToStruct (TranslatorInfo* self, FILE* file);

int CmdToStruct (const char* code, TranslatorInfo* self);

int AllocateCmdArrays (TranslatorInfo* self);

void RunCode (TranslatorInfo* self);

void StartTranslation (TranslatorInfo* self);

void FillPushPopStruct (TranslatorInfo* self, Command* new_cmd,
                        const char* code, int cmd, int name);

bool IsJump (int cmd);

//-- Translation Units:

void TranslatePushImmRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopImmRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushImmRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopImmRam (TranslatorInfo* self, Command* cur_cmd);

void TranslateOut (TranslatorInfo* self, Command* cur_cmd);

void TranslateBaseMath (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushImm (TranslatorInfo* self, Command* cur_cmd);

int CalcVariationSum (Command* cur_cmd);

void LoadToX86Buffer (TranslatorInfo* self, char* op_code, size_t len);

void HandlePushPopVariation (TranslatorInfo* self, Command* cur_cmd);

void TranslatePop (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopReg (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushReg (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslateJmpCall (TranslatorInfo* self, Command* cur_cmd);

void TranslateRet (TranslatorInfo* self, Command* jmp_cmd);

void TranslateConditionJmp (TranslatorInfo* self, Command* jmp_cmd);

void DumpCurBuffer (char* cur_buff, size_t len);

void EndLog();

// ===============================================

#endif