code segment
oper:
   mov al, 7
   mov cl, 10d
   mul cl
   cmp ax, 50d
   ja res
   mov ah, 02h
   mov dl, lower
   int 21h
res:
   mov ah,  02h
   mov dl, greater
   int   21h
int 20h
greater db '>'
lower db '<'  
code ends