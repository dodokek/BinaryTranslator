#ifndef TRANS_H
#define TRANS_H

#include "parser.h"

void StartTranslation (TranslatorInfo* self);

void WriteDoubleNum (TranslatorInfo* self, double value);

void WriteCmd (TranslatorInfo* self, Opcode cmd);

void WritePtr (TranslatorInfo* self, uint32_t ptr);

void WriteAbsPtr (TranslatorInfo* self, uint64_t ptr);

void RunCode (TranslatorInfo* self);

//-- Translation Units:

void TranslateIn (TranslatorInfo* self, Command* cur_cmd);

void TranslateSqr (TranslatorInfo* self, Command* cur_cmd);

void RegToRsiOffset(TranslatorInfo* self, Command* cur_cmd);

void TranslatePushImmRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopImmRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushImmRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopImmRam (TranslatorInfo* self, Command* cur_cmd);

void TranslateOut (TranslatorInfo* self, Command* cur_cmd);

void TranslateBaseMath (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushImm (TranslatorInfo* self, Command* cur_cmd);

int CalcVariationSum (Command* cur_cmd);

void LoadToX86Buffer (TranslatorInfo* self, char* op_code, size_t len);

void HandlePushPopVariation (TranslatorInfo* self, Command* cur_cmd);

void TranslatePop (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopReg (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushReg (TranslatorInfo* self, Command* cur_cmd);

void TranslatePushRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslatePopRegRam (TranslatorInfo* self, Command* cur_cmd);

void TranslateJmpCall (TranslatorInfo* self, Command* cur_cmd);

void TranslateRet (TranslatorInfo* self, Command* jmp_cmd);

void TranslateConditionJmp (TranslatorInfo* self, Command* jmp_cmd);


#endif
