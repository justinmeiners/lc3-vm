.ORIG x3000

; echo "ab" | ./lc3 test.obj > test_output.txt
; cmp test_output.txt test_expected.txt

INIT
    ; Print the character !
    AND R0, R0, 0
    ADD R0, R0, 10
    ADD R0, R0, 10
    ADD R0, R0, 13
    OUT ; !

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

    ; Save offset for printing ASCII integers.
    AND R1, R1, 0
    ADD R1, R1, 10
    ADD R1, R1, 10
    ADD R1, R1, R1
    ADD R1, R1, 8
    ADD R0, R1, 0
    OUT ; 0

    ; Test some arithemtic
    ; 1 + 1
    AND R2, R2, 0
    ADD R2, R2, 1
    ADD R2, R2, R2
    ADD R0, R1, R2
    OUT  ; 2

    ; 2 + 2
    ADD R3, R2, R2
    ADD R0, R1, R3
    OUT ; 4

    ; 4 - 1
    ADD R4, R3, -1
    ADD R0, R1, R4
    OUT ; 3

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

; BRANCHING
    LEA R0, BRANCH_MESSAGE
    PUTS
    BRnzp COUNT_DOWN
    BRANCH_MESSAGE .STRINGZ "BRANCH\n"

COUNT_DOWN
    AND R2, R2, 0
    ADD R2, R2, 9
COUNT_LOOP
    ADD R0, R1, R2
    OUT ; 9,8,7...0
    ADD R2, R2, -1
    BRp COUNT_LOOP

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

    AND R3, R3, 0
    BRnp BAD_BRANCH ; false
    ADD R3, R3, 4
    BRnz BAD_BRANCH ; false
    ADD R3, R3, -6
    BRzp BAD_BRANCH ; false
    BRn GOOD_BRANCH ; true
BAD_BRANCH
    HALT

; LOAD/STORE
GOOD_BRANCH
    LEA R0, LOAD_MESSAGE
    PUTS
    BRnzp STORE
    LOAD_MESSAGE .STRINGZ "LOAD/STORE\n"
    STACK .FILL x4000

STORE
    LD R6, STACK

    ; store a value
    AND R3, R3, 0
    ADD R3, R3, 7
    STR R3, R6, 0

    ; change it
    ADD R3, R3, 1
    ADD R0, R1, R3
    OUT ; 8

    ; load the original value back
    LDR R3, R6, 0
    ADD R0, R1, R3
    OUT ; 7

    ; store and load. this time with offset.
    ADD R4, R3, -1
    STR R4, R6, 1
    LDR R3, R6, 1
    ADD R0, R1, R3
    OUT ; 6

    ; load the original value again
    LDR R3, R6, 0
    ADD R0, R1, R3
    OUT ; 7

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

    LEA R0, CALL_MESSAGE
    PUTS
    BRnzp CALLS
    CALL_MESSAGE .STRINGZ "CALL/JSR/RET\n"

; multiply R2 and R3. Return in R0
MULT
    AND R0, R0, 0
    ADD R3, R3, 0
    BRnz MULT_END
MULT_LOOP
    ADD R0, R0, R2
    ADD R3, R3, -1
    BRnp MULT_LOOP
MULT_END
    RET

CALLS
    ; 9 * 8 = 72
    AND R2, R2, 0
    ADD R2, R2, 9
    ADD R0, R1, R2
    OUT ; 9

    AND R3, R3, 0
    ADD R3, R3, 8
    ADD R0, R1, R3
    OUT ; 8

    JSR MULT
    OUT ; H

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

    LEA R0, INPUT_MESSAGE
    PUTS
    BRnzp INPUT
    INPUT_MESSAGE .STRINGZ "INPUT/GETc/IN\n"
    INPUT_A .STRINGZ "Press \"a\"\n"
    INPUT_B .STRINGZ "Press \"b\"\n"

INPUT
    LEA R0, INPUT_A
    PUTs
    GETc
    OUT

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n

    LEA R0, INPUT_B
    PUTs
    IN
    OUT

    AND R0, R0, 0
    ADD R0, R0, 10
    OUT ; \n
    
    HALT

.END

