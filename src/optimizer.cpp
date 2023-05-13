#include "../include/optimizer.h"

void OptimizeCmdFlow (TranslatorInfo* self)
{
    for (int i = 0; i < self->cmd_amount - 1; i++)
    {
        Command* cur_cmd = self->cmds_array[i]; 


        if (cur_cmd->checksum == IMM && 
            cur_cmd->name     == PUSH)
        {    
            LOG ("Optimization attempt: mov r?x, num\n");
            HandleImmStorage (self, i);
        }

        if (cur_cmd->name == ADD || cur_cmd->name == SUB ||
            cur_cmd->name == MUL || cur_cmd->name == DIV)
        {
            LOG ("Optimization attempt: constant arithmetics\n");

            HandleConstantCalculation (self, i);
        }
        
    }
}


void HandleConstantCalculation (TranslatorInfo* self, int cmd_indx)
{
    Command* operation = self->cmds_array[cmd_indx]; 
    Command* member1   = self->cmds_array[cmd_indx - 1];
    Command* member2   = self->cmds_array[cmd_indx - 2];

    if (member1->checksum == IMM && member2->checksum == IMM)
    {
        int result_value = 0;

        switch (operation->name)
        {
        case ADD:
            result_value = member1->value + member2->value;
            break;

        case SUB:
            result_value = member1->value - member2->value;
            break;

        case MUL:
            result_value = member1->value * member2->value;
            break;

        case DIV:
            result_value = member1->value / member2->value;
            break;
        
        default:
            LOG ("No such operation for optimization\n");
            break;
        }

        // push 1          |
        // push 2          | ---- > push 3
        // add             |

        operation->value = result_value;
        operation->native_size += member1->native_size + member2->native_size;
        operation->name = PUSH;
        operation->checksum = IMM;
        operation->x86_size = PUSH_IMM_SIZE;

        member1->is_skip = true;
        member2->is_skip = true;

        LOG ("\t Arithmetic optimization success: indx %d\n", cmd_indx);
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
        
        LOG ("\t Storage optimization success: indx %d\n", cmd_indx);
        
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
 

