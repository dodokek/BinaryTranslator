#include "../include/translator.h"

#ifdef DEBUG
    FILE* LOG_FILE = get_file ("log_file.txt", "w");
#endif

const char NotFound[] = "Not found\n";
const char* INPUT_FILE_PATH = "../../Processor/data/cmds.bin";


//  ===================================================================
// The JIT  made by
//                                     
//         88                      88              
//         88                      88              
//         88                      88              
//  ,adPPYb,88  ,adPPYba,   ,adPPYb,88  ,adPPYba,   
// a8"    `Y88 a8"     "8a a8"    `Y88 a8"     "8a  
// 8b       88 8b       d8 8b       88 8b       d8  
// "8a,   ,d88 "8a,   ,a8" "8a,   ,d88 "8a,   ,a8"  
//  `"8bbdP"Y8  `"YbbdP"'   `"8bbdP"Y8  `"YbbdP"'   
//
//  My final project in Ilya Dednisky's C programming course.
//  All my gained knowledge was extracted from my brain and thown into
//  this .cpp file
//  Please, enjoy.
//
//  Dodokek, 2023.
//  ===================================================================


void TranslatorCtor (TranslatorInfo* self)
{
    self->src_cmds.content = nullptr;
    self->src_cmds.len = 0;

    self->dst_x86.content = nullptr;
    self->dst_x86.len = 0;

    self->cmds_array = nullptr;
    self->cmds_counter = 0;

    self->orig_ip_counter = 0;
    self->x86_ip_counter = 0;
}


int AllocateCmdArrays (TranslatorInfo* self)
{
    self->cmds_array = (Command**) calloc (self->src_cmds.len, sizeof (Command*));

    self->dst_x86.content = (char*) aligned_alloc (PAGESIZE, self->src_cmds.len * sizeof (char)); // alignment for mprotect
    memset ((void*) self->dst_x86.content, 0x00, self->src_cmds.len);      // Filling whole buffer
                                                                           // With ret (0xC3) byte code

    self->memory_buffer = (char*) aligned_alloc (MEMORY_ALIGNMENT, MEMORY_SIZE * sizeof(char));
    memset ((void*) self->memory_buffer, 0xAA, MEMORY_SIZE); // filling for debug


    if (self->src_cmds.content != nullptr &&
        self->dst_x86.content  != nullptr &&
        self->memory_buffer    != nullptr)
        return SUCCESS;
    
    return ALLOCATION_FAILURE;
}


void ParseOnStructs (TranslatorInfo* self)
{   
    FILE* input_file = get_file (INPUT_FILE_PATH, "rb");
    ReadFileToStruct (self, input_file);
    close_file (input_file, INPUT_FILE_PATH);

    if (AllocateCmdArrays (self) == ALLOCATION_FAILURE)
        LOG ("Failed to allocate the memory for buffers");

    self->orig_ip_counter = HEADER_OFFSET;

    int counter = 0;
    while (self->orig_ip_counter < self->src_cmds.len)
    {
        LOG ("Ip %3d:\n", self->orig_ip_counter);

        int flag = CmdToStruct (self->src_cmds.content + self->orig_ip_counter, self);
        

        if (flag == END_OF_PROG)
            break;   

    }
}


int FillCmdInfo (const char* code, TranslatorInfo* self)
{
    int name = *code & CMD_BITMASK; 
    int cmd = *code;

    Command* new_cmd = (Command*) calloc (1, sizeof (Command));

    new_cmd->orig_ip = self->orig_ip_counter;
    new_cmd->x86_ip  = self->x86_ip_counter;

    if (name == PUSH || name == POP)
    {
        FillPushPopStruct (self, new_cmd, code, cmd, name);
    }
    else
    {
        if (IsJump (name))
            new_cmd->value = *(elem_t*)(code + 1);

        new_cmd->name = (EnumCommands) name;
        
        self->x86_ip_counter  += InstrSizes[name].x86_size;
        self->orig_ip_counter += InstrSizes[name].original_size;
    }

    self->cmds_array[self->cmds_counter] = new_cmd;
    self->cmds_counter++;

    return 0;
}


