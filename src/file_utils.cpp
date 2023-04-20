
#include "file_utils.h"
//-----------------------------------------------------------------------------

FILE* get_file (const char file_name[], const char* mode)
{
    FILE* tmp_file = fopen(file_name, mode);

    if (tmp_file)
    {
        printf("File name is %s\n", file_name);
        return tmp_file;
    }

    printf("Failed to open the file %s.\n", file_name);

    return nullptr;
}


int read_file (FILE* file, char** buffer)
{
    assert (file != nullptr);

    fseek (file, 0L, SEEK_END);
    long int file_len = ftell (file);
    fseek (file, 0L, SEEK_SET);

    return (int) fread (*buffer, sizeof(char), file_len, file);
}



FILE* close_file (FILE* file, const char* name)
{
    assert (file != nullptr);

    printf ("Succesfully closing the file: %s\n", name);

    fclose (file);

    return 0;
}
