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

How commands from my Assembly are translated to x86-64 Assembly commands.

<details>
<summary>Translation table</summary>
<details>
<summary>Push/Pop variations</summary>

|   Native       | x86-64      |  
| ------         | :---------------: | 
| Push Imm       | аа сложна                 |  
| push reg       | ууу сложна              |
</details>


</details>



# Useful links

https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170#x64-register-usage

https://www.felixcloutier.com/x86/divsd

https://gist.github.com/williballenthin/6857590dab3e2a6559d7

## Translation of commands to x86

| Origin cmd     | x86      |  
| ------         | :---------------: | 
| push imm       | аа сложна                 |  
| push reg  | ууу сложна              |
