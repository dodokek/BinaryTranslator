
section .data

template_str: 		db "Hello %%there %%%x %s, %c, %d, %b, %j", 10d, 0	; template string

buffer: times 64 	db 12				; buffer

string_buffer: times 16 db 0			; for translator to store the number to print

string_help_buffer: times 16 db 0		; for reversing number string

message_error:		db 10d, "No such option for % in printf, my friend :(", 10d, 0

;------------------------------------------------

section .text

extern printf
global DodoPrint

DodoPrint:
	pop r13					; saving return address

	; rax - number to print
	call DecToCmd

	push r13

	ret





;------------------------------------------------
; Translates value in eax into dec 
; template string
;------------------------------------------------
;  Entry: rax - dec value to print
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

	mov byte [rsi], 10d	; new line symb
	call Putch

	ret


