#ifndef ELF_H
#define ELF_H

#include <cstddef>
#include <stdio.h>
#include <cstring>
#include <elf.h>

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

#include "parser.h"


void WriteInelf (TranslatorInfo* self);

void writeELFHeader (FILE* fileptr);


#endif