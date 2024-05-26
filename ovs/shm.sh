kill $(pgrep ivshmem-server)
rm /dev/shm/ivshmem
~iwai/qemu/build/contrib/ivshmem-server/ivshmem-server -F -v &
chown libvirt-qemu:kvm /dev/shm/ivshmem /tmp/ivshmem_socket
