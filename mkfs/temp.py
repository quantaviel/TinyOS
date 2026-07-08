content = (
    "Welcome to TinyOS!\n" + '=' * 20 + "\n\nThis is a test file created by mkfs.py.\n\nDirectory structure:\n/bin/bash     - Simulated bash executable\n/etc/passwd  - User account information\n/home/user/ - User home directory\n/var/log/ - System logs\n\nEnjoy exploring TinyOS!\nHello Disk World! This is a message writen by os, not in disk, it will be writen to disk and to store consistency. The goal is to fill to 512 Byte, that call block alloc to test cross block write and read. So it is some meaningless words. My favorite anime charater is Hastsne Miku, which is a virtual singer."
)

print(len(content))