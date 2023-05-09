#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "parser.h"
#include "translation.h"

void OptimizeCmdFlow (TranslatorInfo* self);

void HandleConstantCalculation (TranslatorInfo* self, int cmd_indx);

void HandleImmStorage (TranslatorInfo* self, int cmd_indx);

void OptimizeMovRegNum (TranslatorInfo* self, Command* cur_cmd, double num_to_mov);

#endif
