

#include "../include/translator.h"

FILE* LOG_FILE = get_file ("log_file.txt", "w");


int main()
{
    TranslatorMain TranslatorInfo = {};
    TranslatorCtor (&TranslatorInfo);

    ParseOnStructs (&TranslatorInfo);

    DumpRawCmds (&TranslatorInfo);

    StartTranslation (&TranslatorInfo);

    Dump86Buffer (&TranslatorInfo);

    RunCode (&TranslatorInfo);
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
            TranslatePushPop (self, self->cmds_array[cmd_indx]);
            break;

        case JMP:
            TranslateJmp (self, self->cmds_array[cmd_indx]);
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



void TranslatePushPop (TranslatorMain* self, Command* cur_cmd)
{
    // For now let's just write xor rax, rax; mov rax, 2d

    char opcode_buff[] = { 0x48, 0x31, 0xC0, 0xB8, 0x02, 0x00, 0x00, 0x00 }; // mov rax, 02d



    LoadToX86Buffer (self, opcode_buff, sizeof (opcode_buff));
}


void TranslateJmp (TranslatorMain* self, Command* cur_cmd)
{
    // Need to add cool calculations of jump
}



void TranslatorCtor (TranslatorMain* self)
{
    self->src_cmds.content = nullptr;
    self->src_cmds.len = 0;

    self->dst_x86.content = nullptr;
    self->dst_x86.len = 0;

    self->cmds_array = nullptr;
    self->cmds_counter = 0;

    self->jump_table = (int*) calloc (MAX_IP, sizeof (int));
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

    #define DEF_CMD(name, OFFSET, id)    \
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

    if (name == PUSH || name == POP)
    {
        new_cmd->name = (EnumCommands) name;
    
        new_cmd->reg_index = code[sizeof(elem_t) + BYTE_OFFSET];
        new_cmd->value = *(elem_t*)(code + 1);

        new_cmd->is_immed = cmd & ARG_IMMED;
        new_cmd->use_reg  = cmd & ARG_REG;
        new_cmd->use_mem  = cmd & ARG_MEM;
        // Here calculating offset for push/pop

        new_cmd->name = (EnumCommands) name;
    }
    else
    {
        new_cmd->name = (EnumCommands) name;
    }
    // printf ("Adding %d\n", InstrSizes[name].original_size);
    
    self->jump_table[self->orig_ip_counter] = self->x86_ip_counter;
    
    self->x86_ip_counter  += InstrSizes[name].x86_size;
    self->orig_ip_counter += InstrSizes[name].original_size;

    self->cmds_array[self->cmds_counter] = new_cmd;
    self->cmds_counter++;

    return 0;
}


int AllocateCmdArrays (TranslatorMain* self)
{
    self->cmds_array = (Command**) calloc (self->src_cmds.len, sizeof (Command*));

    self->dst_x86.content = (char*) aligned_alloc (PAGESIZE, self->src_cmds.len * sizeof (char)); // alignment for mprotect
    memset ((void*) self->dst_x86.content, 0xC3, self->src_cmds.len);      // Filling all buffer
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

        LOG ("\nCommand. Id: %d, Orig ip: %d, x86 ip: %d\n",
                cur_cmd->name, cur_cmd->orig_ip, self->jump_table[cur_cmd->orig_ip]);
        if (cur_cmd->name == PUSH || cur_cmd->name == POP)
        {
            if (cur_cmd->use_reg)
                LOG ("\tUsing register, its id: %d\n", cur_cmd->reg_index);
            
            if (cur_cmd->is_immed)
                LOG ("\tOperating width digit, value: %lg\n", cur_cmd->value);
            if (cur_cmd->use_mem)
                LOG ("--- Adresses to memory\n");
        }
    }

    LOG ("\n================ End of struct dump ==================\n\n");
}


void Dump86Buffer (TranslatorMain* self)
{
    LOG ("\n\n====== x86 buffer dump begin =======\n");

    for (int i = 0; i < self->dst_x86.len + 1; i++)
    {
        if (i % 10 == 0) LOG ("\n%d: ", i);

        LOG ("%02x ", (unsigned char) self->dst_x86.content[i]);
    }

    LOG ("\n\n====== x86 buffer dump end =======\n\n");

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