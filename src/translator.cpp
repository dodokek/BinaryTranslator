

#include "../include/translator.h"


int main()
{
    TranslatorMain TranslatorInfo = {};
    
    InitializeTranslation (&TranslatorInfo);
 
}

void InitializeTranslation (TranslatorMain* self)
{   
    FILE* input_file = get_file (INPUT_FILE_PATH, "rb");
    ReadFileToStruct (self, input_file);

    for (int ip = HEADER_OFFSET; ip < self->src_cmds.len; ip++)
    {
        printf ("Ip %3d:\n ", ip);

        int offset_or_hlt = CmdToStruct (self->src_cmds.content + ip);
        
        if (offset_or_hlt == END_OF_PROG)
            break;
        
        ip += offset_or_hlt;   
    }
}



int CmdToStruct (const char* code)
{

    #define DEF_CMD(name, OFFSET, id)    \
        case name:                       \
            printf ("\t%d\n", name);     \
            return OFFSET;               \
            break;              

    switch (*code & CMD_BITMASK)
    {
        #include "../codegen/cmds.h"

        case HLT:
            LOG ("End of prog.\n");
            return END_OF_PROG;

        default:
            printf ("SIGILL %d\n", *code);
            return END_OF_PROG;
    }

    #undef DEF_CMD

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

// void FillPushPopData (int cmd, const char* code)          
// {
//     // elem_t* arg_ptr = 0;
//     // int reg_indx = code[sizeof(elem_t) + BYTE_OFFSET];

//     // if (cmd & ARG_IMMED) *val = *(elem_t*)(code + 1);
    
//     // if ((cmd & ARG_REG) && (cmd & ARG_IMMED))
//     // {
//     //     *val += CpuInfo->Regs[reg_indx]; 
//     // }

//     // if ((cmd & ARG_REG) && !(cmd & ARG_IMMED))
//     // {
//     //     arg_ptr = CpuInfo->Regs + reg_indx; 
//     //     *val    = CpuInfo->Regs[reg_indx];
//     // }

//     // if (cmd & ARG_MEM) arg_ptr = CpuInfo->Ram + int(*val);

//     // return arg_ptr;
// }