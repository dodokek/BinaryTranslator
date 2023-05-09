#include "../include/optimizer.h"

void OptimizeCmdFlow (TranslatorInfo* self)
{
    for (int i = 0; i < self->cmd_amount - 1; i++)
    {
        if (self->cmds_array[i]->checksum == IMM && 
            self->cmds_array[i]->name     == PUSH)
        {    
            LOG ("Optimizing to mov r?x, num\n");
            HandleImmStorage (self, i);
        }
        
    }
}


void HandleImmStorage (TranslatorInfo* self, int cmd_indx)
{
    if (self->cmds_array[cmd_indx + 1]->name     == POP && 
        self->cmds_array[cmd_indx + 1]->checksum == REG)
    {
        OptimizeMovRegNum (self, *(self->cmds_array + cmd_indx + 1),
                           self->cmds_array[cmd_indx]->value);

        self->cmds_array[cmd_indx]->is_skip = true;
        
        self->cmds_array[cmd_indx + 1]->native_size += self->cmds_array[cmd_indx]->native_size; 
    }


}


void OptimizeMovRegNum (TranslatorInfo* self, Command* cur_cmd, double num_to_mov)
{
    // push num | ---> mov r?x, num
    // pop r?x  |
    
    cur_cmd->name = MOV_REG_NUM_CMD;                          
    cur_cmd->x86_size = SIZE_MOV_REG_NUM + sizeof (double);   
    cur_cmd->value = num_to_mov;
}
 

