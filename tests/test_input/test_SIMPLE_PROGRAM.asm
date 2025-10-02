LDA 2000H
MOV B, A
LDA 2001H
ADD B ; Adds the two numbers in A & B together
STA 2002H
HLT ; This halts program execution

ORG 2000H ; This sets the offset of the program to 2000H
DB 01H
DB 02H