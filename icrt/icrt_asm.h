// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

#ifndef _ICRT_ASM_H
#define _ICRT_ASM_H

// in there we define a few inline asm utilities:
    // 32/64 bits define
    // macro to call a func with stack pointer as argument
    // macro to define the injection entrypoint (nops + call)
    // macro to set SP & jump to specified address

// arch currently supported:
    // x86
    // x86_64
    // arm
    // arm64

#ifdef __i386__
    #define BITS32

    // x86: code to call a function with stack pointer as argument
    #define CALL_SP(addr) \
    asm volatile("push %%esp; call *%%eax;" : : "a"(addr) );

    // x86: entry point of code injected into remote process
    #define INJ_ENTRY(name, func) \
    asm(".text; .global " #name "; " #name ":; nop; nop; call " #func "; int3;"); \
    extern void name(void);

    // x86: code to set stack pointer and jump to address
    #define FIX_SP_JMP(stack, addr) \
    asm volatile("xchg %%esp, %0; jmp *%%eax;" : "=r"(stack) : "0"(stack), "a"(addr) );

#elif __x86_64__
    #define BITS64

    // x86_64: code to call a function with stack pointer as argument
    #define CALL_SP(addr) \
    asm volatile("mov %%rsp, %%rdi; call *%%rax;" : : "a"(addr) );

    // x86_64: entry point of code injected into remote process
    #define INJ_ENTRY(name, func) \
    asm(".text; .global " #name "; " #name ":; nop; nop; call " #func "; int3;"); \
    extern void name(void);

    // x86_64: code to set stack pointer and jump to address
    #define FIX_SP_JMP(stack, addr) \
    asm volatile("xchg %%rsp, %0; jmp *%%rax;" : "=r"(stack) : "0"(stack), "a"(addr) );

#elif __arm__
    #define BITS32

    // arm: code to call a function with stack pointer as argument
    #define CALL_SP(addr) \
    asm volatile("mov r0, sp; mov pc, %0;" : : "r"(addr) );

    // arm: entry point of code injected into remote process
    // .inst 0xe7f001f0 = bkpt
    #define INJ_ENTRY(name, func) \
    asm(".text; .global " #name "; " #name ":; nop; bl " #func ";.inst 0xe7f001f0;"); \
    extern void name(void);

    // arm: code to set stack pointer and jump to address
    #define FIX_SP_JMP(stack, addrp) \
    asm volatile("mov %%sp, %0; bx %[addr];" : "=r"(stack) : "0"(stack), [addr] "r"(addrp) );

#elif __aarch64__
    #define BITS64

    // arm64: code to call a function with stack pointer as argument
    #define CALL_SP(func) \
    asm volatile("mov x0, sp; bl " #func"; ");

    // arm64: entry point of code injected into remote process
    #define INJ_ENTRY(name, func) \
    asm(".text; .global " #name "; " #name ":; nop; bl " #func ";.inst 0xd4200000;"); \
    extern void name(void);

    // arm64: code to set stack pointer and jump to address
    #define FIX_SP_JMP(stackp, addrp) \
    asm volatile("mov x0, sp; mov x1, %[stack]; sub x0, x1, x0; add sp, sp, x0; br %[addr]" \
        : : [stack] "r"(stackp), [addr] "r"(addrp) : "x0", "x1");

#endif

// small defines for auto detect injection address
#ifdef BITS32
    #define MAPS_ADDR_MASK 0xf0000000
    #define MAPS_ADDR_ALIGN(x) (x + 0x00010000 - (x % 0x00010000))
#else
    #define MAPS_ADDR_MASK 0x7f0000000000
    #define MAPS_ADDR_ALIGN(x) (x + 0x01000000 - (x % 0x01000000))
#endif


#endif
