code segment
read:
    mov ah, 01h
    int 21h
    or init, al
    sub cnt, 1d
    jnz read
print:
    mov dl, init
    mov ah, 02h
    int 21h
    mov dl, init
    xor dl, 7h
    int 21h
exit:
    int 20h
init db 'b'
cnt dw 3d
code ends