code segment
mov ch,0FFh
mov dh,0FFh
rcl ch,1 
rcl dh,2
sub dh,ch
shr dx,8
add dx,6
mov ax,5
mul dx
mov dx,ax
mov ah,2
int 21h
int 20h
code ends