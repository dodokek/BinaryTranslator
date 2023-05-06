#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <chrono>

#include "file_utils.h"

// ===============================================
// Defines   ||
//           \/
#define DEBUG 1

#ifdef DEBUG
    #define LOG(...) fprintf (LOG_FILE, __VA_ARGS__);
#else
    #define LOG(...) ;
#endif


#define BYTE(X) 8*X

// ===============================================


// Constants ||
//           \/

const int MEMORY_ALIGNMENT = 4096;

const int MAX_IP = 1000;

const int MAX_LABELS = 100;

const int MAX_OPCODE_LEN = 6;


const int HEADER_OFFSET = 8;

const int PAGESIZE = 4096;


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

// Watch OPCODE_MASKS if you want to construct one of the following cmds.
// ATTENTION: all opcodes are written in reverse order.

    ARITHM_XMM0_XMM1 = 0xC1000FF2,  // <add, sub, mul, div> xmm0, xmm1
                        // ^------changing this byte to get add/sub/div/mul     

                //         ,------- xmm 0 - 4
    MOV_XMM_RSP = 0x002400100FF2, // movsd xmm0-4, [rsp + ?]
                //  ^--[rsp + ?]

                //      ,------- xmm 0 - 4
    MOV_RSP_XMM = 0x002400110FF2, // same as previous
                //  ^--[rsp + ?]
    
    ADD_RSP = 0x00C48348,   // add rsp+?
    //          ^-- how much to add

    // mov rdi, [r10 + ?]
    MOV_RDI_R10 = 0xBA8B49, // this must be followed with uint32 ptr

    // mov [r10 + ?], rdi
    MOV_R10_RDI = 0xBA8949, // this must be followed with uint32 ptr

    // transforms double precision num in xmm0 to integer in rsi
    CVTSD2SI_RSI_XMM0 = 0xF02D0F48F2,
    
    SHL_RSI = 0x00E6C148,   // rsi *= 2^(?)
    //          ^-- how much bites to shift

    PUSH_REG = 0x50, //    push/pop r?x
    POP_REG = 0x58,  //              ^--- add 0, 1, 2, 3 to get rax, rcx, rdx or rbx 

    COND_JMP = 0x000F,
            //   ^-- by applying bit mask, can get all types of jmp

// Constant expressions, no need for bit masks

    MOV_R10 = 0xBA49,   // mov r10, <64b ptr>. Begin of memory must be stored in R10
    CALL_OP = 0xE8,     // call <32b ptr>
    RET_OP = 0xC3,      // ret

    JMP_OP = 0xE9,      // jmp <32b ptr>

    PUSH_RSI = 0x56,    // push rsi
    PUSH_RDI = 0x57,    // push rdi

    POP_RSI = 0x5E,     // push rsi
    POP_RDI = 0x5F,     // pop rdi

    CMP_RDI_RSI = 0xf73948, // cmd rdi, rsi

    PUSH_ALL = 0x505152535241,  // push r10 - rax - ... - rdx
    POP_ALL = 0x5A415B5A5958,   // pop rdx - ... rax - r10

    MOV_RBP_RSP = 0xE48949,
    MOV_RSP_RBP = 0xE4894C,
    AND_RSP_FF = 0xF0E48348,

    MOV_RSI = 0xBE48, // mov rsi, (double num)
                      // next 8 bytes should be double number

    ADD_R10_RSI = 0xF20149,
    SUB_R10_RSI = 0xF22949,
    SQRTPD_XMM0_XMM0 = 0xC0510F66   // get square root from xmm0 and store it in xmm0
};


enum OPCODE_MASKS : uint64_t
{
    ADD_MASK = 0x58,    // Arithm operation with double
    SUB_MASK = 0x5c,
    MUL_MASK = 0x59,
    DIV_MASK = 0x5e,

    XMM0_MASK = 0x44,   // masks for work with xmm registers
    XMM1_MASK = 0x4c,

    JE_MASK = 0x84,     // Conditional jumps
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

