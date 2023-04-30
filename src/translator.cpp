
#include "../include/translator.h"

FILE* LOG_FILE = get_file ("log_file.txt", "w");

extern "C" void DodoPrint (const char* template_string, ...);

int main()
{
    TranslatorMain TranslatorInfo = {};
    TranslatorCtor (&TranslatorInfo);

    ParseOnStructs (&TranslatorInfo);

    DumpRawCmds (&TranslatorInfo);

    FillJumpLables (&TranslatorInfo);
    DumpRawCmds (&TranslatorInfo);

    StartTranslation (&TranslatorInfo);

    Dump86Buffer (&TranslatorInfo);

    RunCode (&TranslatorInfo);
}

void RunCode (TranslatorMain* self)
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

    int (* god_save_me)(void) = (int (*)(void))(self->dst_x86.content);

    int kek = god_save_me();

    printf ("Bruh: %d\n", kek);
}


void StartTranslation (TranslatorMain* self)
{
    LOG ("---------- Begin translation -------------\n");

    char* memory_buffer = (char*) calloc (MEMORY_SIZE, sizeof(char));

    // LoadToX86Buffer (self, memory_buffer, sizeof (memory_buffer)); // at the beginning of prog space for RAM



    char header[] = { 0x56, 0x41, 0x52,   // push rsi; push r10
                      0x41, 0xBA, 0x00, 0x00, 0x00, 0x00 // mov r10, ptr of ram begin
    }; 

    *(uint64_t*)(header + 5) = (uint64_t) memory_buffer;

    LoadToX86Buffer (self, header, sizeof (header));


    for (int cmd_indx = 0; cmd_indx < self->cmds_counter; cmd_indx++)
    {
        switch (self->cmds_array[cmd_indx]->name)
        {
        case PUSH:
        case POP:
            HandlePushPopVariation (self, self->cmds_array[cmd_indx]);
            break;

        case JMP:
            TranslateJmp (self, self->cmds_array[cmd_indx]);
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

        default:
            break;
        }
    }

    // char opcode_buff[] = { 0x48, 0x89, 0xD8}; // mov rax, rbx
    // LoadToX86Buffer (self, opcode_buff, sizeof (opcode_buff));

    char footer[] = {0x41, 0x5A, 0x5e}; // pop r10; push rsi
    LoadToX86Buffer (self, footer, sizeof (footer));
}


void LoadToX86Buffer (TranslatorMain* self, char* op_code, size_t len)
{

    DumpCurBuffer (op_code, len);

    memcpy (self->dst_x86.content + self->dst_x86.len, (void*) op_code, len);
    self->dst_x86.len += len;

    // self->dst_x86.content[self->dst_x86.len] = 0x90; // nop at the end
    // self->dst_x86.len++;
}


void HandlePushPopVariation (TranslatorMain* self, Command* cur_cmd)
{
    int variation = CalcVariationSum (cur_cmd);

    switch (variation)
    {
    case VOID:
        TranslatePop (self, cur_cmd);
        break;

    case IMM:
        LOG ("<%d>:Push/Pop IMM\n", cur_cmd->x86_ip);
        if (cur_cmd->name == PUSH)
            TranslatePushImm (self, cur_cmd);
        // else
        //     // TranslatePopImm (self, cur_cmd);
        break;

    case REG:
        LOG ("%d:Push/Pop REG\n", cur_cmd->x86_ip);
        if (cur_cmd->name == PUSH)
            TranslatePushReg (self, cur_cmd);
        else
            TranslatePopReg (self, cur_cmd);
        break;
    
    case IMM_REG:
        /* code */
        break;

    case IMM_RAM:
        if (cur_cmd->name == PUSH)
            TranslatePushImmRam (self, cur_cmd);
        else
            TranslatePopImmRam (self, cur_cmd);
        break;

    case REG_RAM:
        /* code */
        break;

    case IMM_REG_RAM:
        /* code */
        break;

    default:
        LOG ("**No such variation of Push/Pop**\n");

        break;
    }

    
}


void CursedOut (double num)
{
    printf ("I am gay %g %%\n", num);
}


