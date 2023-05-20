#include "../include/translation.h"

void StartTranslation (TranslatorInfo* self)
{
    LOG ("---------- Begin translation -------------\n");

    EMIT (mov_r10, MOV_R10, SIZE_MOV_R10);

    WriteAbsPtr(self, ENTRY_POINT + MEMORY_OFFSET );

    for (int cmd_indx = 0; cmd_indx < self->cmd_amount; cmd_indx++)
    {   
        if (self->cmds_array[cmd_indx]->is_skip) continue;

        switch (self->cmds_array[cmd_indx]->name)
        {
        case PUSH:
        case POP:
            HandlePushPopVariation (self, self->cmds_array[cmd_indx]);
            break;

        case JMP:
        case CALL:
            LOG ("Translating Jump or Call");
            TranslateJmpCall (self, self->cmds_array[cmd_indx]);
            break;

        case JG:
        case JGE:
        case JA:
        case JAE:
        case JE:
        case JNE:
            LOG ("%d: Translating jmp\n", cmd_indx);
            TranslateConditionJmp (self, self->cmds_array[cmd_indx]);
            break;
        
        case MUL:
        case ADD:
        case SUB:
        case DIV:
            LOG ("%d: Translating math\n", cmd_indx);
            TranslateBaseMath (self, self->cmds_array[cmd_indx]);
            break;

        case SQR:
            TranslateSqr (self, self->cmds_array[cmd_indx]);
            break;

        case OUT:
            LOG ("Translating OUT");
            TranslateOut (self, self->cmds_array[cmd_indx]);
            break;

        case IN:
            LOG ("Translating IN");
            TranslateIn (self, self->cmds_array[cmd_indx]);
            break;

        case RET:
            LOG ("Translating RET");
            TranslateRet (self, self->cmds_array[cmd_indx]);
            break;

        case MOV_REG_NUM_CMD:
            LOG ("Translating mov reg, num\n");
            TranslateMovRegNum (self, self->cmds_array[cmd_indx]);
            break;

        case HLT:
            LOG ("Translating exit()\n");
            TranslateHlt (self, self->cmds_array[cmd_indx]);
            break;

        default:
            break;
        }
    }

    // Opcode ret = {
    //     .code = RET_OP,
    //     .size = SIZE_RET
    // };
    // WriteCmd (self, ret);
}


// ======================= Translation Units ==============================

void WriteCmd (TranslatorInfo* self, Opcode cmd)
{
    DumpCurBuffer ((char*) &cmd.code, cmd.size);

    *(u_int64_t*) (self->dst_x86.content + self->dst_x86.len) = cmd.code;
    self->dst_x86.len += cmd.size;
}

void WriteDoubleNum (TranslatorInfo* self, double value)
{
    *(double*) (self->dst_x86.content + self->dst_x86.len) = value;
    self->dst_x86.len += sizeof (double);
}

void WritePtr (TranslatorInfo* self, uint32_t ptr)
{
    *(uint32_t*) (self->dst_x86.content + self->dst_x86.len) = ptr;
    self->dst_x86.len += sizeof (uint32_t);
}

void WriteAbsPtr (TranslatorInfo* self, uint64_t ptr)
{
    *(uint64_t*) (self->dst_x86.content + self->dst_x86.len) = ptr;
    self->dst_x86.len += sizeof (uint64_t);
}


void TranslateMovRegNum (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (mov_reg_num, MOV_REG_NUM | (RAX_MASK + cur_cmd->reg_index) << BYTE(1), SIZE_MOV_REG_NUM);

    WriteDoubleNum (self, cur_cmd->value);
}

void TranslateHlt (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (syscall_exit, SYSCALL, SYSCALL_SIZE);
}


