# JIT compilator 

## Overview
In this project I combined all my skills, that I gained from educating on the 1st course of MIPT. I will translate binary code, generated from my own <a href="https://github.com/dodokek/ProgrammingLanguage">Programming language</a> into **x86-64** machine code.

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


## Performance test
Using <a href="https://githrsiub.com/dodokek/ProgrammingLanguage">my assembly work implementation</a> I wrote program, wich finds factorial 1000 times. 

After that I executed it with my CPU work implementation on C, then executed it using my JIT compilator.

Let's see the improvement in speed:

|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (mcr. s) | 810       | 36                 |  
| Relative boost | 1       | 22.5                 |  


Let's also test solving of Quadratic equation:


|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (mcr. s) | 3375       | 59                 |  
| Relative boost | 1       | 57.2                 |  

> Program was given equation  $x^2 + 4x + 3 = 0$ to solve 1000 times

You can find source code of all Assembly programs <a href = "https://github.com/dodokek/Processor/tree/origin/examples">here</a>.


We can see the huge improvement in speed, JIT compilation rules!


## Conclusion

*Just in time compilation* - an essential part in many project. Despite that I implemented only a small part of real JIT, I aquired useful skills. I get a vast amount of knowledge about CPU architecture and x86-64 commands structure. All this journey... was... pretty good. 


## Useful links


I really recommend to read this:
https://dse.in.tum.de/wp-content/uploads/2022/01/translating_x86_binaries_into_llvm_intermediate_representation.pdf  
  
https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170#x64-register-usage

https://www.felixcloutier.com/x86/divsd

https://gist.github.com/williballenthin/6857590dab3e2a6559d7

