; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix
		

Thous				EQU			12;
Hun					EQU			8;
Ten					EQU			4;
One					EQU			0;


    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB

  

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec
	PUSH{R4-R11};
	MOV R5,LR; Original LR
	SUB SP,#16; Move stack pointer up
	
	;Initialize variables to 0
	MOV R1,#0;
	STR R1,[SP,#One];
	STR R1,[SP,#Ten];
	STR R1,[SP,#Hun];
	STR R1,[SP,#Thous];
	
	
	MOV R4,#0;
LCD_OutDec1	
	CMP R0,#10; no need to use mod if less than 10
	BLT PushN; signifies the base case(recursion),skip to Pushn
	MOV R2,#10;
	UDIV R3,R0,R2; Division
	MUL R1,R3,R2; Modulus
	SUB R1,R0,R1; Modulus
	
	STR R1,[SP,R4]; Push the current N, Offset is R4
	ADD R4,#4;
	MOV R0,R3; Move back into input register
	B LCD_OutDec1;
	
PushN
	STR R0,[SP,R4]; Store final digit
	MOV R4,#12; Reset offset to Thousands location

ChekDig
	LDR R0,[SP,R4]; Load current digit
	
DispDig
	ADD R0,#48; Convert to ascii
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	SUB R4,#4; Decrement offset (working downwards from Thousand -> One
	CMP R4,#0; 
	BGE ChekDig; Including offset of 0 (ones digit)
	
Lv1
	ADD SP,#16; Move back
	MOV LR,R5;
	POP{R4-R11};
      BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.001, range 0.000 to 9.999
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.000 "
;       R0=3,    then output "0.003 "
;       R0=89,   then output "0.089 "
;       R0=123,  then output "0.123 "
;       R0=9999, then output "9.999 "
;       R0>9999, then output "*.*** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix
	PUSH{R4-R11};
	MOV R5,LR;
	SUB SP,#16; Pointing to Ones variable
	
	;Initialize variables to 0
	MOV R1,#0;
	STR R1,[SP,#One];
	STR R1,[SP,#Ten];
	STR R1,[SP,#Hun];
	STR R1,[SP,#Thous];
	
	
	MOV R4,#0;
LCD_OutFix1	
	CMP R0,#10; no need to use mod if less than 10
	BLT PushN1; signifies the base case(recursion),skip to Pushn
	MOV R2,#10;
	UDIV R3,R0,R2; Division
	MUL R1,R3,R2; Modulus
	SUB R1,R0,R1; Modulus
	
	STR R1,[SP,R4]; Push the current N, Offset is R4
	ADD R4,#4;
	MOV R0,R3; Move back into input register
	B LCD_OutFix1;
	
PushN1
	STR R0,[SP,R4]; Store final digit
	
	MOV R4,#12; Start with thousands
	
	LDR R0,[SP,R4]; Thousands
	ADD R0,#48;
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	
	MOV R0,#46; Point
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	SUB R4,#4;
	
	
	LDR R0,[SP,R4]; Hundreds
	ADD R0,#48;
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	SUB R4,#4;
	
	LDR R0,[SP,R4]; Tens
	ADD R0,#48;
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	SUB R4,#4;
	
	LDR R0,[SP,R4]; Ones
	ADD R0,#48;
	PRESERVE8 {TRUE};
	BL ST7735_OutChar; Display on LCD
	
	
	ADD SP,#16;
	MOV LR,R5;
	POP{R4-R11};
     BX   LR
 
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN                           ; make sure the end of this section is aligned
     END                             ; end of file