void HandlePushPopVariation (TranslatorInfo* self, Command* cur_cmd)
{
    switch (cur_cmd->checksum)
    {
    case VOID:
        TranslatePop (self, cur_cmd);
        break;

    case IMM:
        LOG ("<%d>:Push IMM\n", cur_cmd->x86_ip);
        TranslatePushImm (self, cur_cmd);
        break;

    case REG:
        LOG ("%d:Push/Pop REG\n", cur_cmd->x86_ip);
        if (cur_cmd->name == PUSH)
            TranslatePushReg (self, cur_cmd);
        else
            TranslatePopReg (self, cur_cmd);
        break;

    case IMM_RAM:
        if (cur_cmd->name == PUSH)
            TranslatePushImmRam (self, cur_cmd);
        else
            TranslatePopImmRam (self, cur_cmd);
        break;

    case IMM_REG_RAM:
    case REG_RAM:
        LOG ("Translating push ram\n");
        if (cur_cmd->name == PUSH)
            TranslatePushRegRam (self, cur_cmd);
        else
            TranslatePopRegRam (self, cur_cmd);
        break;

    default:
        LOG ("**2: No such variation of Push/Pop** %d\n", cur_cmd->checksum);

        break;
    }
}


void DoublePrintf (double num)
{
    fprintf (stderr, "%g\n", num);
}


void TranslateOut (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (mov_xmm0_stack, MOV_XMM_RSP | XMM0_MASK << BYTE(3), SIZE_MOV_XMM_RSP);
    EMIT (double_to_int,  CVTSD2SI_RSI_XMM0,                  SIZE_CVTSD2SI);
    EMIT (push_rsi,       PUSH_RSI,                           SIZE_PUSH_RSI);
    EMIT (pop_rax,        POP_REG,                            SIZE_POP_REG);

    EMIT (push_all,       PUSH_ALL,    SIZE_PUSH_POP_All);
    EMIT (mov_rbp_rsp,    MOV_RBP_RSP, SIZE_MOV_REG_REG);
    EMIT (align_stack,    AND_RSP_FF,  SIZE_AND_RSP);
    EMIT (call_double_printf, CALL_OP, SIZE_JMP);

    uint32_t out_ptr = (uint64_t) self->x86_ip_counter - cur_cmd->x86_ip - 27 - sizeof (int);                              
    WritePtr (self, out_ptr);


    EMIT (mov_rsp_rbp, MOV_RSP_RBP, SIZE_MOV_REG_REG);
    EMIT (popa,        POP_ALL,     SIZE_PUSH_POP_All);
    EMIT (pop_rdi,     POP_RDI,     SIZE_POP_RDI);

}


void DoubleScanf (double* num)
{
    printf ("Enter the number: ");
    scanf ("%lf", num);
}


void TranslateIn (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (push_all,          PUSH_ALL, SIZE_PUSH_POP_All);
    EMIT (call_double_scanf, CALL_OP,  SIZE_JMP);    

    uint32_t in_ptr = (uint64_t) self->x86_ip_counter - cur_cmd->x86_ip - 7 - sizeof (int) + PRINTF_LEN;

    WritePtr (self, in_ptr);

    EMIT (pop_all,  POP_ALL,  SIZE_PUSH_POP_All);
    EMIT (push_rsi, PUSH_RSI, SIZE_PUSH_RSI);

}


void TranslateSqr (TranslatorInfo* self, Command* cur_cmd)
{
    // movsd xmm0, [rsp]
    // sqrt xmm0, xmm0
    // movsd [rsp], xmm0

    EMIT (mov_xmm0_stack, MOV_XMM_RSP | XMM0_MASK << BYTE(3), SIZE_MOV_XMM_RSP);
    EMIT (sqrt_xmm0_xmm0, SQRTPD_XMM0_XMM0,                   SIZE_SQRT);
    EMIT (mov_rsp_xmm0,   MOV_RSP_XMM | XMM0_MASK << BYTE(3), SIZE_MOV_XMM_RSP);

}


