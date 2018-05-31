// ======================================================================== //
// author:  ixty                                                       2018 //
// project: inline c runtime library                                        //
// licence: beerware                                                        //
// ======================================================================== //

// inline assembly implementation of linux syscalls
// we define a _syscall_do() macro which calls a syscall with up to 6 arguments
// arguments are assumed to be declared as a1, a2, ..., a6

// we then use this macro to define macros for syscalls with specifid number of args
// finally, we use those macros to define the functions that are of interest to us

#ifndef _ICRT_SYSCALL_H
#define _ICRT_SYSCALL_H

// syscall numbers
#include <sys/syscall.h>


// ========================================================================== //
// define syscall asm stub for all archs here
// ========================================================================== //

// x86
#ifdef __i386__
    #define _syscall_do(sys_nbr, rettype)                                   \
    {                                                                       \
        rettype ret = 0;                                                    \
        register int r0 asm ("ebx") = (int)a1;                              \
        register int r1 asm ("ecx") = (int)a2;                              \
        register int r2 asm ("edx") = (int)a3;                              \
        register int r3 asm ("esi") = (int)a4;                              \
        register int r4 asm ("edi") = (int)a5;                              \
        register int r5 asm ("ebp") = (int)a6;                              \
        register int r7 asm ("eax") = sys_nbr;                              \
        asm volatile                                                        \
        (                                                                   \
            "int $0x80;"                                                    \
            : "=r" (ret)                                                    \
            : "r"(r7), "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) \
        );                                                                  \
        return ret;                                                         \
    }

// x86_64 aka amd64
#elif __x86_64__
    #define _syscall_do(sys_nbr, rettype)                                   \
    {                                                                       \
        register long r10 asm("r10") = (long)a4;                            \
        register long r8  asm("r8")  = (long)a5;                            \
        register long r9  asm("r9")  = (long)a6;                            \
        rettype ret = 0;                                                    \
        asm volatile                                                        \
        (                                                                   \
            "syscall"                                                       \
            : "=a" (ret)                                                    \
            : "0"(sys_nbr), "D"(a1),  "S"(a2),                              \
              "d"(a3),      "r"(r10), "r"(r8), "r"(r9)                      \
            : "cc", "rcx", "r11", "memory"                                  \
        );                                                                  \
        return ret;                                                         \
    }


// arm
#elif __arm__
    #define _syscall_do(sys_nbr, rettype)                                   \
    {                                                                       \
        rettype ret = 0;                                                    \
        register int r0 asm ("r0") = (int)a1;                               \
        register int r1 asm ("r1") = (int)a2;                               \
        register int r2 asm ("r2") = (int)a3;                               \
        register int r3 asm ("r3") = (int)a4;                               \
        register int r4 asm ("r4") = (int)a5;                               \
        register int r5 asm ("r5") = (int)a6;                               \
        register int r7 asm ("r7") = sys_nbr;                               \
        asm volatile                                                        \
        (                                                                   \
            "swi #0; mov %0, r0"                                            \
            : "=r" (ret)                                                    \
            : "r"(r7), "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) \
        );                                                                  \
        return ret;                                                         \
    }


// arm64
#elif __aarch64__
    #define _syscall_do(sys_nbr, rettype)                                   \
    {                                                                       \
        rettype ret = 0;                                                    \
        register long x0 asm ("x0") = (long)a1;                             \
        register long x1 asm ("x1") = (long)a2;                             \
        register long x2 asm ("x2") = (long)a3;                             \
        register long x3 asm ("x3") = (long)a4;                             \
        register long x4 asm ("x4") = (long)a5;                             \
        register long x5 asm ("x5") = (long)a6;                             \
        register long x8 asm ("x8") = sys_nbr;                              \
        asm volatile                                                        \
        (                                                                   \
            "svc #0; mov %0, x0"                                            \
            : "=r" (ret)                                                    \
            : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) \
        );                                                                  \
        return ret;                                                         \
    }

// something else?
#else
    #error "unknown arch"
#endif


// ========================================================================== //
// defines to generate syscall wrappers
// ========================================================================== //

#define _syscall6(sys_nbr, sys_name, rettype, t1, t2, t3, t4, t5, t6)       \
static inline rettype sys_name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)    \
{                                                                           \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall5(sys_nbr, sys_name, rettype, t1, t2, t3, t4, t5)           \
static inline rettype sys_name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)           \
{                                                                           \
    long a6=0;                                                              \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall4(sys_nbr, sys_name, rettype, t1, t2, t3, t4)               \
static inline rettype sys_name(t1 a1, t2 a2, t3 a3, t4 a4)                  \
{                                                                           \
    long a6=0, a5=0;                                                        \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall3(sys_nbr, sys_name, rettype, t1, t2, t3)                   \
static inline rettype sys_name(t1 a1, t2 a2, t3 a3)                         \
{                                                                           \
    long a6=0, a5=0, a4=0;                                                  \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall2(sys_nbr, sys_name, rettype, t1, t2)                       \
static inline rettype sys_name(t1 a1, t2 a2)                                \
{                                                                           \
    long a6=0, a5=0, a4=0, a3=0;                                            \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall1(sys_nbr, sys_name, rettype, t1)                           \
static inline rettype sys_name(t1 a1)                                       \
{                                                                           \
    long a6=0, a5=0, a4=0, a3=0, a2=0;                                      \
    _syscall_do(sys_nbr, rettype)                                           \
}

#define _syscall0(sys_nbr, sys_name, rettype)                               \
static inline rettype sys_name(void)                                        \
{                                                                           \
    long a6=0, a5=0, a4=0, a3=0, a2=0, a1=0;                                \
    _syscall_do(sys_nbr, rettype)                                           \
}


// ========================================================================== //
// define desired syscalls
// ========================================================================== //

_syscall0(SYS_getpid,   _getpid,    int)

_syscall1(SYS_exit,     __exit,     int,        int)
void _exit(int c) { __exit(c); }

_syscall1(SYS_close,    _close,     int,        int)
_syscall1(SYS_brk,      _brk,       long,       unsigned long)

_syscall2(SYS_munmap,   _munmap,    long,       char*, int)

_syscall3(SYS_read,     _read,      ssize_t,    int, void *, size_t)
_syscall3(SYS_write,    _write,     ssize_t,    int, const void *, size_t)
_syscall3(SYS_lseek,    _lseek,     long,       int, long, int)
_syscall3(SYS_mprotect, _mprotect,  long,       void*, long, int)

#if __i386__ || __arm__ || __x86_64__
_syscall3(SYS_open,     _open,      int,        char *, int, int)
#else
_syscall4(SYS_openat,   _openat,    int,        int, char *, int, int)
#define AT_FDCWD        -100
#define _open(a, b, c) _openat(AT_FDCWD, a, b, c)
#endif

_syscall4(SYS_ptrace,   _ptrace,    long,       int, int, void*, void*)
_syscall4(SYS_wait4,    _wait4,     int,        int, int*, int, void*)

#if __i386__ || __arm__
    _syscall6(SYS_mmap2, _mmap, void *, void *, long, int, int, int, long)
#else
    _syscall6(SYS_mmap, _mmap, void *, void *, long, int, int, int, long)
#endif


// ========================================================================== //
// usefull constants for common syscalls
// ========================================================================== //
#define O_RDONLY        00
#define O_WRONLY        01
#define O_RDWR          02
#define O_CREAT         0100
#define O_TRUNC         01000
#define O_APPEND        02000

#define F_DUPFD         0
#define F_GETFD         1
#define F_SETFD         2
#define F_GETFL         3
#define F_SETFL         4

#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4
#define PROT_NONE       0x0

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_TYPE        0x0f
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20


#define PTRACE_TRACEME      0
#define PTRACE_PEEKTEXT     1
#define PTRACE_PEEKDATA     2
#define PTRACE_PEEKUSER     3
#define PTRACE_POKETEXT     4
#define PTRACE_POKEDATA     5
#define PTRACE_POKEUSER     6
#define PTRACE_CONT         7
#define PTRACE_KILL         8
#define PTRACE_SINGLESTEP   9
#define PTRACE_GETREGS      12
#define PTRACE_SETREGS      13
#define PTRACE_GETFPREGS    14
#define PTRACE_SETFPREGS    15
#define PTRACE_ATTACH       16
#define PTRACE_DETACH       17
#define PTRACE_GETFPXREGS   18
#define PTRACE_SETFPXREGS   19
#define PTRACE_SET_SYSCALL  23
#define PTRACE_SYSCALL      24
#define PTRACE_SYSEMU       31

#define PTRACE_GETREGSET    0x4204
#define PTRACE_SETREGSET    0x4205

#define NT_ARM_SYSTEM_CALL  0x404

#define NT_PRSTATUS         1
#define NT_PRFPREG          2
#define NT_PRPSINFO         3
#define NT_TASKSTRUCT       4
#define NT_AUXV             6

#endif
