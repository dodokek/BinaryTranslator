

#include "../include/translator.h"


int main()
{
    TranslatorMain TranslatorInfo = {};
    TranslatorCtor (&TranslatorInfo);

    InitializeTranslation (&TranslatorInfo);
 
    DumpRawCmds (&TranslatorInfo);
}


void TranslatorCtor (TranslatorMain* self)
{
    self->src_cmds.content = nullptr;
    self->src_cmds.len = 0;

    self->dst_x86.content = nullptr;
    self->dst_x86.len = 0;

    self->cmds_array = nullptr;
    self->cmds_counter = 0;
}


void InitializeTranslation (TranslatorMain* self)
{   
    FILE* input_file = get_file (INPUT_FILE_PATH, "rb");
    ReadFileToStruct (self, input_file);

    if (AllocateCmdArrays (self) == ALLOCATION_FAILURE)
        LOG ("Failed to allocate the memory for buffers");

    for (int ip = HEADER_OFFSET; ip < self->src_cmds.len; ip++)
    {
        printf ("Ip %3d:\n ", ip);

        int offset_or_hlt = CmdToStruct (self->src_cmds.content + ip, self);
        
        if (offset_or_hlt == END_OF_PROG)
            break;
        
        ip += offset_or_hlt;   
    }

    close_file (input_file, INPUT_FILE_PATH);
}


int CmdToStruct (const char* code, TranslatorMain* self)
{

    #define DEF_CMD(name, OFFSET, id)    \
        case name:                       \
            printf ("\t%d\n", name);     \
            FillCmdInfo (code, self);    \
            return OFFSET;               \

    switch (*code & CMD_BITMASK)
    {
        #include "../codegen/cmds.h"    // generating cases for all cmds

        case HLT:
            LOG ("End of prog.\n");
            return END_OF_PROG;

        default:
            printf ("SIGILL %d\n", *code);
            return END_OF_PROG;
    }

    #undef DEF_CMD

}


void FillCmdInfo (const char* code, TranslatorMain* self)
{
    int name = *code & CMD_BITMASK; 
    int cmd = *code;

    Command* new_cmd = (Command*) calloc (1, sizeof (Command));

    if (name == PUSH || name == POP)
    {
        new_cmd->name = (EnumCommands) name;
    
        new_cmd->reg_index = code[sizeof(elem_t) + BYTE_OFFSET];
        new_cmd->value = *(elem_t*)(code + 1);

        new_cmd->is_immed = cmd & ARG_IMMED;
        new_cmd->use_reg  = cmd & ARG_REG;
        new_cmd->use_mem  = cmd & ARG_MEM;
    }
    else
    {
        new_cmd->name = (EnumCommands) name;
    }
    
    self->cmds_array[self->cmds_counter] = new_cmd;

    self->cmds_counter++;

    return;
}


int AllocateCmdArrays (TranslatorMain* self)
{
    self->cmds_array = (Command**) calloc (self->src_cmds.len, sizeof (Command*));

    self->dst_x86.content = (char*) calloc (self->src_cmds.len, sizeof (char));

    if (self->src_cmds.content != nullptr &&
        self->dst_x86.content  != nullptr)
        return SUCCESS;
    
    return ALLOCATION_FAILURE;
}


void DumpRawCmds (TranslatorMain* self)
{
    for (int i = 0; i < self->cmds_counter; i++)
    {
        Command* cur_cmd = self->cmds_array[i];


        printf ("Command. Id: %d\n", cur_cmd->name);
        if (cur_cmd->name == PUSH || cur_cmd->name == POP)
        {
            if (cur_cmd->use_reg)
                printf ("\tUsing register, its id: %d\n", cur_cmd->reg_index);
            
            if (cur_cmd->is_immed)
                printf ("\tOperating width digit, value: %lg\n", cur_cmd->value);
            if (cur_cmd->use_mem)
                printf ("--- Adresses to memory\n");
        }
    }

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