void FillPushPopStruct (TranslatorInfo* self, Command* new_cmd,
                        const char* code, int cmd, int name)
{
    new_cmd->name = (EnumCommands) name;
    
    new_cmd->reg_index = code[sizeof(elem_t) + BYTE_OFFSET];
    new_cmd->value = *(elem_t*)(code + 1);

    new_cmd->checksum  = 0;
    new_cmd->checksum |= (cmd & ARG_IMMED); 
    new_cmd->checksum |= (cmd & ARG_MEM); 
    new_cmd->checksum |= (cmd & ARG_REG); 
    
    self->orig_ip_counter += InstrSizes[name].original_size;
    
    switch (new_cmd->checksum)
    {
    case IMM:
        if (name == PUSH)
            self->x86_ip_counter += PUSH_IMM_SIZE; 
        else
            self->x86_ip_counter += POP_REG_SIZE;
        break;

    case REG:
        if (name == PUSH)
            self->x86_ip_counter += PUSH_REG_SIZE; 
        else
            self->x86_ip_counter += POP_REG_SIZE;
        break;
    
    case IMM_RAM:
        if (name == PUSH)
            self->x86_ip_counter += PUSH_IMM_RAM_SIZE; 
        else
            self->x86_ip_counter += POP_IMM_RAM_SIZE;
        break;

    case REG_RAM:
        if (name == PUSH)
            self->x86_ip_counter += PUSH_REG_RAM_SIZE; 
        else
            self->x86_ip_counter += POP_REG_RAM_SIZE;
        break;

    case IMM_REG_RAM:
        if (name == PUSH)
            self->x86_ip_counter += PUSH_IMM_REG_RAM_SIZE; 
        else
            self->x86_ip_counter += POP_IMM_REG_RAM_SIZE;
        break;

    default:
        LOG ("**1: No such variation of Push/Pop** %d\n", new_cmd->checksum);
        break;
    }
}


bool IsJump (int cmd)
{
    if (cmd == JMP || cmd == JG || cmd == JGE || cmd == JA ||
        cmd == JAE || cmd == JE || cmd == JNE || cmd == CALL)
    {
        LOG ("\tYeah, it is jump\n");
        return true;
    }
    return false;
}


void FillJumpLables (TranslatorInfo* self)
{
    LOG ("\n\n---------- Filling labels --------\n\n");

    for (int i = 0; i < self->cmds_counter; i++)
    {
        if (IsJump(self->cmds_array[i]->name))
        {
            LOG ("%2d: Trying to find x86 ip for ip %lg:\n", i, self->cmds_array[i]->value);

            self->cmds_array[i]->value = FindJumpIp (self, self->cmds_array[i]->value);
        }
    }

    LOG ("\n\n---------- End filling labels --------\n\n");
}


int FindJumpIp (TranslatorInfo* self, int orig_ip)
{
    for (int i = 0; i < self->cmds_counter; i++)
    {
        if (self->cmds_array[i]->orig_ip == orig_ip)
        {
            LOG ("\t Found, x86 ip is %d\n", self->cmds_array[i]->x86_ip)
            return self->cmds_array[i]->x86_ip;
        }
    }

    LOG ("\tNot found\n");

    return -1;
}


void StartTranslation (TranslatorInfo* self)
{
    LOG ("---------- Begin translation -------------\n");

    Opcode mov_r10 = {
        .code = MOV_R10,
        .size = SIZE_MOV_R10
    };

    WriteCmd (self, mov_r10);
    WriteAbsPtr(self, (uint64_t) self->memory_buffer);

    for (int cmd_indx = 0; cmd_indx < self->cmds_counter; cmd_indx++)
    {
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

        case OUT:
            LOG ("Translating OUT");
            TranslateOut (self, self->cmds_array[cmd_indx]);
            break;

        case RET:
            LOG ("Translating RET");
            TranslateRet (self, self->cmds_array[cmd_indx]);

        default:
            break;
        }
    }

    Opcode footer = {
        .code = RET_OP,
        .size = SIZE_RET
    };
    WriteCmd (self, footer);

}


