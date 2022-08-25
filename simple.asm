mainn:
    lidt[zero]
    int 0x01
    ret

zero:
    db 0

times 512-($-$$) db 0