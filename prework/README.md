# Build GCC Cross Compiler

Download the source code of [binutils](https://ftp.gnu.org/gnu/binutils/) and [gcc](https://ftp.gnu.org/gnu/gcc/).

# Preparing for Build

Dependences:

+ Make
+ Bison
+ Flex
+ GMP
+ MPFR
+ MPC
+ Texinfo

```shell
sudo apt install build-essential \
            bison \
            flex  \
            libgmp3-dev \
            libmpc-dev \
            libmpfr-dev \
            texinfo
```

# Tools for run

+ qemu
+ grub-file
+ grub-mkrescue
+ xorriso
+ mtools

# Tools for debug

+ gdb
+ telnet