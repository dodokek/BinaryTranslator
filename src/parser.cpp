#include "../include/parser.h"

#ifdef DEBUG
    FILE* LOG_FILE = get_file ("../data/log_file.txt", "w");
#endif

// const char* INPUT_FILE_PATH = "../../Processor/data/cmds.bin";
const char* INPUT_FILE_PATH = "../ProgrammingLanguage/execution/proc/data/cmds.bin";


void TranslatorCtor (TranslatorInfo* self)
{
    self->src_cmds.content = nullptr;
    self->src_cmds.len = 0;

    self->dst_x86.content = nullptr;
    self->dst_x86.len = 0;

    self->cmds_array = nullptr;
    self->cmd_amount = 0;

    self->native_ip_counter = 0;
}



void TranslatorDtor (TranslatorInfo* self)
{
    for (int i = 0; i < self->cmd_amount; i++)
    {
        free (self->cmds_array[i]);
        self->cmds_array[i] = nullptr;
    }

    self->native_ip_counter = 0;
    
    free (self->memory_buffer);
    self->memory_buffer = nullptr;

    free (self->src_cmds.content);
    free (self->dst_x86.content);
    free (self->cmds_array);
}


int AllocateCmdArrays (TranslatorInfo* self)
{
    self->cmds_array = (Command**) calloc (self->src_cmds.len, sizeof (Command*));

    self->dst_x86.content = (char*) calloc (PAGESIZE * 2, sizeof (char)); // alignment for mprotect
    memset ((void*) self->dst_x86.content, 0xC3, self->src_cmds.len);      // Filling whole buffer
                                                                           // With ret (0xC3) byte code

    self->memory_buffer = (char*) calloc (MEMORY_SIZE, sizeof(char));
    memset ((void*) self->memory_buffer, 0xAA, MEMORY_SIZE); // filling for with 0xAA


    if (self->src_cmds.content &&
        self->dst_x86.content  &&
        self->memory_buffer)
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

    self->native_ip_counter = HEADER_OFFSET;

    int ip_counter = 0;
    while (ip_counter < self->src_cmds.len)
    {
        LOG ("Ip %3d:\n", self->native_ip_counter);

        int flag = CmdToStruct (self->src_cmds.content + self->native_ip_counter, self);
        
        if (flag == END_OF_PROG)
            break;   
        
        ip_counter++;
    }
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

        case END:
            LOG ("End of prog.\n");
            return END_OF_PROG;

        default:
            LOG ("*!**!**!**!* SIGILL %d\n *!**!**!**!*", *code);
            return END_OF_PROG;
    }

    #undef DEF_CMD

}


int FillCmdInfo (const char* code, TranslatorInfo* self)
{

    int name = *code & CMD_BITMASK; 
    int cmd = *code;

    self->native_ip_counter += InstrSizes[name].native_size;

    Command* new_cmd = (Command*) calloc (1, sizeof (Command));

    if (name == PUSH || name == POP)
    {
        FillPushPopStruct (self, new_cmd, code, cmd, name);
    }
    else
    {
        if (IsJump (name))
            new_cmd->value = *(elem_t*)(code + BYTE_OFFSET);

        new_cmd->name = (EnumCommands) name;
        
        new_cmd->x86_size    = InstrSizes[name].x86_size;
        new_cmd->native_size = InstrSizes[name].native_size;
    }

    self->cmds_array[self->cmd_amount] = new_cmd;
    self->cmd_amount++;


    return 0;
}


void FillPushPopStruct (TranslatorInfo* self, Command* new_cmd,
                        const char* code, int cmd, int name)
{
    new_cmd->name = (EnumCommands) name;
    
    new_cmd->reg_index = code[sizeof(elem_t) + BYTE_OFFSET];
    new_cmd->value = *(elem_t*)(code + BYTE_OFFSET);

    new_cmd->checksum  = 0;
    new_cmd->checksum |= (cmd & ARG_IMMED); 
    new_cmd->checksum |= (cmd & ARG_MEM); 
    new_cmd->checksum |= (cmd & ARG_REG); 
    
    new_cmd->native_size = InstrSizes[name].native_size;
    
    switch (new_cmd->checksum)
    {
    case IMM:
        if (name == PUSH)
            new_cmd->x86_size = PUSH_IMM_SIZE; 
        else
            new_cmd->x86_size = POP_REG_SIZE;
        break;

    case REG:
        if (name == PUSH)
            new_cmd->x86_size = PUSH_REG_SIZE; 
        else
            new_cmd->x86_size = POP_REG_SIZE;
        break;
    
    case IMM_RAM:
        if (name == PUSH)
            new_cmd->x86_size = PUSH_IMM_RAM_SIZE; 
        else
            new_cmd->x86_size = POP_IMM_RAM_SIZE;
        break;

    case IMM_REG_RAM:
    case REG_RAM:
        if (name == PUSH)
            new_cmd->x86_size = PUSH_REG_RAM_SIZE; 
        else
            new_cmd->x86_size = POP_REG_RAM_SIZE;
        break;


    default:
        LOG ("**1: No such variation of Push/Pop** %d\n", new_cmd->checksum);
        break;
    }
}



void FillCmdIp (TranslatorInfo* self)
{
    int native_ip = HEADER_OFFSET;
    int x86_ip = 0;

    for (int i = 0; i < self->cmd_amount; i++)
    {
        if (self->cmds_array[i]->is_skip) continue;

        self->cmds_array[i]->native_ip = native_ip;
        self->cmds_array[i]->x86_ip    = x86_ip;

        native_ip += self->cmds_array[i]->native_size;
        x86_ip    += self->cmds_array[i]->x86_size;

    }
    self->x86_ip_counter = x86_ip;

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

    for (int i = 0; i < self->cmd_amount; i++)
    {
        if (IsJump(self->cmds_array[i]->name))
        {
            LOG ("%2d: Trying to find x86 ip for ip %lg:\n", i, self->cmds_array[i]->value);

            self->cmds_array[i]->value = FindJumpIp (self, self->cmds_array[i]->value);
        }
    }

    LOG ("\n\n---------- End filling labels --------\n\n");
}


int FindJumpIp (TranslatorInfo* self, int native_ip)
{
    for (int i = 0; i < self->cmd_amount; i++)
    {
        if (self->cmds_array[i]->native_ip == native_ip)
        {
            LOG ("\t Found, x86 ip is %d\n", self->cmds_array[i]->x86_ip)
            return self->cmds_array[i]->x86_ip;
        }
    }

    LOG ("\tNot found\n");

    return -1;
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

    for (int i = 0; i < self->cmd_amount; i++)
    {
        Command* cur_cmd = self->cmds_array[i];
        if (cur_cmd->is_skip) continue;


        LOG ("\nCommand < %s: %d > | Native/x86 size: %d/%d | x86 ip: %d\n",
                GetNameFromId(cur_cmd->name), cur_cmd->name, cur_cmd->native_size,
                cur_cmd->x86_size, cur_cmd->x86_ip);
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
            LOG ("Not standart instrucion.");
            return nullptr;
    }

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


void LogData(char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    
    vfprintf(LOG_FILE, format, argptr);
    
    va_end(argptr);
}


void mprotect_change_rights (TranslatorInfo* self, int protect_status)
{
    int mprotect_status = mprotect (self->dst_x86.content, self->dst_x86.len + 1, protect_status);
    if (mprotect_status == -1)
    {
        printf ("Mrotect error\n");
        return;
    }
}