void TranslateOut (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = {

                        0xF2, 0x0F, 0x10, 0x04, 0x24,   // movsd xmm0, qword [rsp]
                        0x50, 0x53, 0x51, 0x52,         // push rax - ... - rdx       

                        0x49, 0x89, 0xE4,                // mov rbp, rsp

                        0x48, 0x83, 0xE4, 0xF0,         // and rsp, -16 - aligning stack

                        0xE8, 0x00, 0x00, 0x00, 0x00,   // call CursedOut

                        0x4C, 0x89, 0xE4,               // mov rsp, rbp

                        0x5A, 0x59, 0x5B, 0x58,         // pop rdx - ... - rax

                        0x5F        // pop rdi
                    };

    *(uint32_t *)(x86_buffer + 17) = (uint64_t)CursedOut - 
                                     (uint64_t)(self->dst_x86.content + cur_cmd->x86_ip + 26 + sizeof (int));

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslateBaseMath (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0xF2, 0x0F, 0x10, 0x04, 0x24,       // movsd xmm0, [rsp]
                          0xF2, 0x0F, 0x10, 0x4C, 0x24, 0x08, // movsd xmm1, [rsp+8]
                          0x48, 0x83, 0xC4, 0x10,             // add rsp, 16
                          0xF2, 0x0F, 0x00, 0xC1              // add, sub, mul or div. third byte is changed
    };

    switch (cur_cmd->name)
    {
        case ADD:
            x86_buffer[17] = 0x58;
            break;

        case MUL:
            x86_buffer[17] = 0x59;
            break;

        case SUB:
            x86_buffer[17] = 0x5C;
            break;

        case DIV:
            x86_buffer[17] = 0x5E;
            break;
        
        default:
            LOG ("No such Math operation\n");
            break;
    }

    char x86_addition[] = { 0x48, 0x83, 0xEC, 0x08,        // sub rsp, 8
                            0xF2, 0x0F, 0x11, 0x04, 0x24   // movsd [rsp], xmm0
    };

    LoadToX86Buffer (self, x86_buffer,   sizeof (x86_buffer));
    LoadToX86Buffer (self, x86_addition, sizeof (x86_addition));
}

void TranslatePop (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x5F }; // pop rdi

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePopReg (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x90 }; // pop r_x


    switch (cur_cmd->reg_index)
    {
    case RAX:
        *x86_buffer = 0x58;  // pop rax
        break;

    case RCX:
        *x86_buffer = 0x59;  // pop rcx
        break;

    case RDX:
        *x86_buffer = 0x5A; // pop rdx
        break;

    case RBX:
        *x86_buffer = 0x5B; // pop rbx
        break;
    
    default:
        LOG ("**No such register!**\n");
        break;
    }

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}

void TranslatePushReg (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x90 }; // push r_x

    switch (cur_cmd->reg_index)
    {
        case RAX:
            *x86_buffer = 0x50;      // push rax
            break;
        case RCX:
            *x86_buffer = 0x51;      // push rcx
            break;
        case RDX:
            *x86_buffer = 0x52;      // push rdx
            break;
        case RBX:
            *x86_buffer = 0x53;      // push rbx
            break;
    
        default:
            LOG ("**No such register!**\n");
            break;
    }

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePushImm (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x48, 0xBE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rsi, 64b number - filled down below
                          0x56                                // push rsi
                        }; 

    *(double*)(x86_buffer + 2) = cur_cmd->value;
    
    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePushImmRam (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 
                        0x49, 0x8B, 0xBA,            // mov rdi, [r10 + ?]
                        0x00, 0x00, 0x00, 0x00,     // ? - offset from begin of ram

                        0x57                        // push rdi
    };   

    *(int*)(x86_buffer + 3) = cur_cmd->value;
    
    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePopImmRam (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 
                        0x5F,                        // pop rdi
  
                        0x49, 0x89, 0xBA,            // mov [r10 + ?], rdi
                        0x00, 0x00, 0x00, 0x00       // ? - offset from begin of ram

    };   

    *(int*)(x86_buffer + 4) = cur_cmd->value;
    
    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}