    SIZE_PUSH_POP_All = 6,
    SIZE_AND_RSP = 4,

    SIZE_CMP_RSI_RDI = 3,
    SIZE_MOV_RSI     = 2,
    SIZE_MOV_R10 = 2,

    SIZE_PUSH_REG = 1,
    SIZE_POP_REG = 1,

    SIZE_ARITHM_XMM = 4,
    SIZE_MOV_XMM_RSP = 6,

    SIZE_ADD_RSP = 4,

    SIZE_MOV_REG_REG = 3,

    SIZE_JMP = 1,
    SIZE_COND_JMP = 2,
    SIZE_RET = 1,

    SIZE_CVTSD2SI = 5,
    SIZE_R10_RSI = 3,

    SIZE_SHL = 4,
    SIZE_SQRT = 4,
};


// ===============================================
// Enums     ||
//           \/

#define DEF_CMD(CMD, id) CMD = id, 

enum EnumCommands
{
    HLT = 0,

    #include "../codegen/cmds.h"

    NONE = -1,
};

#undef DEF_CMD


enum BitMasks   // For native assembly
{
    CMD_BITMASK  = 0b00011111,
    SPEC_BITMASK = 0b11100000,
    ARG_IMMED    = 0b00100000,
    ARG_REG      = 0b01000000,
    ARG_MEM      = 0b10000000,
};


enum OffsetsAndFlags
{
    BYTE_OFFSET = 1,
    NONE_S = 0,

    END_OF_PROG = -1,
};


enum ExitCodes
{
    SUCCESS,
    ALLOCATION_FAILURE,
};


enum PushPopVariations  // For IR of command. 
{                       
    VOID = 0,
    IMM = 1 << 5,
    REG = 1 << 6,
    RAM = 1 << 7,

    IMM_REG = (1 << 5) + (1 << 6),
    IMM_RAM = (1 << 5) + (1 << 7),
    REG_RAM = (1 << 6) + (1 << 7),
    IMM_REG_RAM = (1 << 5) + (1 << 6) + (1 << 7),
};


enum PushPopSizes
{
    POP_REG_SIZE = 1, 
    POP_IMM_RAM_SIZE = 8,
    POP_REG_RAM_SIZE = 34,

    PUSH_REG_SIZE = 1, 
    PUSH_IMM_SIZE = 11, 
    PUSH_IMM_RAM_SIZE = 8,
    PUSH_REG_RAM_SIZE = 34,
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

const struct InstructionSizes InstrSizes[] =
{
    {HLT,  1,   1},
    {PUSH, 10,  1}, 
    {MUL, 1, 26},
    {ADD, 1, 26},
    {SUB, 1, 26},
    {DIV, 1, 26},
    {POP, 10,  1},
    {OUT, 1, 34},
    {NONE, NONE_S, NONE_S}, // made for proper indexation
    {JMP, 10,  5},
    {JG,  10,  11},
    {JGE, 10,  11},
    {JA,  10,  11},
    {JAE, 10,  11},
    {JE,  10,  11},
    {JNE, 10,  11},
    {NONE, NONE_S, NONE_S},
    {CALL, 10,  5},
    {RET, 1,  1},
    {SQR, 1, 16}
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
    elem_t value;   // might be push/pop value, jmp label

    int checksum;   // used in case of push/pop handle
};


struct TranslatorInfo
{
    CmdsBuffer src_cmds; 
    CmdsBuffer dst_x86;

    Command** cmds_array; 
    int cmds_counter;

    int orig_ip_counter;
    int x86_ip_counter;

    char* memory_buffer; // Buffer for commands, addressing to memory  
};


// ===============================================

// ===============================================
// Functions ||
//           \/

void WriteAbsPtr (TranslatorInfo* self, uint64_t ptr);

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

void TranslateSqr (TranslatorInfo* self, Command* cur_cmd);

void RegToRsiOffset(TranslatorInfo* self, Command* cur_cmd);

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