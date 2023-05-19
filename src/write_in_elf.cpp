#include "../include/write_in_elf.h"


void WriteInelf (TranslatorInfo* self)
{
    FILE* exec_file = get_file ("execute.elf", "wb");
    writeELFHeader(exec_file);

    writeTextSection (self, exec_file);

    AppendBinFunc (self, exec_file, "../data/printf.bin");
    AppendBinFunc (self, exec_file, "../data/scanf.bin");

    fwrite(self->dst_x86.content, sizeof (unsigned char), PAGESIZE * 2, exec_file);
    
    close_file (exec_file, "execute.elf");
}


void writeTextSection (TranslatorInfo* self, FILE* exec_file)
{
    ElfW(Phdr) textSection = {};

    textSection.p_type = PT_LOAD;
    textSection.p_flags = PROT_EXEC + PROT_READ + PROT_WRITE; 
    textSection.p_offset = 0x78;
    textSection.p_vaddr = 0x400078;
    textSection.p_filesz = MEMORY_SIZE +  self->dst_x86.len;
    textSection.p_memsz =  MEMORY_SIZE +  self->dst_x86.len;
    textSection.p_align = 0x1000;

    fwrite(&textSection, sizeof (textSection), 1, exec_file);
}


void writeELFHeader (FILE* exec_file)
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

    fwrite(&header, sizeof (header), 1, exec_file);
}


void AppendBinFunc (TranslatorInfo* self, FILE* exec_file, char* bin_file_name)
{
    FILE* printf_file = get_file (bin_file_name, "rb");
    char* buffer = (char*) calloc (1000, sizeof(char));
    assert (buffer != nullptr);
    
    int file_binsize = GetTextBuffer (printf_file, buffer);
    
    memcpy (self->dst_x86.content + self->dst_x86.len, buffer, file_binsize);
    self->dst_x86.len += file_binsize;

    free (buffer);
    close_file (printf_file, bin_file_name);
}


int GetTextBuffer (FILE* file, char* buffer)
{
    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    return fread (buffer, sizeof(char), size, file);
}
