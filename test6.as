    noop
    lw 0 1 two
    beq 1 0 1
    lw 0 1 1
    add 1 1 1
    noop
    beq 0 0 1
    beq 0 0 1
    beq 4 1 end
    lw 1 1 2
    nor 2 2 1
    noop
    nor 1 2 3
end halt
two .fill 2
