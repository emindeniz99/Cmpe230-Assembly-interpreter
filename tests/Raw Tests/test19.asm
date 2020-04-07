code segment 
mov ah,1
int 21h
cmp al,97
jb printit
cmp al,122
ja printit
sub al,32
printit:
mov cx,2
mul cx
mov dl,al
rcr dx,1
int 21h
int 20h
code ends