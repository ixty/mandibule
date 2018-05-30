# ======================================================================== #
# author:  ixty                                                       2018 #
# project: mandibule                                                       #
# licence: beerware                                                        #
# ======================================================================== #

# mandibule makefile

CFLAGS 	+= -D_GNU_SOURCE -std=gnu99
CFLAGS 	+= -static-libgcc -lgcc
CFLAGS 	+= -I icrt/ -I code/
CFLAGS 	+= -fno-common -fno-stack-protector -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables
# CFLAGS  += -Wall -Werror -Wpedantic
PORTABLE = -pie -fPIE -fno-builtin
MACHINE  = $(shell uname -m)

# automaticaly target current arch
all: $(MACHINE)

# PC 32 bits
# add -m32 so we can compile it on amd64 as well
i386: x86
i486: x86
i686: x86
x86: clean
	$(CC) $(CFLAGS) $(PORTABLE) -nostdlib -m32 -o mandibule mandibule.c
	$(CC) $(CFLAGS) $(PORTABLE) -m32 -o toinject samples/toinject.c
	$(CC) $(CFLAGS) -m32 -o target samples/target.c

# PC 64 bits
amd64: x86_64
x86_64: clean
	$(CC) $(CFLAGS) $(PORTABLE) -nostdlib -o mandibule mandibule.c
	$(CC) $(CFLAGS) $(PORTABLE) -o toinject samples/toinject.c
	$(CC) $(CFLAGS) -o target samples/target.c


# ARM 32 bits
# unable to compile arm without nostdlib (uidiv divmod ...)
arm: armv7l
armhf: armv7l
armv7: armv7l
armv7l: clean
	$(CC) $(CFLAGS) $(PORTABLE) -nostartfiles -o mandibule mandibule.c
	$(CC) $(CFLAGS) $(PORTABLE) -o toinject samples/toinject.c
	$(CC) $(CFLAGS) -o target samples/target.c

# ARM 64 bits
arm64: aarch64
armv8: aarch64
armv8l: aarch64
aarch64: clean
	$(CC) $(CFLAGS) $(PORTABLE) -nostdlib -o mandibule mandibule.c
	$(CC) $(CFLAGS) $(PORTABLE) -o toinject samples/toinject.c
	$(CC) $(CFLAGS) -o target samples/target.c

clean:
	rm -rf mandibule target toinject

