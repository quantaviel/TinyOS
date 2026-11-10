#!/usr/bin/env bash

set -Eeuo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly HOST_OS="$(uname -s)"

log() {
    printf '[toolchain] %s\n' "$*"
}

die() {
    printf '[toolchain] error: %s\n' "$*" >&2
    exit 1
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

verify_toolchain() {
    local prefix="$1"
    local tool

    for tool in gcc g++ as ld objdump; do
        command_exists "${prefix}${tool}" || die "missing ${prefix}${tool}"
    done

    log "C compiler: $(${prefix}gcc --version | head -n 1)"
    log "C++ compiler: $(${prefix}g++ --version | head -n 1)"
    log "target: $(${prefix}gcc -dumpmachine)"
}

install_with_homebrew() {
    command_exists brew || die 'Homebrew is required on macOS: https://brew.sh/'

    if ! command_exists i686-elf-g++ \
        || ! command_exists i686-elf-ld \
        || ! command_exists i686-elf-grub-file \
        || ! command_exists i686-elf-grub-mkrescue; then
        log 'installing the i686-elf C/C++ toolchain with Homebrew'
        brew install i686-elf-gcc i686-elf-grub
    else
        log 'Homebrew toolchain is already installed; skipping'
    fi

    verify_toolchain 'i686-elf-'
    command_exists i686-elf-grub-file || die 'missing i686-elf-grub-file'
    command_exists i686-elf-grub-mkrescue || die 'missing i686-elf-grub-mkrescue'
}

download() {
    local url="$1"
    local output="$2"
    local partial="${output}.part"

    if [[ -f "$output" ]]; then
        return
    fi

    log "downloading $(basename -- "$output")"
    rm -f -- "$partial"
    if command_exists curl; then
        curl --fail --location --retry 3 --output "$partial" "$url"
    elif command_exists wget; then
        wget --output-document="$partial" "$url"
    else
        die 'curl or wget is required to download the source archives'
    fi
    mv -- "$partial" "$output"
}

extract() {
    local archive="$1"
    local directory="$2"

    if [[ ! -d "$directory" ]]; then
        log "extracting $(basename -- "$archive")"
        tar -xf "$archive"
    fi
}

detect_jobs() {
    local jobs=''

    if command_exists getconf; then
        jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
    fi
    if [[ -z "$jobs" ]] && command_exists sysctl; then
        jobs="$(sysctl -n hw.logicalcpu 2>/dev/null || true)"
    fi
    if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
        jobs=1
    fi
    printf '%s\n' "$jobs"
}

build_from_source() {
    local target="${TARGET:-i386-elf}"
    local prefix="${PREFIX:-${SCRIPT_DIR}/toolchain}"
    local binutils_version="${BINUTILS_VERSION:-2.44}"
    local gcc_version="${GCC_VERSION:-12.1.0}"
    local jobs="${JOBS:-$(detect_jobs)}"
    local binutils_archive="binutils-${binutils_version}.tar.gz"
    local binutils_source="binutils-${binutils_version}"
    local binutils_build="binutils-build"
    local gcc_archive="gcc-${gcc_version}.tar.gz"
    local gcc_source="gcc-${gcc_version}"
    local gcc_build="gcc-build"
    local toolchain_prefix="${prefix}/bin/${target}-"

    command_exists make || die 'make is required'
    command_exists tar || die 'tar is required'

    mkdir -p -- "$prefix"
    cd -- "$SCRIPT_DIR"

    download \
        "https://mirrors.tuna.tsinghua.edu.cn/gnu/binutils/${binutils_archive}" \
        "$binutils_archive"
    download \
        "https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-${gcc_version}/${gcc_archive}" \
        "$gcc_archive"

    extract "$binutils_archive" "$binutils_source"
    extract "$gcc_archive" "$gcc_source"

    if [[ ! -x "${toolchain_prefix}as" || ! -x "${toolchain_prefix}ld" ]]; then
        log "building binutils ${binutils_version} for ${target}"
        mkdir -p -- "$binutils_build"
        (
            cd -- "$binutils_build"
            "../${binutils_source}/configure" \
                --prefix="$prefix" \
                --target="$target" \
                --disable-nls \
                --disable-werror
            make -j"$jobs"
            make install
        )
    else
        log 'binutils is already installed; skipping'
    fi

    if [[ ! -x "${toolchain_prefix}gcc" || ! -x "${toolchain_prefix}g++" ]]; then
        if [[ ! -f "${gcc_source}/.prerequisites-downloaded" ]]; then
            log 'downloading GCC prerequisites'
            (
                cd -- "$gcc_source"
                ./contrib/download_prerequisites
                touch .prerequisites-downloaded
            )
        fi

        log "building GCC ${gcc_version} for ${target}"
        mkdir -p -- "$gcc_build"
        (
            cd -- "$gcc_build"
            "../${gcc_source}/configure" \
                --prefix="$prefix" \
                --target="$target" \
                --disable-nls \
                --disable-multilib \
                --enable-languages=c,c++ \
                --without-headers
            make -j"$jobs" all-gcc
            make -j"$jobs" all-target-libgcc
            make install-gcc
            make install-target-libgcc
        )
    else
        log 'GCC is already installed; skipping'
    fi

    verify_toolchain "$toolchain_prefix"
    log "add the toolchain to this shell with: export PATH=\"${prefix}/bin:\$PATH\""
    log "build TinyOS with: make CROSS_PREFIX=${toolchain_prefix}"
}

main() {
    if [[ "$HOST_OS" == 'Darwin' && "${BUILD_FROM_SOURCE:-0}" != '1' ]]; then
        install_with_homebrew
    else
        build_from_source
    fi
}

main "$@"
