sudo kill $(pgrep ivshmem-server)
sudo rm /dev/shm/ivshmem
sudo ~iwai/qemu/build/contrib/ivshmem-server/ivshmem-server -F -v &
sudo chown libvirt-qemu:kvm /dev/shm/ivshmem /tmp/ivshmem_socket
