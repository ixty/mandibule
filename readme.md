# mandibule: linux elf injector

## intro
Mandibule is a program that allows to inject an ELF file into a remote process.

Both static & dynamically linked programs can be targetted.
Supported archs:

- x86
- x86_64
- arm
- aarch64

Example usage: https://asciinema.org/a/KkOHP2Jef0E6wViPCglkXLRcV

@ixty 2018


## installation
```shell
git clone https://github.com/ixty/mandibule
make
```


## usage
```shell
usage: ./mandibule <elf> [-a arg]* [-e env]* [-m addr] <pid>

loads an ELF binary into a remote process.

arguments:
    - elf: path of binary to inject into <pid>
    - pid: pid of process to inject into

options:
    -a arg: argument to send to injected program - can be repeated
    -e env: environment value sent to injected program - can be repeated
    -m mem: base address at which program is loaded in remote process, default=AUTO

Note: order of arguments must be respected (no getopt sry)
```


## example run
```shell
$ make x86_64

# in shell 1
$ ./target
> started.
......

# in shell 2
$ ./mandibule ./toinject `pidof target`
> target pid: 6266
> arg[0]: ./toinject
> args size: 51
> shellcode injection addr: 0x7f0f4719c000 size: 0x5000 (available: 0x195000)
> success attaching to pid 6266
> backed up mem & registers
> injected shellcode at 0x7f0f4719c000
> running shellcode..
> shellcode executed!
> restored memory & registers
> successfully injected shellcode into pid 6266

# back to shell 1
...
> target pid: 6266
> arg[0]: ./toinject
> args size: 51
> auxv len: 304
> auto-detected manual mapping address 0x55f6e1000000
> mapping './toinject' into memory at 0x55f6e1000000
> reading elf file './toinject'
> loading elf at: 0x55f6e1000000
> load segment addr 0x55f6e1000000 len 0x1000 => 0x55f6e1000000
> load segment addr 0x55f6e1200dd8 len 0x1000 => 0x55f6e1200000
> max vaddr 0x55f6e1212000
> loading interp '/lib64/ld-linux-x86-64.so.2'
> reading elf file '/lib64/ld-linux-x86-64.so.2'
> loading elf at: 0x55f6e1212000
> load segment addr 0x55f6e1212000 len 0x23000 => 0x55f6e1212000
> load segment addr 0x55f6e1435bc0 len 0x2000 => 0x55f6e1435000
> max vaddr 0x55f6e1448000
> eop 0x55f6e1212c20
> setting auxv
> set auxv[3] to 0x55f6e1000040
> set auxv[5] to 0x9
> set auxv[9] to 0x55f6e10006e0
> set auxv[7] to 0x55f6e1000000
> eop 0x55f6e1212c20
> starting ...

# oh hai from pid 6266
# arg[0]: ./toinject
# :)
# :)
# :)
# bye!
...........


```


## injection proces
mandibule has no dependency (not even libc) and is compiled with pie and fpie in order to make it fully relocatable.

This way we can copy mandibule's code into any process and it will be able to run as if it were a totally independant shellcode.

Here is how mandibule works:

- find an executable section in target process with enough space (~5Kb)
- attach to process with ptrace
- backup register state
- backup executable section
- inject mandibule code into executable section
- let the execution resume on our own injected code
- wait until exit() is called by the remote process
- restore registers & memory
- detach from process

In the remote process, mandibule does the following:

- read arguments, environment variables and other options from its own memory
- find a suitable memory address to load the target elf file if needed
- manually load & map the elf file into memory using only syscalls
- load the ld-linux interpreter if needed
- call the main function of the manually loaded binary


## tested on

- __x86__:      Linux debian 4.9.0-3-amd64 #1 SMP Debian 4.9.30-2+deb9u5 (2017-09-19) x86_64 GNU/Linux
- __x86_64__:   Linux debian 4.9.0-3-amd64 #1 SMP Debian 4.9.30-2+deb9u5 (2017-09-19) x86_64 GNU/Linux
- __arm64__:    Linux buildroot 4.13.6 #1 SMP Sat Mar 3 16:40:18 UTC 2018 aarch64 GNU/Linux
- __arm__:      Linux buildroot 4.11.3 #1 SMP Sun Mar 4 02:36:56 UTC 2018 armv7l GNU/Linux

arm & arm64 where tested using [arm_now](https://github.com/nongiach/arm_now) by [@chaignc](https://twitter.com/chaignc) to easily spawn qemu vms with the desired arch.
