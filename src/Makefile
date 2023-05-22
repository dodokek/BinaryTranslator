ifeq ($(OS),Windows_NT)
	OUTPUT_FILE_NAME = main.exe
else
    OUTPUT_FILE_NAME = main
endif

CC = g++
# CFLAGS = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -D_DEBUG -D_EJUDGE_CLIENT_SIDE
CFLAGS = -Wshadow -Wno-narrowing -fno-stack-protector

IFLAGS = -I./../include/ 

#main

SRC_DIR = ./
OBJ_DIR = ./lib/

SRC   = file_utils.cpp translation.cpp parser.cpp optimizer.cpp write_in_elf.cpp main.cpp
OBJ   = $(patsubst %.cpp, $(OBJ_DIR)%.o, $(SRC)) 

ASM_SRC = ../data/asm_sources/

#linking
all : $(OBJ)
	$(CC) $(IFLAGS) $(CFLAGS) $^ -no-pie -o $(OUTPUT_FILE_NAME)

#main
$(OBJ_DIR)%.o : $(SRC_DIR)%.cpp
	mkdir -p $(@D)
	$(CC) $(IFLAGS) $(CFLAGS) -c $^ -o $@

run:
	./main

scanf: $(ASM_SRC)scanf.s
	nasm -f elf64 -g $(ASM_SRC)scanf.s -o $(ASM_SRC)scanf.o
	ld $(ASM_SRC)scanf.o -o $(ASM_SRC)scanf

printf: $(ASM_SRC)printf.s
	nasm -f elf64 -g $(ASM_SRC)printf.s -o $(ASM_SRC)printf.o
	ld $(ASM_SRC)printf.o -o $(ASM_SRC)printf

clear: 
	rm -f $(OBJ_DIR)*.o