void TranslateBaseMath (TranslatorInfo* self, Command* cur_cmd)
{
    // movsd xmm0, [rsp]
    // movsd xmm1, [rsp+8]
    // add rsp, 8
    // add, sub, mul or div. third byte is changed
    // movsd [rsp], xmm0

    EMIT (movsd_xmm0_rsp,   MOV_XMM_RSP | XMM0_MASK << BYTE(3),                           SIZE_MOV_XMM_RSP);
    EMIT (movsd_xmm1_rsp_8, MOV_XMM_RSP | XMM1_MASK << BYTE(3) | (WORD_SIZE) << BYTE (5), SIZE_MOV_XMM_RSP);
    EMIT (add_rsp_8,        ADD_RSP | WORD_SIZE << BYTE (3),                              SIZE_ADD_RSP);

    Opcode arithm_xmm0_xmm1 = {
        .code = ARITHM_XMM0_XMM1,
        .size = SIZE_ARITHM_XMM
    };

    switch (cur_cmd->name)
    {
        case ADD:
            arithm_xmm0_xmm1.code |= ADD_MASK << BYTE(2);
            break;

        case MUL:
            arithm_xmm0_xmm1.code |= MUL_MASK << BYTE(2);
            break;

        case SUB:
            arithm_xmm0_xmm1.code |= SUB_MASK << BYTE(2);
            break;

        case DIV:
            arithm_xmm0_xmm1.code |= DIV_MASK << BYTE(2);
            break;
        
        default:
            LOG ("No such Math operation\n");
            break;
    }

    WriteCmd (self, arithm_xmm0_xmm1);

    EMIT (mov_rsp_xmm0, MOV_RSP_XMM | XMM0_MASK << BYTE(3), SIZE_MOV_XMM_RSP);
}


void TranslatePop (TranslatorInfo* self, Command* cur_cmd)
{
    // No such command in original proc
}


void TranslatePushRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    RegToRsiOffset (self, cur_cmd);

    EMIT (add_r10_rsi,     ADD_R10_RSI, SIZE_R10_RSI);

    // Push to stack part
    EMIT (mov_rdi_r10_mem, MOV_RDI_R10, SIZE_MOV_REG_REG);

    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    EMIT (push_rdi,        PUSH_RDI, SIZE_PUSH_RDI);

    // Back to normal mem ptr
    
    EMIT (sub_r10_rsi,     SUB_R10_RSI, SIZE_R10_RSI);
}


void RegToRsiOffset(TranslatorInfo* self, Command* cur_cmd)
{
    // Translating double data in register to actual integer offset in memory
    // (double) r?x -> (int) rsi * 8

    EMIT (push_reg,       PUSH_REG    + cur_cmd->reg_index,    PUSH_REG_SIZE);
    
    EMIT (movsd_xmm0_rsp, MOV_XMM_RSP | XMM0_MASK << BYTE(3),  SIZE_MOV_XMM_RSP);
    
    EMIT (add_rsp_8,      ADD_RSP     | WORD_SIZE << BYTE (3), SIZE_ADD_RSP);
    
    EMIT (double_to_int,  CVTSD2SI_RSI_XMM0,                   SIZE_CVTSD2SI);
    
    EMIT (shl_rsi_3,      SHL_RSI     | (3) << BYTE(3),        SIZE_SHL);
}


void TranslatePopRegRam (TranslatorInfo* self, Command* cur_cmd)
{

    RegToRsiOffset (self, cur_cmd);

    // Adjusting memory pointer

    EMIT (add_r10_rsi, ADD_R10_RSI, SIZE_R10_RSI);

    // Pop to stack part

    EMIT (pop_rdi,         POP_RDI,     SIZE_POP_RDI);
    EMIT (mov_r10_mem_rsi, MOV_R10_RDI, SIZE_MOV_REG_REG);

    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    // Back to normal mem ptr

    EMIT (sub_r10_rsi, SUB_R10_RSI, SIZE_R10_RSI);
}