void RunCode (TranslatorInfo* self)
{
    int flag = mprotect (self->dst_x86.content, self->dst_x86.len + 1, PROT_EXEC);
        // flag = mprotect (self->dst_x86.content, self->dst_x86.len + 1, PROT_WRITE);
        // flag = mprotect (self->dst_x86.content, self->dst_x86.len + 1, PROT_READ);
    
    printf ("Address: %p\n", RunCode);

    if (flag == -1)
    {
        LOG ("**** mprotect error ****\n");
        return;
    }

    void (* god_save_me)(void) = (void (*)(void))(self->dst_x86.content);

    god_save_me();

    // printf ("Bruh: %d\n", kek);
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

    case REG_RAM:
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


void CursedOut (double num)
{
    printf ("Output: %g\n", num);
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

    uint32_t out_ptr = (uint64_t)CursedOut - 
                   (uint64_t)(self->dst_x86.content + cur_cmd->x86_ip + 28 + sizeof (int));
                                      
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

    // LoadToX86Buffer (self, x86_buffer,   sizeof (x86_buffer));
    // LoadToX86Buffer (self, x86_addition, sizeof (x86_addition));
}


void TranslatePop (TranslatorInfo* self, Command* cur_cmd)
{
    // No such command in original proc
}


void TranslatePushRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x48, 0x8B, 0x00,  // mov rsi, [r_x]
                          0x56              // push rsi
     }; 

    switch (cur_cmd->reg_index)
    {
        case RAX:
            x86_buffer[2] = 0x30;      // mov rsi, [rax]
            break;
        case RCX:
            x86_buffer[2] = 0x31;      // mov rsi, [rcx]
            break;
        case RDX:
            x86_buffer[2] = 0x32;      // mov rsi, [rdx]
            break;
        case RBX:
            x86_buffer[2] = 0x33;      // mov rsi, [rbx]
            break;
    
        default:
            LOG ("**No such register!**\n");
            break;
    }

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePopRegRam (TranslatorInfo* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x5E,             // pop rsi
                          0x48, 0x89, 0x00  // mov [r_x], rsi
     }; 

    switch (cur_cmd->reg_index)
    {
        case RAX:
            x86_buffer[3] = 0x30;      // mov rsi, [rax]
            break;
        case RCX:
            x86_buffer[3] = 0x31;      // mov rsi, [rcx]
            break;
        case RDX:
            x86_buffer[3] = 0x32;      // mov rsi, [rdx]
            break;
        case RBX:
            x86_buffer[3] = 0x33;      // mov rsi, [rbx]
            break;
    
        default:
            LOG ("**No such register!**\n");
            break;
    }

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
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
    Opcode pop_reg = {
        .code = PUSH_REG + cur_cmd->reg_index,
        .size = PUSH_REG_SIZE
    };

    WriteCmd (self, pop_reg);
}


void TranslatePushImm (TranslatorInfo* self, Command* cur_cmd)
{
    Opcode mov_rsi = {
        .code = MOV_RSI,
        .size = SIZE_MOV_RSI
    };

    Opcode push_rsi = {
        .code = PUSH_RSI,
        .size = SIZE_PUSH_RSI
    };

    WriteCmd (self, mov_rsi);
    WriteDoubleNum (self, cur_cmd->value);
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
        
    Opcode pop_rsi = {
        .code = POP_RSI,
        .size = SIZE_POP_RSI
    };

    Opcode pop_rdi = {
        .code = POP_RDI,
        .size = SIZE_POP_RDI
    };

    Opcode cmp_rdi_rsi = {
        .code = CMP_RDI_RSI,
        .size = SIZE_CMP_RSI_RDI
    };

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

    uint32_t rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 2 + sizeof(int) + 5);
    
    WriteCmd (self, pop_rsi);
    WriteCmd (self, pop_rdi);
    WriteCmd (self, cmp_rdi_rsi);
    WriteCmd (self, cond_jmp);
    WritePtr (self, rel_ptr);

}



