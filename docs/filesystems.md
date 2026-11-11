# VFS and tinyfs

The VFS implementation is maintained separately from the tinyfs backend.
Splitting tinyfs into modules does not change the VFS implementation.

The kernel calls `vfs::init()` before creating the first process. VFS
initializes the block cache, loads tinyfs, mounts its root at `/`, and opens
`/dev/tty` as the default terminal. It creates the device node through
`vfs::mknod` when the node is absent.

- `includes/fs/vfs.h`, `kernel/fs/vfs/`: inode and file objects, path lookup,
  root mounting, and file lifetime. `syscall.cc` translates process file
  descriptors to VFS file objects.
- `includes/fs/tinyfs/`, `kernel/fs/tinyfs/`: the tinyfs disk backend, split by
  responsibility. `tinyfs.h` is the umbrella header; `tinyfs.cc` loads the
  filesystem instance, binds VFS operations and handles device-node setup.
  The modules retain the original inode/directory/file algorithm structure.
- `includes/fs/buf.h`, `kernel/fs/buf.cc`: shared block cache, separate from the
  filesystem backend. Its key includes both device and block number.
- `kernel/tty/tty.cc`: character-device operations using VFS file objects.

`lookup`, `create`, `mkdir`, and `namei` return one owned inode reference.
`file_alloc` consumes that reference only on success. `dup_file` shares the same
open-file object and offset. The last file reference invokes the close callback
only if open succeeded, then drops the inode reference. The original `close` returns 1 and does not propagate the backend close error.
The original VFS also retains its existing flag handling and does not enforce
read/write access modes or implement `O_CREAT`. Creation is available through
the backend inode operations.

The static filesystem instance holds the root inode reference. The existing
VFS root pointer borrows it; paths acquire their own references. tinyfs caches inodes by `(super_block, inode number)` and writes
dirty metadata before recycling cached objects, including metadata of pinned
root/directories.

## tinyfs modules

Headers live in `includes/fs/tinyfs/`, with implementations in
`kernel/fs/tinyfs/`. Consumers include `<fs/tinyfs/tinyfs.h>`.

| Module | Responsibility |
| --- | --- |
| `tinyfs.h/.cc` | Public entry points, filesystem loading, device nodes and VFS operation binding |
| `types.h` | Shared disk layouts, backend inode/instance types and format constants |
| `inode.h/.cc` | Inode hash cache, references, metadata loading/writeback and inode-number allocation |
| `block.h/.cc` | Data-block allocation and bitmap rollback |
| `dir.h/.cc` | Name validation, directory lookup/insertion and inode creation |
| `file.h/.cc` | Ordinary-file block reads, writes, append and offsets |

The submodules call each other's declared backend operations directly; there
is no additional dispatch framework. The existing VFS operation tables remain
the dispatch boundary. The Makefile discovers the new `.cc` files recursively,
and the host integration test links all tinyfs module sources.

## Disk compatibility and limits

The existing magic, 512-byte blocks, 128-byte disk inodes and 64-byte directory
entries are retained. All twelve zone entries are direct block addresses, so a
file or directory occupies at most 6144 bytes. Names are at most 59 bytes plus
NUL. The inode table has four inodes per block, so the default 64-block table
holds 256 inodes. Allocation is bounded by the one-block bitmap capacity;
although mkfs creates a 64 MiB image, the current data bitmap addresses only the
first 4096 absolute block numbers. These limits are checked rather than allowing
array or bitmap overflow. Data bitmap indices are absolute LBAs, matching mkfs.

The first four bytes of the old inode padding now hold a character-device ID.
ID 1 denotes the TTY. No pointers are persisted. Images with a legacy
`/dev/tty` device ID of 0 must be regenerated.

Current scope is a single ATA disk (device 1), root inode handoff, absolute paths,
backend file/directory creation, ordinary file read/write/append and TTY output.
Mount-aware path traversal remains outside the scope of this backend refactor.
Relative paths, symlinks, unlink, dynamic unmount, keyboard-to-TTY input,
permission checks against user credentials, concurrency and crash consistency
are not implemented. Failed ATA writes remain dirty in the block cache, but
end-to-end write-error reporting and durable flush guarantees remain incomplete.
Kernel syscall pointers retain the existing direct user-buffer handling.

## Validation

Run `make -j4` for the freestanding kernel and ISO. Run `tests/run-vfs.sh` for
host integration checks with AddressSanitizer and UndefinedBehaviorSanitizer.
The tests compile the production VFS, tinyfs, buffer cache and TTY implementation;
ATA I/O is backed by a temporary mkfs image and VGA output is intercepted. A
second process reloads the modified image with empty caches to check persistence.
The host tests do not validate privileged hardware I/O or process scheduling.
