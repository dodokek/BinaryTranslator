# Binary translator

## Overview
In this project I combined all my skills, that I gained from educating on the 1st course of MIPT. 

## About

I will translate binary code, generated from my own <a href="https://github.com/dodokek/ProgrammingLanguage">Programming language</a> into **x86-64** machine code. Program will walk through the executable and translate each instruction into the corresponding instruction for x86-64 architecture. My translator translates the instructions and puts them in a buffer that is allocated in the C program. Further with the help of mprotect the buffer becomes executable, and code injection occurs.

## Build

Step 1: Clone the repo on your PC.

~~~
git clone git@github.com:dodokek/BinaryTranslator.git
~~~

Step 2: Build the project using Makefile

~~~
~BinaryTranslator cd src
~BinaryTranslator/src make
~~~






## Translation table

Here is the translation table from *Native assembly* to *x86-64 assembly*


<details>
<summary>Translation of Push/Pop variations</summary>

|   Native       | x86-64      |  
| ------         | :---------------: | 
| Push Num       | mov rsi, 0x1ff0...00 (double) <br> push rsi                |  
| Push reg       | push r_x              |
| Push [ Num ]      | mov rdi, [r10 + Num] <br> push rdi              |
| Push [ r_x ]       | mov [r10 + r_x]       |
| Push [ Num + r_x ]       | add r10, r_x <br> mov rdi, [r10 + Num] <br> sub r10, r_x <br> push rdi|
| Pop r_x        | pop r_x              |
| Pop [ r_x ]       | pop rdi <br> mov [r10 + r_x], rdi              |
| Pop [Num + r_x]       | pop rdi <br> add r10, r_x <br> mov [r10 + Num], rdi <br> sub r10, r_x           |

> **r10** register is used as pointer to begin of the memory.
</details>


<details>
<summary>Translation of Arithmetics</summary>

|   Native       | x86-64      |  
| ------         | :---------------: | 
| Add        |  addpd xmm0, xmm1       |
| Sub        |  subpd xmm0, xmm1       |
| Mul        |  mulpd xmm0, xmm1       |
| Div        |  divpd xmm0, xmm1       |
| Sqr        |  sqrtpd xmm0, xmm1      |
> This operations are followed with moving values from stack to xmm1, xmm0 registers.
</details>

<details>
<summary>Translation of OUT</summary>
In native assembly <b>OUT</b> command pops the value from stack and prints it on the screen using <b>printf from STL</b>. 
<br>
<br>
In x86-64 I will call printf function to print one double number using the same printf from STL. The only difference - the stack should be aligned and data stored in specific registers

~~~C++
mov xmm0, [rsp]

push r10
pusha

mov rbp, rsp
and rsp, -16 // aligning stack

call printf

mov rsp, rbp

popa
pop r10

~~~

</details>


<details>
<summary>Jumps, function calls</summary>

Jump, Call, Ret are translated straightforward:
|   Native       | x86-64      |  
| ------         | :---------------: | 
| Jmp <32b rel. ptr.>        |  jmp <32b rel. ptr.>       |
| Call <32b rel. ptr.>        |  call <32b rel. ptr.>       |
| Ret        |  ret       |

Conditional Jumps are also translated the same, but in Native assembly we also need to compare two values from stack. Comparison part:

~~~C++
mov xmm0, [rsp]
mov xmm1, [rsp+8]
add rsp, 16

ucomisd xmm0, xmm1
~~~

The rest translation of conditional jumps is the same with non-conditional jump.

</details>

## Optimizations

Before translation, binary code is transformed into *IR*. Thanks to that, some instrucions might be optimized.

### Immediate's storage optimization
In native Assembly representation we can see a lot of instruction series like this:

~~~C++
push 123
pop rax
~~~

This obviously can be transformed into:

~~~
mov rax, 123
~~~

According to translation table, *x86-64* will be *2 bytes shorter*. The bigger source file would be - the greater performance boost we'll get.

### Arithmetics optimizations
Any structures like these:

~~~C++
push 1  |
push 2  |----> push 1+2
add     |
~~~

The same optimization is applied on *sub, mul and div* instructions.


## Performance test
Using <a href="https://githrsiub.com/dodokek/ProgrammingLanguage">my assembly work implementation</a> I wrote program, wich finds factorial. 

I executed it with my CPU work implementation on C. After that I executed it using my Binary translator.

Let's see the improvement in speed:

|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (mcr. s) | 810       | 36                 |  
| Relative boost | 1       | 22.5                 |  

> Program was given $5!$ to calculate 1000 times 


Let's also test solving of Quadratic equation:


|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (mcr. s) | 3375       | 59                 |  
| Relative boost | 1       | 57.2                 |  

> Program was given equation  $x^2 + 4x + 3 = 0$ to solve 1000 times

You can find source code of all Assembly programs <a href = "https://github.com/dodokek/Processor/tree/origin/examples">here</a>.


We can see the huge improvement in speed, JIT compilation rules!


## Conclusion

I learned a lot about Binary translators, IR and x86-64 architecture. In future I want to improve IR, so it will have real CFG. A lot of new optimizations might be done with the help of it. 

<br> <br> All this journey from simple Quadratic equation to Binary translator was... pretty good. 


## Useful links


This one really helped me to understand some important concepts of binary translators: <br>
https://dse.in.tum.de/wp-content/uploads/2022/01/translating_x86_binaries_into_llvm_intermediate_representation.pdf  
  
  
Information, that helped me in some parts of project: <br>
https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170#x64-register-usage

https://www.felixcloutier.com/x86/divsd

https://gist.github.com/williballenthin/6857590dab3e2a6559d7

