section .data

template_str: 		db "Hello %%there %%%x %s, %c, %d, %b, %j", 10d, 0	; template string

buffer: times 64 	db 12				; buffer

string_buffer: times 16 db 0			; for translator to store the number to print

string_help_buffer: times 16 db 0		; for reversing number string

message_error:		db 10d, "No such option for % in printf, my friend :(", 10d, 0

section .rodata

jump_table:
						dq Bin
						dq Char
						dq Decimal
times ('o' - 'd' - 1)   dq Void
						dq Oct
times ('s' - 'o' - 1)   dq Void
						dq String
times ('x' - 's' - 1)   dq Void
						dq Hex

;------------------------------------------------

section .text

extern printf
global DodoPrint

DodoPrint:
	pop r13					; saving return address

	push r9					; due to the call of the printf, first 6 args are being stored in the following registers			
	push r8					; other arguments are being in stack
	push rcx
	push rdx
	push rsi ; <---------------------<---------------------------<-------------------------------																					
	push rdi ;																					|
	push rbp				; saving base pointer 												|
	;																							|
	mov rbp, rsp			; saving stack pointer to access args								^
	add rbp, 16				; skipping template string in stack	also skipping rbp				|
	;																							|
	; rbp -->----------------------------->---------------->-------------------------------------

	mov rsi, rdi			; rsi points to template string which passed as 1-st argument of printf
	
	; call PrintfMain			; calling main function
	
	pop rbp

	pop rdi 				; balancing stack by deleting 6 args from it
	pop rsi
	pop rdx
	pop rcx
	pop r8
	pop r9

	push r13

	ret


;------------------------------------------------
; Prints arguments in the cmd according to 
; template string
;------------------------------------------------
;  Entry: rsi - pointer to template string
;  		  r8 - r15 arguments for printf 
;		
;  Destroys:
;
;  Expects: template strings ends on \0
;------------------------------------------------

PrintfMain:

.loop:
	mov al, [rsi]	; al = *tmpl_string

	cmp al, 0	; if al == \0 then return
	je .end

	cmp al, '%'		; if al == % then handle argument
	je HandleArg

	call Putch		; print char


	inc rsi			; ptr++
	jmp .loop

.end:
	ret


;------------------------------------------------
;  Handle arguments, triggers on % appearence 
;------------------------------------------------
;  Entry: rsi - template string pointer
;
;  Destroys: 
;
;  Expects: 
;------------------------------------------------

HandleArg:

	inc rsi		; now rsi is on type-symbol
	xor rax, rax
	mov al, [rsi]

	cmp al, '%'
	je .is_percent

	cmp al, 'b'
	jl Void

	cmp al, 'x'
	jg Void

.is_percent:
	cmp al, '%'
	jne .no_percent
	
	call Putch
	inc rsi
	jmp PrintfMain

.no_percent:

	inc rsi		; skipping the arg character
	jmp [jump_table + 8 * (rax - 'b')]		; jumping to jump table address


	ret


;------------------------------------------------
;  Functions of jump table for printf 
;------------------------------------------------
;  Entry: to bin, oct, hex, dec, string, char, %%
;
;  Destroys: 
;
;  Expects: jmp table links correctly
;------------------------------------------------	


Char:
	push rsi		; saving pointer to original string

	xor rax, rax
	mov rax, [rbp]	; getting from stack value of next argument
	add rbp, 8d 	; moving ptr to next argument in stack

	mov rsi, buffer	; tmp storage of char
	mov byte [rsi], al

	call Putch

	pop rsi
	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------

String:
	push rsi		; saving pointer to original string

	mov rsi, [rbp]	; getting from stack value of next argument
	add rbp, 8d 	; moving ptr to next argument in stack

	call Puts

	pop rsi
	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------


Bin:
	
	mov rcx, 1		; setting base of 2

	call Base2ToCmd

	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------


Oct:

	mov rcx, 3		; setting base of 8

	call Base2ToCmd

	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------

Decimal:

	call DecToCmd

	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------

Hex:

	mov rcx, 4		; setting base of 16

	call Base2ToCmd

	jmp PrintfMain	; returning to proccessing of each template string's char

;----------------------------------------------------------------------------


Void:

	push rsi

	mov rsi, message_error
	call Puts

	pop rsi

	jmp PrintfMain	; returning to proccessing of each template string's char



;------------------------------------------------
; Translates value in eax into bin, oct or hex 
; template string
;------------------------------------------------
;  Entry: rbp pointing to argument in stack
;  		  rcx - base 2^(1, 3, 4) = 2, 8, 16 = bin, oct, hex
;
;  Destroys: eax, ebx, edx, r15
;
;  Expects: template strings ends on \0
;------------------------------------------------

