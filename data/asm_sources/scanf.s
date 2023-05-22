section .data
    buffer: times 10 db 5

section .text
    global _start

_start:
    mov eax, 0x0    ; putting registers in state for scanf syscall
	xor rdx, rdx    
	mov rsi, buffer ; storing pointer to buffer where to write string
	mov rdi, 20     ; maximum length
	syscall

    ; Convert input to decimal
    mov rax, 0      ; result will be stored in rax
    mov rcx, 10     ; setting base for atoi
    mov rdi, buffer ; pointer to string buffer
    call atoi       ; string in buffer -> decimal number in rax

	cvtsi2sd xmm0, eax      ; transforming number in eax to double-precision number in xmm0
	sub rsp, 8 
	movsd qword [rsp], xmm0 ; storing xmm0 in stack
	pop rsi                 ; moving result to rsi

	ret
;-----------------------------------------

atoi:
    mov rax, 0              ; Set initial total to 0
     
convert:
    movzx rsi, byte [rdi]   ; Get the current character
    test rsi, rsi           ; Check for \0
    je done
    
    cmp rsi, 48             ; Anything less than 0 is invalid
    jl error
    
    cmp rsi, 57             ; Anything greater than 9 is invalid
    jg error
     
    sub rsi, 48             ; Convert from ASCII to decimal 
    imul rax, 10            ; Multiply total by 10
    add rax, rsi            ; Add current digit to total
    
    inc rdi                 ; Get the address of the next character
    jmp convert

error:
    mov rax, -1             ; Return -1 on error
 
done:
    ret                     ; Return total or error code