
#include "../include/translator.h"

FILE* LOG_FILE = get_file ("log_file.txt", "w");


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

    // RunCode (&TranslatorInfo);
}


void RunCode (TranslatorMain* self)
{
    int flag = mprotect (self->dst_x86.content, self->dst_x86.len + 1, PROT_EXEC);

    if (flag == -1)
    {
        LOG ("**** mprotect error ****\n");
        return;
    }

    int (* god_save_me)(void) = (int (*)(void))(self->dst_x86.content);

    int kek = god_save_me();

    printf ("Bruh: %x\n", kek);
}


void StartTranslation (TranslatorMain* self)
{
    LOG ("---------- Begin translation -------------\n");

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
        
        default:
            break;
        }
    }

}


void LoadToX86Buffer (TranslatorMain* self, char* op_code, size_t len)
{
    memcpy (self->dst_x86.content + self->dst_x86.len, (void*) op_code, len);
    self->dst_x86.len += len;
}


void HandlePushPopVariation (TranslatorMain* self, Command* cur_cmd)
{
    // char opcode_buff[] = { 0x48, 0x31, 0xC0, 0xB8, 0x02, 0x00, 0x00, 0x00 }; // mov rax, 02d
    int variation = CalcVariationSum (cur_cmd);

    switch (variation)
    {
    case VOID:
        TranslatePop (self, cur_cmd);
        break;

    case IMM:
        /* code */
        break;

    case REG:
        TranslatePopReg(self, cur_cmd);
        break;
    
    case IMM_REG:
        /* code */
        break;

    case IMM_RAM:
        /* code */
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

    // LoadToX86Buffer (self, opcode_buff, sizeof (opcode_buff));
}


void TranslatePop (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[] = { 0x5f }; // pop rdi

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslatePopReg (TranslatorMain* self, Command* cur_cmd)
{
    char x86_buffer[1] = { 0x90 }; // pop rax/rbx/rcx/rdx


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


int CalcVariationSum (Command* cur_cmd)
{
    return cur_cmd->is_immed + cur_cmd->use_reg * 3 + cur_cmd->use_mem * 5;
}


void TranslateJmp (TranslatorMain* self, Command* jmp_cmd)
{
    int rel_ptr = 0;
    if (jmp_cmd->value >= jmp_cmd->x86_ip)
        rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 5);
    else
        rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip) - 5;


    char x86_buffer[] = {0xE9, 0x00, 0x00, 0x00, 0x00}; // jmp 00 00 00 00 - rel adress x32 

    *( int* )(x86_buffer + 1) = rel_ptr;

    LoadToX86Buffer (self, x86_buffer, sizeof (x86_buffer));
}


void TranslateConditionJmp (TranslatorMain* self, Command* jmp_cmd)
{
    int rel_ptr = 0;
    if (jmp_cmd->value >= jmp_cmd->x86_ip)
        rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip + 5);
    else
        rel_ptr = jmp_cmd->value - (jmp_cmd->x86_ip) - 6;



    // Need to add cool calculations of jump
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

    #define DEF_CMD(name, id)    \
        case name:                       \
            LOG ("\tId: %d\n", name);     \
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
            /* code */
            break;

        case REG:
            self->x86_ip_counter  += POP_REG_SIZE;
            break;
        
        case IMM_REG:
            /* code */
            break;

        case IMM_RAM:
            /* code */
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
        if (i % 10 == 0) LOG ("\n%2d: ", i);

        LOG ("%02x ", (unsigned char) self->dst_x86.content[i]);
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