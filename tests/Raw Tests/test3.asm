code segment
mov cx, value
rev:
   mov ah, 0
   mov al, cl
   mov ch, 10d
   div ch
   mov cl, al
   add ah, '0'
   mov dl, ah 
   mov ah,  02h
   int  21h
   cmp cl, 0
   jnz rev
int 20h
value dw 30h
code ends