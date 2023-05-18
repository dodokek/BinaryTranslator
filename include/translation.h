#ifndef TRANS_H
#define TRANS_H

#include "parser.h"

//========================
// Defines

#define EMIT(name, cmd, size_)                                                  \
        Opcode name = {                                                 \
            .code = cmd,  \
            .size = size_                                           \
        };                                                                     \
        WriteCmd (self, name);           


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

    LEA_RDI_RSP = 0x00247C8D48, // lea rdi, [rsp + ?]
            //       ^----------------------------+

    MOV_REG_NUM = 0x0048,    // mov r?x, double num. followed with double word
            //      ^----------------+ 
// Constant expressions, no need for bit masks

    SYSCALL = 0x050fff31583c6a,

    MOV_R10 = 0xBA49,   // mov r10, <64b ptr>. Begin of memory must be stored in R10
    CALL_OP = 0xE8,     // call <32b ptr>
    RET_OP = 0xC3,      // ret

    JMP_OP = 0xE9,      // jmp <32b ptr>

    PUSH_RSI = 0x56,    // push rsi
    PUSH_RDI = 0x57,    // push rdi

    POP_RSI = 0x5E,     // push rsi
    POP_RDI = 0x5F,     // pop rdi

    CMP_RDI_RSI = 0xf73948, // cmd rdi, rsi
    CMP_XMM0_XMM1 = 0xC12E0F66, // ucomisd xmm0, xmm1

    PUSH_ALL = 0x505152535241,  // push r10 - rax - ... - rdx
    POP_ALL = 0x5A415B5A5958,   // pop rdx - ... rax - r10

    MOV_RBP_RSP = 0xE48949,
    MOV_RSP_RBP = 0xE4894C,
    AND_RSP_FF = 0xF0E48348,

    MOV_RSI = 0xBE48, // mov rsi, (double num)
                      // next 8 bytes should be double number

    ADD_R10_RSI = 0xF20149,
    SUB_R10_RSI = 0xF22949,
    SQRTPD_XMM0_XMM0 = 0xC0510F66,   // get square root from xmm0 and store it in xmm0   
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
    JG_MASK = 0x8f,
    JAE_MASK = 0x83,
    JGE_MASK = 0x8d,
    JA_MASK = 0x87,

    RAX_MASK = 0xb8,
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

    SIZE_CMP_XMM = 4,
    SIZE_LEA_RDI_RSP = 5,

    SIZE_MOV_REG_NUM = 2,
    SYSCALL_SIZE = 7,
};


const int WRAPPER_OFFSET = 30;

// ===============================================

void DoubleScanf (double* num);

void StartTranslation (TranslatorInfo* self);

void WriteDoubleNum (TranslatorInfo* self, double value);

void WriteCmd (TranslatorInfo* self, Opcode cmd);

void WritePtr (TranslatorInfo* self, uint32_t ptr);

void WriteAbsPtr (TranslatorInfo* self, uint64_t ptr);

void RunCode (TranslatorInfo* self);

//-- Translation Units:

void TranslateMovRegNum (TranslatorInfo* self, Command* cur_cmd);

void TranslateIn (TranslatorInfo* self, Command* cur_cmd);

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

void TranslateHlt (TranslatorInfo* self, Command* cur_cmd);

#endif
