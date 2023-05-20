#ifndef ELF_H
#define ELF_H

#include <cstddef>
#include <stdio.h>
#include <cstring>
#include <elf.h>

#include "file_utils.h"
#include "parser.h"
#include "translation.h"

#define ElfW(type) Elf64_ ## type

void WriteInelf (TranslatorInfo* self);

void writeELFHeader (FILE* fileptr);

void writeTextSection (TranslatorInfo* self, FILE* exec_file);

void AppendBinFunc (TranslatorInfo* self, FILE* exec_file, char* bin_file_name);

int GetTextBuffer (FILE* file, char* buffer);

// =========================================

const int BIN_BUFFER_SIZE = 1000;

#define EM_AMD64 0x3E

#define BASE_ADDRESS 0x400000

#define ZERO_OFFSET 0x00

// =========================================

#endif