void TranslatePushRegRam (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x48, 0x8B, 0x00, // mov rsi, [r_x]
                          0x56 };           // push rdi


    switch (cur_cmd->reg_index)
    {
        case RAX:
            x86_buffer[2] = 0x30;    // rax
            break;
        case RCX:
            x86_buffer[2] = 0x31;    // rcx
            break;
        case RDX:
            x86_buffer[2] = 0x32;    // rdx
            break;
        case RBX:
            x86_buffer[2] = 0x33;    // rbx
            break;
        
        default:
            LOG ("**No such register!**\n");
            break;
    }

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


int CalcVariationSum (Command* cur_cmd)
{
    return cur_cmd->is_immed + cur_cmd->use_reg * 3 + cur_cmd->use_mem * 5;
}


void TranslateJmp (TranslatorMain* self, Command* jmp_cmd)
{
    int rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 1 + sizeof (int));

    char x86_buffer[] = {0xE9, 0x00, 0x00, 0x00, 0x00}; // jmp 00 00 00 00 - rel adress x32 

    *( int* )(x86_buffer + 1) = rel_ptr;

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}



void TranslateConditionJmp (TranslatorMain* self, Command* jmp_cmd)
{
    int rel_ptr = 0;
 
    rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 2 + sizeof(int) + 5);
        
    char x86_buffer[] = {
                        0x5E,                   // pop rsi           
                        0x5F,                   // pop rdi
                        0x48, 0x39, 0xF7,       // cmp rdi, rsi
                        0x0F, 0x00,             // j?? 
                        0x00, 0x00, 0x00, 0x00  // filled in the end
    };

    // filling proper conditional jmp bytecode

    switch (jmp_cmd->name)
    {
        case JE:
            x86_buffer[6] = 0x84;   // je
            break;
        case JNE:
            x86_buffer[6] = 0x85;   // jne
            break;
        case JG:
            x86_buffer[6] = 0x8C;   // jl
            break;
        case JAE:
            x86_buffer[6] = 0x8D;   // jge
            break;
        case JGE:
            x86_buffer[6] = 0x8E;   // jle
            break;
        case JA:
            x86_buffer[6] = 0x8F;   // jg
            break;
    
        default:
            LOG ("No such conditional jmp!\n");
            break;
    }

    *( int* )(x86_buffer + 7) = rel_ptr;

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatorCtor (TranslatorMain* self)
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


void ParseOnStructs (TranslatorMain* self)
{   
    FILE* input_file = get_file (INPUT_FILE_PATH, "rb");
    ReadFileToStruct (self, input_file);
    close_file (input_file, INPUT_FILE_PATH);

    if (AllocateCmdArrays (self) == ALLOCATION_FAILURE)
        LOG ("Failed to allocate the memory for buffers");

    self->orig_ip_counter = HEADER_OFFSET;

    while (self->orig_ip_counter < self->src_cmds.len)
    {
        LOG ("Ip %3d:\n", self->orig_ip_counter);

        int flag = CmdToStruct (self->src_cmds.content + self->orig_ip_counter, self);
        
        if (flag == END_OF_PROG)
            break;   
    }

}


int CmdToStruct (const char* code, TranslatorMain* self)
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


