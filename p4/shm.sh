cd ~iwai/ivshmem-uio
git pull
make clean

if sudo dmesg | grep -iq 'Memory Encryption Features active:'; then
    make -f Makefile.sev
else
    make
fi

modprobe uio
insmod ivshmem_uio.ko