int CmdToStruct (const char* code, TranslatorInfo* self)
{

    #define DEF_CMD(name, id)            \
        case name:                       \
            LOG ("\tId: %d\n", name);    \
            FillCmdInfo (code, self);    \
            return 0;                    \

    switch (*code & CMD_BITMASK)
    {
        #include "../codegen/cmds.h"    // generating cases for all cmds

        case HLT:
            LOG ("End of prog.\n");
            return END_OF_PROG;

        default:
            LOG ("*!**!**!**!* SIGILL %d\n *!**!**!**!*", *code);
            return END_OF_PROG;
    }

    #undef DEF_CMD

}


// =========================== DUMP ===============================



void DumpCurBuffer (char* cur_buff, size_t len)
{
    LOG ("\n##########");

    for (int i = 0; i < len; i++)
    {
        if (i % 8 == 0) LOG ("\n\t%2d: ", i);

        if (cur_buff[i] == (char)144) 
        {
            LOG ("| ");
        }
        else
        {
            LOG ("%02x ",  (unsigned char) cur_buff[i]);
        }
    }

    LOG ("\n##########\n\n");

}



void DumpRawCmds (TranslatorInfo* self)
{
    LOG ("\n================ Begin of struct dump ==================\n");

    for (int i = 0; i < self->cmds_counter; i++)
    {
        Command* cur_cmd = self->cmds_array[i];

        LOG ("\nCommand < %s > | Orig ip: %d | x86 ip: %d\n",
                GetNameFromId(cur_cmd->name), cur_cmd->orig_ip, cur_cmd->x86_ip);
        if (cur_cmd->name == PUSH || cur_cmd->name == POP)
        {
            LOG ("Checksum: %d\n", cur_cmd->checksum);
            if (cur_cmd->checksum & ARG_REG)
                LOG ("\t+Using register, its id: %d\n", cur_cmd->reg_index);
            if (cur_cmd->checksum & ARG_IMMED)
                LOG ("\t+Operating width digit, value: %lg\n", cur_cmd->value);
            if (cur_cmd->checksum & ARG_MEM)
                LOG ("--- Adresses to memory\n");
        }
        if (IsJump(cur_cmd->name))
        {
            LOG ("\t+Wants to jump into %lg\n", cur_cmd->value);
        }
    }

    LOG ("\n================ End of struct dump ==================\n\n");
}


void Dump86Buffer (TranslatorInfo* self)
{
    LOG ("\n\n====== x86 buffer dump begin =======\n");

    for (int i = 0; i < self->dst_x86.len + 1; i++)
    {
        if (i % 8 == 0) LOG ("\n%2d: ", i);

        if (self->dst_x86.content[i] == (char)144) 
        {
            LOG ("|  ");
        }
        else
        {
            LOG ("%02x ", (unsigned char) self->dst_x86.content[i]);
        }
    }

    LOG ("\n\n====== x86 buffer dump end =======\n\n");

}


char* GetNameFromId (EnumCommands id)
{
    #define DEF_CMD(name, id)    \
        case name:               \
            return (char*) #name;        \


    switch (id)
    {
        #include "../codegen/cmds.h"    // generating cases for all cmds

        default:
            return (char*) NotFound;
    }

    return (char*) NotFound;
}


void ReadFileToStruct (TranslatorInfo* self, FILE* file)
{
    assert (file != nullptr);

    fseek (file, 0L, SEEK_END);
    long int file_len = ftell (file);
    fseek (file, 0L, SEEK_SET);

    char* buffer = (char*) calloc (file_len, sizeof (char));

    self->src_cmds.content = buffer;
    self->src_cmds.len = (int) fread (buffer, sizeof(char), file_len, file);

    return;
}
