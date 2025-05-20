cd ~iwai/ivshmem-uio
make clean
make
modprobe uio
insmod ivshmem_uio.ko
