#include "../include/write_in_elf.h"

const char ELF_FILENAME[] = "execute.elf";

void WriteInelf (TranslatorInfo* self)
{
    FILE* exec_file = get_file (ELF_FILENAME, "wb");

    writeELFHeader(exec_file);

    writeTextSection (self, exec_file);

    AppendBinFunc (self, exec_file, "../data/printf.bin");
    AppendBinFunc (self, exec_file, "../data/scanf.bin");

    fwrite(self->dst_x86.content, sizeof (unsigned char), PAGESIZE * 2, exec_file);
    
    close_file (exec_file, ELF_FILENAME);
}


void writeTextSection (TranslatorInfo* self, FILE* exec_file)
{
    ElfW(Phdr) textSection = {};

    textSection.p_type = PT_LOAD;
    textSection.p_flags = PROT_EXEC + PROT_READ + PROT_WRITE; 
    textSection.p_offset = 0x00;
    textSection.p_vaddr = BASE_ADDRESS;
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
    header.e_ident[EI_CLASS]   = ELFCLASS64;
    header.e_ident[EI_DATA]    = ELFDATA2LSB;
    header.e_ident[EI_OSABI]   = 0x00;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_version           = EV_CURRENT;
    header.e_type              = ET_EXEC;
    header.e_machine           = X86_MACHINE;  
    header.e_entry             = BASE_ADDRESS + sizeof (Elf64_Phdr) + sizeof (Elf64_Ehdr);
    header.e_phoff             = sizeof (Elf64_Ehdr);
    header.e_shoff             = 0x0; // Section header table's file offset in bytes
    header.e_ehsize            = sizeof (Elf64_Ehdr);
    header.e_phentsize         = sizeof(Elf64_Phdr);
    header.e_phnum             = 0x01; // The product of e_phentsize and e_phnum gives the table's size in bytes

    fwrite(&header, sizeof (header), 1, exec_file);
}


void AppendBinFunc (TranslatorInfo* self, FILE* exec_file, char* bin_file_name)
{
    FILE* printf_file = get_file (bin_file_name, "rb");
    char* buffer      = (char*) calloc (BIN_BUFFER_SIZE, sizeof(char));
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