int FillCmdInfo (const char* code, TranslatorMain* self)
{
    int name = *code & CMD_BITMASK; 
    int cmd = *code;

    Command* new_cmd = (Command*) calloc (1, sizeof (Command));

    new_cmd->orig_ip = self->orig_ip_counter;
    new_cmd->x86_ip  = self->x86_ip_counter;

    if (name == PUSH || name == POP)
    {
        new_cmd->name = (EnumCommands) name;
    
        new_cmd->reg_index = code[sizeof(elem_t) + BYTE_OFFSET];
        new_cmd->value = *(elem_t*)(code + 1);

        new_cmd->is_immed = cmd & ARG_IMMED;
        new_cmd->use_reg  = cmd & ARG_REG;
        new_cmd->use_mem  = cmd & ARG_MEM;
        
        int variation = CalcVariationSum (new_cmd);

        self->orig_ip_counter += InstrSizes[name].original_size;
        
        switch (variation)
        {
        case IMM:
            if (name == PUSH)
                self->x86_ip_counter += PUSH_IMM_SIZE; 
            else
                // self->x86_ip_counter += POP_REG_SIZE;
            break;

        case REG:
            if (name == PUSH)
                self->x86_ip_counter += PUSH_REG_SIZE; 
            else
                self->x86_ip_counter += POP_REG_SIZE;
            break;
        
        case IMM_REG:
            // self->x86_ip_counter += PUSH_REG_IMM_SIZE;
            break;

        case IMM_RAM:
            if (name == PUSH)
                self->x86_ip_counter += PUSH_IMM_RAM_SIZE; 
            else
                self->x86_ip_counter += POP_IMM_RAM_SIZE;
            break;

        case REG_RAM:
            // if (name == PUSH)
                // self->x86_ip_counter += PUSH_REG_RAM_SIZE; 
            // else
                // self->x86_ip_counter += POP_REG_RAM_SIZE;
            break;

        case IMM_REG_RAM:
            // if (name == PUSH)
                // self->x86_ip_counter += PUSH_IMM_REG_RAM_SIZE; 
            // else
                // self->x86_ip_counter += POP_IMM_REG_RAM_SIZE;
            break;

        default:
            LOG ("**No such variation of Push/Pop**\n");
            break;
        }
    }
    else
    {
        if (IsJump (name))
        {
            new_cmd->value = *(elem_t*)(code + 1);
            // LOG ("\tip to jmp:%lg \n", new_cmd->value);
        }

        new_cmd->name = (EnumCommands) name;
        
        self->x86_ip_counter  += InstrSizes[name].x86_size;
        self->orig_ip_counter += InstrSizes[name].original_size;
    }

    self->cmds_array[self->cmds_counter] = new_cmd;
    self->cmds_counter++;

    return 0;
}


bool IsJump (int cmd)
{
    if (cmd == JMP || cmd == JG || cmd == JGE || cmd == JA ||
        cmd == JAE || cmd == JE || cmd == JNE)
    {
        LOG ("\tYeah, it is jump\n");
        return true;
    }
    return false;
}


void FillJumpLables (TranslatorMain* self)
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


int FindJumpIp (TranslatorMain* self, int orig_ip)
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


int AllocateCmdArrays (TranslatorMain* self)
{
    self->cmds_array = (Command**) calloc (self->src_cmds.len, sizeof (Command*));

    self->dst_x86.content = (char*) aligned_alloc (PAGESIZE, self->src_cmds.len * sizeof (char)); // alignment for mprotect
    memset ((void*) self->dst_x86.content, 0xC3, self->src_cmds.len);      // Filling whole buffer
                                                                           // With ret (0xC3) byte code

    if (self->src_cmds.content != nullptr &&
        self->dst_x86.content  != nullptr)
        return SUCCESS;
    
    return ALLOCATION_FAILURE;
}


void DumpRawCmds (TranslatorMain* self)
{
    LOG ("\n================ Begin of struct dump ==================\n");

    for (int i = 0; i < self->cmds_counter; i++)
    {
        Command* cur_cmd = self->cmds_array[i];

        LOG ("\nCommand < %s > | Orig ip: %d | x86 ip: %d\n",
                GetNameFromId(cur_cmd->name), cur_cmd->orig_ip, cur_cmd->x86_ip);
        if (cur_cmd->name == PUSH || cur_cmd->name == POP)
        {
            if (cur_cmd->use_reg)
                LOG ("\t+Using register, its id: %d\n", cur_cmd->reg_index);
            
            if (cur_cmd->is_immed)
                LOG ("\t+Operating width digit, value: %lg\n", cur_cmd->value);
            if (cur_cmd->use_mem)
                LOG ("--- Adresses to memory\n");
        }
        if (IsJump(cur_cmd->name))
        {
            LOG ("\t+Wants to jump into %lg\n", cur_cmd->value);
        }
    }

    LOG ("\n================ End of struct dump ==================\n\n");
}


void Dump86Buffer (TranslatorMain* self)
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


void ReadFileToStruct (TranslatorMain* self, FILE* file)
{
    assert (file != nullptr);

    fseek (file, 0L, SEEK_END);
    long int file_len = ftell (file);
    fseek (file, 0L, SEEK_SET);

    char* buffer = (char*) calloc (file_len, sizeof (char));

    self->src_cmds.content = buffer;
    self->src_cmds.len = (int) fread (buffer, sizeof(char), file_len, file);

    // LOG ("Got buffer: %s\n", buffer);

    return;
}
