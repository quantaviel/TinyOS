#!/usr/bin/bash
export PREFIX=/usr/local/i386elf
export TARGET=i386-elf
export PATH="$PREFIX/bin:$PATH"

if [ ! -f "binutils-2.44.tar.gz" ]; then
    wget https://mirrors.tuna.tsinghua.edu.cn/gnu/binutils/binutils-2.44.tar.gz
fi

if [ ! -f "gcc-12.1.0.tar.gz" ]; then
    wget https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-12.1.0/gcc-12.1.0.tar.gz
    tar -xf gcc-12.1.0.tar.gz
fi

if [ ! -d "binutils-2.44" ]; then
    tar -xf binutils-2.44.tar.gz
    mkdir -p binutils-build
    cd binutils-build
    ../binutils-2.44/configure \
        --prefix="$PREFIX" \
        --target="$TARGET" \
        --disable-nls \
        --disable-werror \
        --enable-languages=c,c++
    (make -j8) && (make install -j8)
    # cd ..
    # rm binutils-2.44
fi

if [ ! -d "gcc-build" ]; then
    cd gcc-12.1.0
    ./contrib/download_prerequisites
    cd ..
    mkdir -p gcc-build
    cd gcc-build
    ../gcc-12.1.0/configure \
        --prefix="$PREFIX" \
        --target="$TARGET" \
        --disable-nls \
        --enable-languages=c,c++ \
        --without-headers
    make all-gcc -j8 &&\
    make all-target-libgcc -j8 &&\
    make install-gcc -j8 &&\
    make install-target-libgcc -j8 
    echo export PATH="$PATH" >> ~/.bashrc
    # cd ..
    # rm gcc-12.1.0.tar.gz
fi
