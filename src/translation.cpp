#include "../include/translation.h"

void StartTranslation (TranslatorInfo* self)
{
    LOG ("---------- Begin translation -------------\n");

    Opcode mov_r10 = {
        .code = MOV_R10,
        .size = SIZE_MOV_R10
    };

    WriteCmd (self, mov_r10);
    WriteAbsPtr(self, (uint64_t) self->memory_buffer);

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

        default:
            break;
        }
    }

    Opcode ret = {
        .code = RET_OP,
        .size = SIZE_RET
    };
    WriteCmd (self, ret);

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
    Opcode mov_reg_num = {
        .code = MOV_REG_NUM | (RAX_MASK + cur_cmd->reg_index) << BYTE(1),
        .size = SIZE_MOV_REG_NUM
    };
    WriteCmd (self, mov_reg_num);

    WriteDoubleNum (self, cur_cmd->value);
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
    printf ("=====\nOutput: %g\n=====\n", num);
}


void TranslateOut (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode movsd_xmm0_rsp = {
        .code = MOV_XMM_RSP | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };
    Opcode pusha = {
        .code = PUSH_ALL,
        .size = SIZE_PUSH_POP_All
    };
    Opcode mov_rbp_rsp = {
        .code = MOV_RBP_RSP,
        .size = SIZE_MOV_REG_REG
    };
    Opcode stack_align = {
        .code = AND_RSP_FF,
        .size = SIZE_AND_RSP
    };

    WriteCmd (self, movsd_xmm0_rsp);
    WriteCmd (self, pusha);
    WriteCmd (self, mov_rbp_rsp);
    WriteCmd (self, stack_align);

    Opcode call_out = {
        .code = CALL_OP,
        .size = SIZE_JMP
    };

    WriteCmd (self, call_out);

    uint32_t out_ptr = (uint64_t)DoublePrintf - 
                   (uint64_t)(self->dst_x86.content + cur_cmd->x86_ip + 30 + sizeof (int));
                                      
    WritePtr (self, out_ptr);

    Opcode mov_rsp_rbp = {
        .code = MOV_RSP_RBP,
        .size = SIZE_MOV_REG_REG,
    };

    Opcode popa = {
        .code = POP_ALL,
        .size = SIZE_PUSH_POP_All
    };

    Opcode pop_rdi = {
        .code = POP_RDI,
        .size = SIZE_POP_RDI
    };

    WriteCmd (self, mov_rsp_rbp);
    WriteCmd (self, popa);
    WriteCmd (self, pop_rdi);
}


void DoubleScanf (double* num)
{
    printf ("Enter the number: ");
    scanf ("%lf", num);
}


void TranslateIn (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode push_rdi = {
        .code = PUSH_RDI,
        .size = SIZE_PUSH_RDI
    };
    WriteCmd (self, push_rdi);

    Opcode lea_rdi_rsp = {
        .code = LEA_RDI_RSP ,
        .size = SIZE_LEA_RDI_RSP
    };
    WriteCmd (self, lea_rdi_rsp);


    Opcode pusha = {
        .code = PUSH_ALL,
        .size = SIZE_PUSH_POP_All
    };
    Opcode mov_rbp_rsp = {
        .code = MOV_RBP_RSP,
        .size = SIZE_MOV_REG_REG
    };
    Opcode stack_align = {
        .code = AND_RSP_FF,
        .size = SIZE_AND_RSP
    };

    WriteCmd (self, pusha);
    WriteCmd (self, mov_rbp_rsp);
    WriteCmd (self, stack_align);

    Opcode call_in = {
        .code = CALL_OP,
        .size = SIZE_JMP
    };

    WriteCmd (self, call_in);

    uint32_t in_ptr = (uint64_t)DoubleScanf - 
                       (uint64_t)(self->dst_x86.content + cur_cmd->x86_ip + 30 + sizeof (int));
                                      
    WritePtr (self, in_ptr);

    Opcode mov_rsp_rbp = {
        .code = MOV_RSP_RBP,
        .size = SIZE_MOV_REG_REG,
    };

    Opcode popa = {
        .code = POP_ALL,
        .size = SIZE_PUSH_POP_All
    };

    WriteCmd (self, mov_rsp_rbp);
    WriteCmd (self, popa);
}