void TranslatePopReg (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (pop_reg, POP_REG + cur_cmd->reg_index, POP_REG_SIZE);
}


void TranslatePushReg (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (push_reg, PUSH_REG + cur_cmd->reg_index, PUSH_REG_SIZE);
}


void TranslatePushImm (TranslatorInfo* self, Command* cur_cmd)
{
    EMIT (mov_rsi,  MOV_RSI,  SIZE_MOV_RSI);

    WriteDoubleNum (self, cur_cmd->value);

    EMIT (push_rsi, PUSH_RSI, SIZE_PUSH_RSI);
}


void TranslatePushImmRam (TranslatorInfo* self, Command* cur_cmd)
{
    // mov rdi, [r10 + ?]
    // push rdi

    EMIT (mov_rdi_r10, MOV_RDI_R10, SIZE_MOV_REG_REG);

    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    EMIT (push_rdi,    PUSH_RDI, SIZE_PUSH_RDI);
}


void TranslatePopImmRam (TranslatorInfo* self, Command* cur_cmd)
{
    // pop rdi
    // mov [r10 + ?], rdi

    EMIT (pop_rdi,     POP_RDI,     SIZE_POP_RDI);
    EMIT (mov_r10_rdi, MOV_R10_RDI, SIZE_MOV_REG_REG);
    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));
}


void TranslatePushImmRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    //
}


void TranslatePopImmRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    //
}


void TranslateJmpCall (TranslatorInfo* self, Command* jmp_cmd)
{
    Opcode jmp_call = {
        .size = SIZE_JMP
    };

    if (jmp_cmd->name == CALL)
        jmp_call.code = CALL_OP;
    else
        jmp_call.code = JMP_OP;

    WriteCmd (self, jmp_call);

    uint32_t rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 1 + sizeof (int));

    WritePtr (self, rel_ptr);
}


void TranslateRet (TranslatorInfo* self, Command* jmp_cmd)
{
    EMIT (ret, RET_OP, SIZE_RET);
}


void TranslateConditionJmp (TranslatorInfo* self, Command* jmp_cmd)
{

    // mov xmm0, [rsp]
    // mov xmm1, [rsp+8]
    // add rsp, 16
    // ucomisd xmm0, xmm1
    // j?? ptr

    EMIT (movsd_xmm0_rsp,   MOV_XMM_RSP | XMM0_MASK << BYTE(3),                           SIZE_MOV_XMM_RSP);
    EMIT (movsd_xmm1_rsp_8, MOV_XMM_RSP | XMM1_MASK << BYTE(3) | (WORD_SIZE) << BYTE (5), SIZE_MOV_XMM_RSP);    
    
    EMIT (add_rsp_16,    ADD_RSP | (WORD_SIZE * 2) << BYTE (3), SIZE_ADD_RSP);
    EMIT (cmp_xmm0_xmm1, CMP_XMM0_XMM1,                         SIZE_CMP_XMM);
    
    Opcode cond_jmp = {
        .code = COND_JMP,
        .size = SIZE_COND_JMP
    };

    switch (jmp_cmd->name)
    {
        case JE:
            cond_jmp.code |= JE_MASK << BYTE(1);   
            break;
        case JNE:
            cond_jmp.code |= JNE_MASK << BYTE(1);  
            break;
        case JG:
            cond_jmp.code |= JG_MASK << BYTE(1); 
            break;
        case JAE:
            cond_jmp.code |= JAE_MASK << BYTE(1); 
            break;
        case JGE:
            cond_jmp.code |= JGE_MASK << BYTE(1); 
            break;
        case JA:
            cond_jmp.code |= JA_MASK << BYTE(1); 
            break;
    
        default:
            LOG ("No such conditional jmp!\n");
            break;
    }

    uint32_t rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 2 + sizeof(int) + 20);
    
    WriteCmd (self, cond_jmp);
    WritePtr (self, rel_ptr);

}
