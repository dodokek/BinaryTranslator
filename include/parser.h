#ifndef PARSER_H
#define PARSER_H

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <chrono>
#include <stdarg.h>

#include "file_utils.h"

// ===============================================
// Defines   ||
//           \/

#define DEBUG 1

#ifdef DEBUG
    #define LOG(...) LogData (__VA_ARGS__);
#else
    #define LOG(...) ;
#endif


#define BYTE(X) 8*X

#pragma GCC diagnostic ignored "-Wwrite-strings"

// ===============================================
// Constants ||
//           \/

const int MEMORY_ALIGNMENT = 4096;

const int MAX_IP = 1000;

const int MAX_LABELS = 100;

const int MAX_OPCODE_LEN = 6;


const int HEADER_OFFSET = 8;

const int PAGESIZE = 4096;

const int MEMORY_OFFSET = 6000;


const int MEMORY_SIZE = 4096;

const uint64_t NIL = 0x0;

const uint64_t DWORD_SIZE = 16;

const uint64_t WORD_SIZE = 8;

const uint64_t ENTRY_POINT = 0x400078;

const int PRINTF_LEN = 219;

// ===============================================


// ===============================================
// Enums     ||
//           \/

#define DEF_CMD(CMD, id) CMD = id, 

enum EnumCommands
{
    #include "../codegen/cmds.h"

    MOV_REG_NUM_CMD = 20,
    NONE,
    END = 28,
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
    
    int native_size;
    int x86_size;
};

const struct InstructionSizes InstrSizes[] =
{
    {HLT,  1,   7},
    {PUSH, 10,  1}, 
    {MUL, 1, 26},
    {ADD, 1, 26},
    {SUB, 1, 26},
    {DIV, 1, 26},
    {POP, 10,  1},
    {OUT, 1, 41},
    {NONE, NONE_S, NONE_S}, // made for proper indexation
    {JMP, 10,  5},
    {JG,  10,  26},
    {JGE, 10,  26},
    {JA,  10,  26},
    {JAE, 10,  26},
    {JE,  10,  26},
    {JNE, 10,  26},
    {IN, 1, 18},
    {CALL, 10,  5},
    {RET, 1,  1},
    {SQR, 1, 16}
};


struct CmdsBuffer
{
    char* content;
    size_t len;
};


// IR - small but powerful

struct Command 
{
    EnumCommands name;

    int native_size;
    int native_ip;

    int x86_size;
    int x86_ip;
    
    int reg_index;
    elem_t value;   // might be push/pop value, jmp label

    int checksum;   // determines, which variation of push/pop is.
    bool is_skip;   // due optimizations, some commands might be deleted.
};


struct TranslatorInfo
{
    CmdsBuffer src_cmds; 
    CmdsBuffer dst_x86;

    Command** cmds_array; 
    int cmd_amount;

    int native_ip_counter;
    int x86_ip_counter;

    char* memory_buffer; // Buffer for commands, addressing to memory  
};

//=======================================

void TranslatorCtor (TranslatorInfo* self);

int AllocateCmdArrays (TranslatorInfo* self);

void ParseOnStructs (TranslatorInfo* self);

int CmdToStruct (const char* code, TranslatorInfo* self);

int FillCmdInfo (const char* code, TranslatorInfo* self);

void FillPushPopStruct (TranslatorInfo* self, Command* new_cmd,
                        const char* code, int cmd, int name);


void FillCmdIp (TranslatorInfo* self);

bool IsJump (int cmd);

void FillJumpLables (TranslatorInfo* self);

int FindJumpIp (TranslatorInfo* self, int native_ip);

void LogData(char* format, ...);

void DumpRawCmds (TranslatorInfo* self);

void Dump86Buffer (TranslatorInfo* self);

char* GetNameFromId (EnumCommands id);

void ReadFileToStruct (TranslatorInfo* self, FILE* file);

void DumpCurBuffer (char* cur_buff, size_t len);

void TranslatorDtor (TranslatorInfo* self);

void mprotect_change_rights (TranslatorInfo* self, int protect_status);

#endif