void TranslateSqr (TranslatorInfo* self, Command* cur_cmd)
{
    // movsd xmm0, [rsp]
    // sqrt xmm0, xmm0
    // movsd [rsp], xmm0

    Opcode movsd_xmm0_rsp = {
        .code = MOV_XMM_RSP | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    }; 

    Opcode sqrt_xmm0_xmm0 = {
        .code = SQRTPD_XMM0_XMM0,
        .size = SIZE_SQRT
    };

    Opcode movsd_rsp_xmm0 = {
        .code = MOV_RSP_XMM | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };

    WriteCmd (self, movsd_xmm0_rsp);
    WriteCmd (self, sqrt_xmm0_xmm0);
    WriteCmd (self, movsd_rsp_xmm0);

}


void TranslateBaseMath (TranslatorInfo* self, Command* cur_cmd)
{
    // movsd xmm0, [rsp]
    // movsd xmm1, [rsp+8]
    // add rsp, 8
    // add, sub, mul or div. third byte is changed
    // movsd [rsp], xmm0

    Opcode movsd_xmm0_rsp = {
        .code = MOV_XMM_RSP | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };
    
    WriteCmd (self, movsd_xmm0_rsp);

    Opcode movsd_xmm1_rsp_8 = {
        .code = MOV_XMM_RSP | XMM1_MASK << BYTE(3) | (WORD_SIZE) << BYTE (5),
        .size = SIZE_MOV_XMM_RSP
    };
    WriteCmd (self, movsd_xmm1_rsp_8);


    Opcode add_rsp_8 = {
        .code = ADD_RSP | WORD_SIZE << BYTE (3),
        .size = SIZE_ADD_RSP
    };
    WriteCmd (self, add_rsp_8);


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

    Opcode mov_rsp_xmm = {
        .code = MOV_RSP_XMM | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };

    WriteCmd (self, mov_rsp_xmm);

}


void TranslatePop (TranslatorInfo* self, Command* cur_cmd)
{
    // No such command in original proc
}


void TranslatePushRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    RegToRsiOffset (self, cur_cmd);

    Opcode add_r10_rsi = {
        .code = ADD_R10_RSI,
        .size = SIZE_R10_RSI 
    };
    WriteCmd (self, add_r10_rsi);

    // Push to stack part

    Opcode mov_rdi_r10_mem = {
        .code = MOV_RDI_R10,
        .size = SIZE_MOV_REG_REG
    };

    WriteCmd (self, mov_rdi_r10_mem);
    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    Opcode push_rdi = {
        .code = PUSH_RDI,
        .size = SIZE_PUSH_RDI
    };

    WriteCmd (self, push_rdi);

    // Back to normal mem ptr
    Opcode sub_r10_rsi = {
        .code = SUB_R10_RSI,
        .size = SIZE_R10_RSI
    };
    WriteCmd (self, sub_r10_rsi);

}


void RegToRsiOffset(TranslatorInfo* self, Command* cur_cmd)
{
    // Translating double data in register to actual integer offset in memory
    // (double) r?x -> (int) rsi * 8

    Opcode push_reg = {
        .code = PUSH_REG + cur_cmd->reg_index,
        .size = PUSH_REG_SIZE
    };
    WriteCmd (self, push_reg);

    Opcode movsd_xmm0_rsp = {
        .code = MOV_XMM_RSP | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };
    WriteCmd (self, movsd_xmm0_rsp);

    Opcode add_rsp_8 = {
        .code = ADD_RSP | WORD_SIZE << BYTE (3),
        .size = SIZE_ADD_RSP
    };
    WriteCmd (self, add_rsp_8);

    Opcode double_to_int = {
        .code = CVTSD2SI_RSI_XMM0,
        .size = SIZE_CVTSD2SI
    };
    WriteCmd (self, double_to_int);

    Opcode shl_rsi_3 = {
        .code = SHL_RSI | (3) << BYTE(3),
        .size = SIZE_SHL
    };
    WriteCmd (self, shl_rsi_3);
}


