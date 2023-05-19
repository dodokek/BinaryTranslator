# The JIT compilator

## Overview
In this project I combined all my skills, that I gained from educating on the 1st year of MIPT. 

## About

I translate binary code, generated from my own <a href="https://github.com/dodokek/ProgrammingLanguage">Programming language</a> into **x86-64** machine code. Program walks through the executable and translate each instruction into one or several *x86-64* instructions architecture. <br><br> My translator translates the instructions and puts them into **Executable and Linkable format (ELF)**.

Commands also can be executed during the main program runtime.With the help of *mprotect syscall* the *x86-64 buffer* becomes executable, and code injection occurs. Check out this version <a href="https://github.com/dodokek/BinaryTranslator/tree/JIT-translation-in-time-execution">here</a>.
<br><br>
With the help of JIT compilation, execution became **~30 times faster**.

> Before translation I optimize command's structure to improve the performance a bit. 

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

Step 3: Execute

~~~
~BinaryTranslator/src make run
~~~





## Translation table

Here is the translation table from *Native assembly* to *x86-64 assembly*


<details>
<summary>Translation of Push/Pop variations</summary>

|   Native       | x86-64      |  
| ------         | :---------------: | 
| ``` Push Num  ```     | <p style="text-align: left;"> ``` mov rsi, 0x1ff0...00 (double)  ``` <br> ``` push rsi ```   </p>          |  
|``` Push reg  ```     | <p style="text-align: left;">  ``` push r_x  ```   </p>           |
|``` Push [ Num ]  ```    | <p style="text-align: left;">  ``` mov rdi, [r10 + Num] ``` <br>  ```push rdi  ```    </p>          |
| ``` Push [ r_x ]  ```     | <p style="text-align: left;">  ``` mov [r10 + r_x]    ```   </p>  |
| ``` Push [ Num + r_x ] ```       | <p style="text-align: left;">  ``` add r10, r_x <br> mov rdi, [r10 + Num] ``` <br> ``` sub r10, r_x``` <br> ``` push rdi ``` </p>|
| ``` Pop r_x ```        | <p style="text-align: left;"> ``` pop r_x  ```   </p>          |
|``` Pop [ r_x ]  ```     | <p style="text-align: left;"> ``` pop rdi ``` <br> ```mov [r10 + r_x], rdi   ```   </p>         |
| ```Pop [Num + r_x]  ```     | <p style="text-align: left;"> ``` pop rdi ```<br> ```add r10, r_x ``` <br> ```mov [r10 + Num], rdi``` <br> ```sub r10, r_x  ```  </p>        |

> **r10** register is used as pointer to begin of *guest-memory*.
</details>


<details>
<summary>Translation of Math</summary>

|   Native       | x86-64      |  
| ------         | :---------------: | 
|``` Add  ```      | ``` addpd xmm0, xmm1  ```     |
|``` Sub  ```      | ``` subpd xmm0, xmm1  ```     |
|``` Mul  ```      | ``` mulpd xmm0, xmm1  ```     |
|``` Div   ```     | ``` divpd xmm0, xmm1  ```     |
|``` Sqr   ```     | ``` sqrtpd xmm0, xmm1 ```     |

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

call DoublePrint // wrapper for standart printf

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
|``` Jmp <32b rel. ptr.>  ```      |  ```jmp <32b rel. ptr.> ```      |
|``` Call <32b rel. ptr.> ```       |  ```call <32b rel. ptr.>  ```     |
|``` Ret  ```      | ``` ret    ```   |

Conditional Jumps are also translated the same, but in Native assembly we also need to compare two values from stack. Comparison part:

~~~C++
mov xmm0, [rsp]
mov xmm1, [rsp+8]
add rsp, 16

ucomisd xmm0, xmm1
~~~

The rest translation of conditional jumps is the same with non-conditional jump.

</details>

## IR - Intermediate Representation

> "In the world, where LLVM exists, we can't go without IR." - Dedinsky, ancient greek philosopher.

Each command of native assembly is translated into it's IR. After first iteration through *source binary file* we get array of structs:

~~~
Array of structs:

+-----------------+          +-----------------+             +-----------------+ 
| ID: PUSH        |          | ID: POP         |             | ID: RET         |
| Native size: 1  |          | Native size: 1  |             | Native size: 1  |
| x86-64 size: 14 |          | x86-64 size: 12 |             | x86-64 size: 1  |
| =============== | -------- | =============== | --- ... --- | =============== | 
| value: 4        |          | value: 0        |             | value: 0        |
| reg index: 1    |          | reg index: 2    |             | reg index: 0    |
| checksum: 12    |          | checksum: 16    |             | checksum: 0     |
+-----------------+          +-----------------+             +-----------------+
~~~

With the help of this representation, translation becomes much **easier and readable**. Moreover, we can apply some optimizations.


## Performance test
Using <a href="https://githrsiub.com/dodokek/ProgrammingLanguage">my assembly work implementation</a> I wrote program, wich finds factorial. 

I executed it with my CPU work implementation on C. After that I executed it using JIT translation.

Let's see the improvement in speed:

|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (μs) | 810 $\pm$ 5  | 36 $\pm$ 5                 |  
| Relative boost | 1       | 22.5 $\pm$ 0.1                |  

> Program was given $5!$ to calculate 1000 times 


Let's also test solving of Quadratic equation:


|  | Native       | x86-64                 |  
| :------: | :------:  | :---------------: | 
| Execution time (μs) | 3375 $\pm$ 5 | 59  $\pm$ 5 |  
| Relative boost | 1       | 57.2  $\pm$ 0.1   |  

> Program was given equation  $x^2 + 4x + 3 = 0$ to solve 1000 times

You can find source code of all Assembly programs <a href = "https://github.com/dodokek/Processor/tree/origin/examples">here</a>.


We can see the huge improvement in speed, JIT compilation rules!


## Optimizations

Before translation, binary code is transformed into *IR*. Thanks to that, some instrucions might be optimized. I got *0.5%* relative performance boost with them.

### Immediate's storage optimization
In *native Assembly representation* we can see a lot of instruction series like this:

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


## Conclusion

During the work on this task, I learned a lot about JIT compilation, IR and x86-64 architecture. In future I want to improve IR, so it will have real CFG. A lot of new optimizations might be done with the help of it.

The earliest version of this project was simple JIT translation with further execution in the main program runtime. Closer to the end of semester I wrote compilation into .elf file.

All this journey from simple Quadratic equation to this project was... pretty good. 


## Useful links


This one really helped me to understand some important concepts of binary translators: <br>
https://dse.in.tum.de/wp-content/uploads/2022/01/translating_x86_binaries_into_llvm_intermediate_representation.pdf  
  
  
Information, that helped me in some parts of project: <br>
https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170#x64-register-usage

https://www.felixcloutier.com/x86/divsd

https://gist.github.com/williballenthin/6857590dab3e2a6559d7

