cd ~iwai/ivshmem-uio
git pull
make clean

if sudo dmesg | grep -iq 'Memory Encryption Features active:'; then
    mv ivshmem_uio-sev.c ivshmem_uio.c 
fi

make
modprobe uio
insmod ivshmem_uio.ko
