code segment
mov ah,1
int 21h
mov dl,al
mov cl,al
sub cl,30h
cmp cl,0
jbe endloop
mov ah,2
progloop:
    int 21h
    sub cl,1
    cmp cl,1
    jae progloop
endloop:
    int 20h
code ends