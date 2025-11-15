cd ~iwai/ivshmem-uio
git pull
make clean
make
modprobe uio
insmod ivshmem_uio.ko
