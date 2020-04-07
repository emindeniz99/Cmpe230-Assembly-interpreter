code segment
inputs:
    mov bp, 0a22h
    mov ah, 01h
    int 21h
    mov b  [ bp ], al
    int 21h
    add bp, 2d
    mov b[ bp ] ,   al
calc:
    sub bp, 2h
    mov al, b [bp]
    shl al, 1h
    xor al, 7h
    mov cl, b[bp]
    mov b[bp], al
    mov al, cl
    rcl al, 1h
    or al, 0Dh
    add bp, 1d
    mov b[bp], al
    add bp, 1d
    mov cl, b[bp]
    rcl cl, 5h
    xor cl, 73h
    mov dl, b[bp]
    mov b[bp], cl
    mov cl, dl
    shl cl, 1h
    add cl, 5h
    add bp, 1
    mov b[bp], cl
    mov cx, 4
    sub bp, 3
print:
    mov ah, 02h
    mov dl, b[bp]
    int 21h
    add bp, 1
    sub cx, 1
    cmp cx, 0
    jnz print
ex:
    int 20h
code ends