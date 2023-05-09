#include "../include/write_in_elf.h"

void writeELFHeader (FILE* fileptr)
{
    ElfW(Ehdr) header = {};
    header.e_ident[EI_MAG0]  = 0x7f;
    header.e_ident[EI_MAG1]  = 'E';
    header.e_ident[EI_MAG2]  = 'L';
    header.e_ident[EI_MAG3]  = 'F';
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA]  = ELFDATA2LSB;
    header.e_ident[EI_OSABI] = 0x00;
    header.e_ident[EI_VERSION] = 0x01;
    header.e_version           = 0x01;
    header.e_type            = ET_EXEC;
    header.e_machine         = 0x3E;
    header.e_entry           = 0x400078;
    header.e_phoff           = 0x40;
    header.e_shoff           = 0x0;
    header.e_ehsize          = 0x40;
    header.e_phentsize       = 0x38;
    header.e_phnum           = 0x01;

    fwrite(&header, sizeof (header), 1, fileptr);
}

void WriteInelf (TranslatorInfo* self)
{
    FILE* fileptr = fopen ("kek", "wb");
    writeELFHeader(fileptr);

    ElfW(Phdr) textSection = {};

    textSection.p_type = 0x01;
    textSection.p_flags = 0x07;
    textSection.p_offset = 0x78;
    textSection.p_vaddr = 0x400078;
    textSection.p_filesz = 0x7;
    textSection.p_memsz = 0x7;
    textSection.p_align = 0x1000;

    fwrite(&textSection, sizeof (textSection), 1, fileptr);

    unsigned char code[] = {0x6a, 0x3c, 0x58, 0x31, 0xff, 0x0f, 0x05, 0x00};
    fwrite(code, sizeof (unsigned char), 8, fileptr);
}