Base2ToCmd:
	call ClearStringBuffer
	xor r15, r15

	push rsi
	push rdi

	xor rax, rax
	mov rax, [rbp]	; getting from stack value of next argument
	add rbp, 8d 	; moving ptr to next argument in stack

	mov rsi, string_buffer

	xor r10, r10
	cmp eax, 0			; in case digit is below zero
	jge .loop
	neg eax
	mov r15, 1			; setting flag to print '-' at the end

.loop:
	
	mov ebx, eax

	shr ebx, cl
	shl ebx, cl
	mov edx, eax
	sub edx, ebx	; now in edx - surplus
	
	shr eax, cl		; no in eax - res of division
	
	cmp edx, 10d	; checking if there are need to print letters like A,B,C,D,E,F
	jl .not_hex

	add edx, 'A'	; to ASCII letter

	jmp .is_hex
.not_hex:	
	add edx, '0'	; to ASCII digit
.is_hex:

	mov [rsi], edx	; string_buffer = on of the digits in the number
	inc rsi			; ptr++

	cmp eax, 0
	jne .loop


	cmp r15, 1		; adding '-' in case of negative flag in r15
	jne .skip 
	
	mov byte [rsi], '-'
	inc rsi

.skip:
	call ReverseAndPrint
	

	pop rdi
	pop rsi
	ret


;------------------------------------------------
; Translates value in eax into dec 
; template string
;------------------------------------------------
;  Entry: rbp pointing to value in stack
;  		  rcx - base 2^(1, 3, 4) = 2, 8, 16 = bin, oct, hex
;
;  Destroys: eax, ebx, edx
;
;  Expects: template strings ends on \0
;------------------------------------------------

DecToCmd:
	call ClearStringBuffer

	xor r15, r15

	push rsi
	push rdi

	xor rax, rax
	mov rax, [rbp]	; getting from stack value of next argument
	add rbp, 8d 	; moving ptr to next argument in stack

	mov rsi, string_buffer
	mov ebx, 10d   ; base of division

	cmp eax, 0			; in case digit is below zero
	jge .loop
	neg eax
	mov r15, 1			; setting flag to print '-' at the end


.loop:
	
	xor edx, edx	; edx = 0

	div ebx			; now in eax - res, edx - surplus
	
	add edx, 48d	; to ASCII digit

	mov [rsi], edx	; string_buffer = on of the digits in the number
	inc rsi			; ptr++

	cmp eax, 0
	jne .loop

	cmp r15, 1		; adding '-' in case of negative flag in r15
	jne .skip 
	
	mov byte [rsi], '-'
	inc rsi

.skip:
	call ReverseAndPrint
	
	pop rdi
	pop rsi
	ret

;-----------------------------------------
;
;	Function clears reversed string buffer
;	from previous prints
;
;	Destroy: string_help_string
;
;-----------------------------------------

ClearStringBuffer:

	push rsi
	push rdi
	push rcx

	xor rcx, rcx
	mov rsi, string_help_buffer
.loop:

	mov byte [rsi], 0
	inc rsi
	inc rcx
	cmp rcx, 15

	jne .loop


	pop rcx
	pop rdi
	pop rsi

	ret


;------------------------------------------------
;  Copies reverted string to another buffer and prints it 
;------------------------------------------------
;  Entry: rsi - pointer to end of the string to revert
;
;  Destroys: 
;
;  Expects: 
;------------------------------------------------

ReverseAndPrint: 

	;-------Now reversing the string upside-down-----

	mov rdi, string_help_buffer
	dec rsi	; setting ptr to end of string buffer
	
.reverse:

	mov ah, byte [rsi]		; string_buffer[n-i] = string_help_buffer[i]
	mov byte [rdi], ah

	inc rdi
	dec rsi

	cmp rsi, string_buffer - 1 
	jne .reverse

	;-------Printing the string we got--------------

	mov byte [rdi], 5
	mov rsi, string_help_buffer
	call Puts

	ret



;------------------------------------------------
; Prints symbol in console 
;------------------------------------------------
;  Entry: rsi - pointer to symbol to print
;
;  Destroys: rdx, rdi, rax
;
;  Expects: 
;------------------------------------------------

Putch:

	push rcx     ; i still havent figured out, but syscall destroys rcx and r11, so restore them
	push r11

	mov rdx, 1   ; msg len
	mov rdi, 1	 ; file descriptor
	mov rax, 1   ; syscall for write()
;   mov rsi, rsi ; no need cuz in prev func it is already set

	syscall

	pop r11
	pop rcx

	ret


;------------------------------------------------
; Prints string in console until \0
;------------------------------------------------
;  Entry: rsi - pointer to string
;
;  Destroys: 
;
;  Expects: template strings ends on \0
;------------------------------------------------

Puts:

.loop:
	
	cmp byte [rsi], 0
	je .end_loop

	call Putch
	inc rsi

	jmp .loop
.end_loop:

	; mov byte [rsi], 10d	; new line symb
	; call Putch

	ret
