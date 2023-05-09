#include "../include/translator.h"

#ifdef DEBUG
    FILE* LOG_FILE = get_file ("../data/log_file.txt", "w");
#endif

const char* INPUT_FILE_PATH = "../../Processor/data/cmds.bin";
// const char* INPUT_FILE_PATH = "../data/quadratic_with_in.bin";


//  ===================================================================
// The JIT  made by
//                                     
//         88                      88              
//         88                      88              
//         88                      88              
//  ,adPPYb,88  ,adPPYba,   ,adPPYb,88  ,adPPYba,   
// a8"    `Y88 a8"     "8a a8"    `Y88 a8"     "8a  
// 8b       88 8b       d8 8b       88 8b       d8  
// "8a,   ,d88 "8a,   ,a8" "8a,   ,d88 "8a,   ,a8"  
//  `"8bbdP"Y8  `"YbbdP"'   `"8bbdP"Y8  `"YbbdP"'   
//
//  My final project in Ilya Dednisky's C programming course.
//  All my gained knowledge was extracted from my brain and thown into
//  this .cpp file
//  Please, enjoy.
//
//  Dodokek, 2023.
//  ===================================================================












