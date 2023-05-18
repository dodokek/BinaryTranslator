#include "../include/translation.h"
#include "../include/parser.h"
#include "../include/optimizer.h"
#include "../include/write_in_elf.h"



int main()
{
    TranslatorInfo TranslatorInfo = {};
    TranslatorCtor (&TranslatorInfo);

    ParseOnStructs (&TranslatorInfo);

    DumpRawCmds (&TranslatorInfo);
    OptimizeCmdFlow (&TranslatorInfo);

    FillCmdIp (&TranslatorInfo);
    FillJumpLables (&TranslatorInfo);
    DumpRawCmds (&TranslatorInfo);

    StartTranslation (&TranslatorInfo);

    Dump86Buffer (&TranslatorInfo);

    // RunCode (&TranslatorInfo);

    WriteInelf (&TranslatorInfo);

    TranslatorDtor (&TranslatorInfo);
}


void RunCode (TranslatorInfo* self)
{
    int mprotect_status = mprotect (self->dst_x86.content, self->dst_x86.len + 1, PROT_EXEC | PROT_READ | PROT_WRITE);
    
    if (mprotect_status == -1)
        return;

    mprotect_change_rights (self, PROT_EXEC | PROT_READ | PROT_WRITE);

    void (* god_save_me)(void) = (void (*)(void))(self->dst_x86.content);
    assert (god_save_me != nullptr);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    // for (int i = 0; i < 1000; i++)
        god_save_me();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    printf ("Elapsed time(mcr. s): %lu\n", std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count());

    mprotect_change_rights (self, PROT_READ | PROT_WRITE);
}

