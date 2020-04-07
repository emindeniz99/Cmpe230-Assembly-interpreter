code segment
oper:
   mov al, 7
   mul first_el
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
first_el db 10d 
code ends