void TranslatePopRegRam (TranslatorInfo* self, Command* cur_cmd)
{

    RegToRsiOffset (self, cur_cmd);

    // Adjusting memory pointer

    Opcode add_r10_rsi = {
        .code = ADD_R10_RSI,
        .size = SIZE_R10_RSI 
    };
    WriteCmd (self, add_r10_rsi);

    // Pop to stack part

    Opcode pop_rdi = {
        .code = POP_RDI,
        .size = SIZE_POP_RDI
    };
    WriteCmd (self, pop_rdi);

    Opcode mov_r10_mem_rsi = {
        .code = MOV_R10_RDI,
        .size = SIZE_MOV_REG_REG
    };

    WriteCmd (self, mov_r10_mem_rsi);
    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    // Back to normal mem ptr
    Opcode sub_r10_rsi = {
        .code = SUB_R10_RSI,
        .size = SIZE_R10_RSI
    };
    WriteCmd (self, sub_r10_rsi);


}


void TranslatePopReg (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode pop_reg = {
        .code = POP_REG + cur_cmd->reg_index,
        .size = POP_REG_SIZE
    };

    WriteCmd (self, pop_reg);
}


void TranslatePushReg (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode push_reg = {
        .code = PUSH_REG + cur_cmd->reg_index,
        .size = PUSH_REG_SIZE
    };

    WriteCmd (self, push_reg);
}


void TranslatePushImm (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode mov_rsi = {
        .code = MOV_RSI,
        .size = SIZE_MOV_RSI
    };
    WriteCmd (self, mov_rsi);
    WriteDoubleNum (self, cur_cmd->value);

    Opcode push_rsi = {
        .code = PUSH_RSI,
        .size = SIZE_PUSH_RSI
    };

    WriteCmd (self, push_rsi);

}


void TranslatePushImmRam (TranslatorInfo* self, Command* cur_cmd)
{
    // mov rdi, [r10 + ?]
    // push rdi

    Opcode mov_rdi_r10 = {
        .code = MOV_RDI_R10,
        .size = SIZE_MOV_REG_REG
    };

    WriteCmd (self, mov_rdi_r10);
    WritePtr (self, (uint32_t) cur_cmd->value * sizeof (double));

    Opcode push_rdi = {
        .code = PUSH_RDI,
        .size = SIZE_PUSH_RDI
    };

    WriteCmd (self, push_rdi);
}


void TranslatePopImmRam (TranslatorInfo* self, Command* cur_cmd)
{
    // pop rdi
    // mov [r10 + ?], rdi

    Opcode pop_rdi = {
        .code = POP_RDI,
        .size = SIZE_POP_RDI
    };

    WriteCmd (self, pop_rdi);

    Opcode mov_r10_rdi = {
        .code = MOV_R10_RDI,
        .size = SIZE_MOV_REG_REG
    };

    WriteCmd (self, mov_r10_rdi);
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
    Opcode ret = {
        .code = RET_OP,
        .size = SIZE_RET
    };

    WriteCmd (self, ret);
}


void TranslateConditionJmp (TranslatorInfo* self, Command* jmp_cmd)
{

    // mov xmm0, [rsp]
    // mov xmm1, [rsp+8]
    // add rsp, 16
    // ucomisd xmm0, xmm1
    // j?? ptr

        
    Opcode movsd_xmm0_rsp = {
        .code = MOV_XMM_RSP | XMM0_MASK << BYTE(3),
        .size = SIZE_MOV_XMM_RSP
    };
    
    WriteCmd (self, movsd_xmm0_rsp);

    Opcode movsd_xmm1_rsp_8 = {
        .code = MOV_XMM_RSP | XMM1_MASK << BYTE(3) | (WORD_SIZE) << BYTE (5),
        .size = SIZE_MOV_XMM_RSP
    };
    WriteCmd (self, movsd_xmm1_rsp_8);

    Opcode add_rsp_16 = {
        .code = ADD_RSP | (WORD_SIZE * 2) << BYTE (3),
        .size = SIZE_ADD_RSP
    };
    WriteCmd (self, add_rsp_16);

    Opcode cmp_xmm0_xmm1 = {
        .code = CMP_XMM0_XMM1,
        .size = SIZE_CMP_XMM,
    };
    WriteCmd (self, cmp_xmm0_xmm1);

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
