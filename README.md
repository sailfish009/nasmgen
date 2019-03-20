https://www.hackitu.de/nasmgen/

Assembler transpiler

nasmgen can be seen as a transpiler that will convert a simple high-level language into 64bit Intel NASM code. In order to conceal calling conventions and particular register/mnemonic semantics, the core idea is to have a more intuitive programming “language” with primitives for:

    Variable names for registers, with declared (pointer) types, type-checking, scopes, and pointer arithmetics
    Conditionals with if and else
    Comparisons using >, >=, etc.
    Loops like while, until or goto

Using the NASM transpiler

A file such as foo.nasmgen provided in the syntax stated below can be built as a binary by using the following make targets:

    make foo.asm will build the main nasmgen binary and convert your code into NASM syntax, in case you want to manually inspect or edit the output.
    make foo.asm.o will do the above and feed the output to nasm for getting an object file suitable for linking. This of course requires nasm to be installed separately beforehand.
    make foo.asm.h will create a matching extern declaration of the function’s signature in C/C++ for convenient include.

Example usage

This atoui function in the custom nasmgen syntax parses a string into an unsigned 64bit integer:

u64 atoui (p8 str) {
    rv := 0

    u64 base # 64 bit as its needed for mul with rv
    base := 10

    while (str[0] <> 0) { # main while loop
        rv *= base

        u64 tmp
        tmp := str[0] # dereference into 64bit for adding to rv
        tmp -= 48
        rv += tmp

        str += 1 # arg ptr increment
    }
}

After transpiling, the resulting output for NASM will look like:

BITS 64
; extern uint64_t atoui(unsigned char*);
section .text
global atoui
atoui:
    ; ret 'rv': RAX (64)
    ; arg 'str': DIL (*8)
    mov RAX, qword 0d0

    ; var 'base': RCX (64) ; 64 bit as its needed for mul with rv
    mov RCX, qword 0d10

    .if_7: ; main while loop
    cmp byte[RDI], byte 0d0 ; main while loop
    je .else_7 ; main while loop
    mul RCX

    ; var 'tmp': RSI (64)
    movzx RSI, byte[RDI] ; dereference into 64bit for adding to rv
    sub RSI, qword 0d48
    add RAX, RSI

    inc RDI ; increment by 1 byte
    ; eol RSI
    jmp .if_7 ; main while loop
    .else_7: ; endif ; main while loop
    ; eol RCX
    ; eol DIL
    ; eol RAX

    ret

Please note the implicitly available rv variable in EAX that thus allows mul, div, and ret operations.
Generate assembler code: Language syntax

Basically, the code is the documentation, but this roughly is the supported “high”-level instruction syntax:

Variable declaration
    Variables can be declared anywhere inside the program by stating their type: type id. This simply reserves a register of corresponding size until the current scope (such as inside loops) is left or until an explicit undef id. Variable identifiers (and labels, see below) can consist of lowercase letters and underscores. If GCC “callee save” registers have to be used, they will be pushed and popped accordingly.
Types
    The most basic type are unsigned integers u8, u16, u32, and u64 that directly refer to a register. Corresponding pointers (or addresses) can be defined by the p prefix such as p64. Pointers can be dereferenced with brackets [] that enclose an identifier or immediate, where supported. Declarations of the special string pointer type such as s8 are immediately followed by a raw string that will be placed in the .data section and thus don’t occupy a register.
Variable operations
    Variable assignment with := and arithmetic in-place operations such as += or *= are stated as var op varval. Identifiers, immediates and pointer lookups can be used where supported.
Function declaration
    The function to be exported can be defined as type id([type id [, …]]) { … }. In addition to the optional arguments, the return variable rv will be implicitly available, which will be located in the A register and thus supports multiplication operations.
Conditionals
    Supported are (negated) if blocks and some shortcuts with the following syntax: if|unless (id [cmp id]) ( { … } [ else { … } ] | var := varval | goto lbl ). Blocks support else branches, while direct assignments and jumps need less instructions.
Loops
    Similar to conditionals, loops use a possibly abbreviated comparison with a following block: while|until (id [cmp id]) { … }.
Goto
    A label defined using lbl: can be jumped to from anywhere with goto lbl. Note that this might interfer with the variable scope stack, so its usage is discouraged.
Inline assembler
    NASM assembler code can be inlined using asm "…". Variable identifiers prefixed with a $ will be replaced by their assigned register’s name.
Loop builtins
    NASM will unroll loops defined using repeat (id, from-imm, to-imm) { … }. This is done by using the NASM assign and rep builtins.
Comments
    Every line starting with a # will be treated as a comment and passed through as a NASM comment unchanged.

Caveats

There are no advanced compiler-design features implemented (yet?), such that the expressive power and basic assembler-like feeling stays intact. As only a simple translation was intended at first, there is no real well-defined grammar or semantics – but due to various corner-cases, it became overall over-engineered very quickly. Thus, the complexity did grow more than initially intended, causing future enhancements (except for simple additional commands) pretty much requiring major refactoring. It still works fine for low-level calculations and control flow.