code segment
mov dx,0h
mov ax,41h
mov bx,20h
mov cx,var1
div bx
xor ax,dx
mul cx
mov dx,ax
int 21h
int 20h
var1 dw 183 
code ends